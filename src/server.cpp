/*

Serialization Protocol:
    TLV - Type Length Value

    Type of Data Being Transmitted/Received - Length in Bytes of Data - Payload with Data

*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <assert.h>
#include <fcntl.h>
#include <vector>
#include <stdbool.h>
#include <poll.h>
#include <map>
#include <string>
#include "hashtable.h"
#include "utils.h"

#define container_of(ptr, type, member) ({ \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *) __mptr - offsetof(type, member)); \
})

// FUTURE UPDATES:
// 1. Use epoll instead of poll
// 2. Perform memove only before a read
// 3. Buffer multiple response and flush with a single write call (buffer limit may get full, flush then)

const size_t k_max_msg = 4096;
enum
{
    STATE_REQ = 0, // Reading requests
    STATE_RES = 1, // Sending requests
    STATE_END = 2  // Terminate connection
};

enum
{
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

enum {
    ERR_UNKNOWN = 1,
    ERR_2BIG = 2,
};

struct Entry {
    struct HNode node;
    std::string key;
    std::string val;
};

static std::map<std::string, std::string> g_map;

static bool entry_eq(HNode *lhs, HNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
}

static void out_nil(std::string &out) {
    out.push_back(SER_NIL);
}

static void out_str(std::string &out, const std::string &val) {
    out.push_back(SER_STR);
    uint32_t len = (uint32_t)val.size();
    out.append((char *)&len, 4);
    out.append(val);
}

static void out_int(std::string &out, int64_t val) {
    out.push_back(SER_INT);
    out.append((char *)&val, 8);
}

static void out_err(std::string &out, int32_t code, const std::string &msg) {
    out.push_back(SER_ERR);
    out.append((char *)&code, 4);
    uint32_t len = (uint32_t)msg.size();
    out.append((char *) &len, 4);
    out.append(msg);
}

static void out_arr(std::string &out, uint32_t n) {
    out.push_back(SER_ARR);
    out.append((char *)&n, 4);
}

struct Connection
{
    int fd = -1;
    // State of connection
    uint32_t state = 0;
    // Buffer for readin
    size_t read_buffer_size = 0;
    uint8_t read_buffer[4 + k_max_msg];
    // Buffer for writing
    size_t write_buffer_size = 0;
    size_t write_buffer_sent = 0;
    uint8_t write_buffer[4 + k_max_msg];
};

static struct {
    HMap db;
} g_data;

static void state_res(Connection *conn);
static void state_req(Connection *conn);
static bool try_flush_buffer(Connection *conn);
static bool try_fill_buffer(Connection *conn);

static void msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg)
{
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

// Sets a file descriptor to non-blocking mode
static void set_fd_nb(int fd)
{
    errno = 0;

    int flags = fcntl(fd, F_GETFL, 0);

    if (errno)
    {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;

    (void)fcntl(fd, F_SETFL, flags);
    if (errno)
    {
        die("fcntl error");
    }
}

static void conn_put(std::vector<Connection *> &fd_to_connection, struct Connection *conn)
{
    if (fd_to_connection.size() <= (size_t)conn->fd)
    {
        fd_to_connection.resize(conn->fd + 1);
    }

    fd_to_connection[conn->fd] = conn;
}

static int32_t accept_new_conn(std::vector<Connection *> &fd_to_connection, int fd)
{
    // Accept
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);

    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);

    if (connfd < 0)
    {
        msg("accept() error");
        return -1;
    }

    // Set new connection fd to nonblocking mode
    set_fd_nb(connfd);

    // Create the Connection struct
    struct Connection *conn = (struct Connection *)malloc(sizeof(struct Connection));

    if (!conn)
    {
        close(connfd);
        return -1;
    }

    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->read_buffer_size = 0;
    conn->write_buffer_size = 0;
    conn->write_buffer_sent = 0;
    conn_put(fd_to_connection, conn);

    return 0;
}

static void h_scan(HTab *tab, void (*f)(HNode *, void *), void *arg) {
    if(tab->size == 0) {
        return;
    }

    for(size_t i = 0; i < tab->mask + 1; ++i) {
        HNode *node = tab->tab[i];
        while(node) {
            f(node, arg);
            node = node->next;
        }
    }
}

static void cb_scan(HNode *node, void *arg) {
    std::string &out = *(std::string *)arg;
    out_str(out, container_of(node, Entry, node)->key);
}

static void do_get(std::vector<std::string> &cmd, std::string &out) {
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);

    if(!node) {
        return out_nil(out);
    }

    const std::string &val = container_of(node, Entry, node)->val;
    out_str(out, val);
}

static void do_set(std::vector<std::string> &cmd, std::string &out) {  
    //Create key
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *) key.key.data(), key.key.size());

    //Find node
    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);

    if(node) {
        //We found the node. Swap it's current val to new one passed in args
        container_of(node, Entry, node)->val.swap(cmd[2]);
    } else {
        //Create new entry into hashtable.
        Entry *entry = new Entry();
        entry->key.swap(key.key);
        entry->node.hcode = key.node.hcode;
        entry->val.swap(cmd[2]);
        hm_insert(&g_data.db, &entry->node);
    }

    return out_nil(out);
}

static void do_del(std::vector<std::string> &cmd, std::string &out)
{    
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_pop(&g_data.db, &key.node, &entry_eq);

    if(node) {
        delete container_of(node, Entry, node);
    }

    //Returns whether or not deletion took place
    return out_int(out, node ? 1 : 0);
}

static void do_keys(std::vector<std::string> &cmd, std::string &out) {
    (void)cmd;
    out_arr(out, (uint32_t)hm_size(&g_data.db));
    h_scan(&g_data.db.h1, &cb_scan, &out);
    h_scan(&g_data.db.h2, &cb_scan, &out);
}

static int32_t parse_req(const uint8_t *data, size_t len, std::vector<std::string> &out)
{
    if (len < 4)
    {
        return -1;
    }

    uint32_t n = 0;
    memcpy(&n, &data[0], 4);
    if (n > k_max_msg)
    {
        return -1;
    }

    size_t pos = 4;
    while (n--)
    {
        if (pos + 4 > len)
        {
            return -1;
        }
        uint32_t sz = 0;
        memcpy(&sz, &data[pos], 4);
        if (pos + 4 + sz > len)
        {
            return -1;
        }

        out.push_back(std::string((char *)&data[pos + 4], sz));
        pos += 4 + sz;
    }

    if (pos != len)
    {
        return -1; // There is misc trailing garbage
    }
    return 0;
}

static bool cmd_is(const std::string &word, const char *cmd)
{
    return 0 == strcasecmp(word.c_str(), cmd);
}

static void do_request(std::vector<std::string> &cmd, std::string &out) {
    if(cmd.size() == 1 && cmd_is(cmd[0], "keys")) {
        //do keys
        do_keys(cmd, out);
    } else if(cmd.size() == 2 && cmd_is(cmd[0], "get")) {
        do_get(cmd, out);
    } else if(cmd.size() == 3 && cmd_is(cmd[0], "set")) {
        do_set(cmd, out);
    } else if(cmd.size() == 2 && cmd_is(cmd[0], "del")) {
        do_del(cmd, out);
    } else {
        //cmd isn't recognized
        out_err(out, ERR_UNKNOWN, "Unknown cmd");
    }
}

static bool try_one_request(Connection *conn) {
    // Try to parse a request from buffer
    if (conn->read_buffer_size < 4)
    {
        // Not enough data in buffer. Retry in next iteration
        return false;
    }

    uint32_t len = 0;

    memcpy(&len, &conn->read_buffer[0], 4);

    if (len > k_max_msg)
    {
        msg("too long");
        conn->state = STATE_END;
        return false;
    }

    if (4 + len > conn->read_buffer_size)
    {
        // Not enough data in buffer. Retry in next iteration
        return false;
    }

    //Parse request
    std::vector<std::string> cmd;
    if(0 != parse_req(&conn->read_buffer[4], len, cmd)) {
        msg("bad request");
        conn->state = STATE_END;
        return false;
    }

    // 1 request, generate response
    std::string out;
    do_request(cmd, out);

    //But response into the buffer
    if(4 + out.size() > k_max_msg) {
        out.clear();
        out_err(out, ERR_2BIG, "Response is too big");
    }
    
    uint32_t wlen = (uint32_t)out.size();
    memcpy(&conn->write_buffer[0], &wlen, 4);
    memcpy(&conn->write_buffer[4], out.data(), out.size());
    conn->write_buffer_size = 4 + wlen;

    // Remove the request from the buffer
    // FIXME: memmove in prod isn't efficient
    size_t remain = conn->read_buffer_size - 4 - len;
    if (remain)
    {
        memmove(conn->read_buffer, &conn->read_buffer[4 + len], remain);
    }

    conn->read_buffer_size = remain;

    // Change state
    conn->state = STATE_RES;
    state_res(conn);

    // Continue outer loop if the request was fully processed
    return (conn->state == STATE_REQ);
}

// Fill read buffer with data. If buffer get's full, we process data immediately to free buffer space
// This is why the function is looped until we hit EAGAIN
// Syscalls (read, etc.) are retried after EINTR. EINTR means syscall was interrupted
static bool try_fill_buffer(Connection *conn)
{
    // Try to fill the buffer
    assert(conn->read_buffer_size < sizeof(conn->read_buffer));

    ssize_t rv = 0;

    do
    {
        size_t cap = sizeof(conn->read_buffer) - conn->read_buffer_size;
        rv = read(conn->fd, &conn->read_buffer[conn->read_buffer_size], cap);
    } while (rv < 0 && errno == EINTR);

    if (rv < 0 && errno == EAGAIN)
    {
        // Got EAGAIN, so stop
        return false;
    }

    if (rv < 0)
    {
        msg("read() error");
        conn->state = STATE_END;
        return false;
    }

    if (rv == 0)
    {
        if (conn->read_buffer_size > 0)
        {
            msg("unexpected EOF");
        }
        else
        {
            msg("EOF");
        }

        conn->state = STATE_END;
        return false;
    }

    conn->read_buffer_size += (size_t)rv;
    assert(conn->read_buffer_size <= sizeof(conn->read_buffer));

    // Process req one by one, pipelining
    while (try_one_request(conn))
    {
    }
    return (conn->state == STATE_REQ);
}

static void state_req(Connection *conn)
{
    while (try_fill_buffer(conn))
    {
    }
}

static void state_res(Connection *conn)
{
    while (try_flush_buffer(conn))
    {
    }
}

static bool try_flush_buffer(Connection *conn)
{
    ssize_t rv = 0;

    do
    {
        ssize_t remain = conn->write_buffer_size - conn->write_buffer_sent;
        rv = write(conn->fd, &conn->write_buffer[conn->write_buffer_sent], remain);
    } while (rv < 0 && errno == EAGAIN);

    if (rv < 0 && errno == EAGAIN)
    {
        // Got EAGAIN, stop
        return false;
    }

    if (rv < 0)
    {
        msg("write() error");
        conn->state = STATE_END;
        return false;
    }

    conn->write_buffer_sent += (size_t)rv;
    assert(conn->write_buffer_sent <= conn->write_buffer_size);
    if (conn->write_buffer_sent == conn->write_buffer_size)
    {
        // Response fully sent, change state back
        conn->state = STATE_REQ;
        conn->write_buffer_sent = 0;
        conn->write_buffer_size = 0;
        return false;
    }

    // Still go some data in the write buffer, keep writing
    return true;
}

static void connection_io(Connection *conn)
{
    if (conn->state == STATE_REQ)
    {
        state_req(conn);
    }
    else if (conn->state == STATE_RES)
    {
        state_res(conn);
    }
    else
    {
        assert(0);
    }
}

static int32_t read_full(int fd, char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0)
        {
            return -1; // Error or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0)
        {
            return -1; // error
        }

        assert((ssize_t)rv <= n);
        n -= (ssize_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t one_request(int connfd)
{
    // Create buffer to read in data. Add size for len and end
    char rBuf[4 + k_max_msg + 1];
    errno = 0;

    // Read in first 4 bytes that specify len of msg
    int32_t err = read_full(connfd, rBuf, 4);

    // Check if error occured when reading in len of msg
    if (err)
    {
        // We reached EOF
        if (errno == 0)
        {
            msg("EOF");
        }
        else
        {
            msg("read() error");
        }
        return err;
    }

    // Assume little endian. Copy read buffer to length
    uint32_t len = 0;
    memcpy(&len, rBuf, 4);

    // Check msg size
    if (len > k_max_msg)
    {
        msg("msg too long");
        return -1;
    }

    // Read in request body
    err = read_full(connfd, &rBuf[4], len);

    // Check for error that occured during read in
    if (err)
    {
        msg("read() error");
        return err;
    }

    // Do something
    rBuf[4 + len] = '\0'; // Add end of msg char
    printf("client says: %s\n", &rBuf[4]);

    // Reply using same protocol
    const char reply[] = "world";
    char wBuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wBuf, &len, 4);        // Copy len of write msg to write buffer
    memcpy(&wBuf[4], reply, len); // Copy msg to write buffer
    return write_all(connfd, wBuf, 4 + len);
}


int main()
{
    // Creates a socket and returns file descriptor
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    // Configure socket. Allows bind() to reuse local addresses if supported by the underlying protocol.
    // Without it, bind() fails when server restarts b/c IP + Port # are reserved for 30 - 120 seconds so in-flight packets get dropped by default
    // See https://stackoverflow.com/questions/3229860/what-is-the-meaning-of-so-reuseaddr-setsockopt-option-linux/3233022#3233022
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Bind. Handles IPv4 addresses
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv)
    {
        die("bind()");
    }

    // Listen. listen() handles TCP handshakes and places established connections in a queue
    rv = listen(fd, SOMAXCONN);
    if (rv)
    {
        die("listen()");
    }

    // Map of all client connections, keyed with fd
    std::vector<Connection *> fd_to_connections;

    // Set the listening fd to nonblocking mode
    set_fd_nb(fd);

    // Event loop
    std::vector<struct pollfd> poll_args;
    while (true)
    {
        // Prepare the args of the poll()
        poll_args.clear();

        // For convenience, the listening fd is put in the 1st position
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);

        // Connection fds
        for (Connection *conn : fd_to_connections)
        {
            if (!conn)
            {
                continue;
            }

            struct pollfd pfd = {};
            pfd.fd = conn->fd;
            pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
            pfd.events = pfd.events | POLLERR;
            poll_args.push_back(pfd);
        }

        // Poll for active fds
        // The timeout arg doesn't matter here
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
        if (rv < 0)
        {
            die("poll");
        }

        // Process active connections
        for (size_t i = 1; i < poll_args.size(); ++i)
        {
            if (poll_args[i].revents)
            {
                Connection *conn = fd_to_connections[poll_args[i].fd];
                connection_io(conn);
                if (conn->state == STATE_END)
                {
                    // Client closed normall or bad thing happened
                    fd_to_connections[conn->fd] = NULL;
                    (void)close(conn->fd);
                    free(conn);
                }
            }
        }

        // Try to accept a new connection if the listening fd is active
        if (poll_args[0].revents)
        {
            (void)accept_new_conn(fd_to_connections, fd);
        }
    }

    return 0;
}