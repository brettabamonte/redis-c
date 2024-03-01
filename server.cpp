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

const size_t k_max_msg = 4096;

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

enum
{
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2
};

struct Connection
{
    int fd = -1;
    // State of connection
    uint32_t state = 0;
    // Buffer for reading
    size_t read_buffer_size = 0;
    uint8_t read_buffer[4 + k_max_msg];
    // Buffer for writing
    size_t write_buffer_size = 0;
    size_t write_buffer_sent = 0;
    uint8_t write_buffer[4 + k_max_msg];
};

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

    // Accept connections
    while (true)
    {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0)
        {
            continue; // error
        }

        while (true)
        {
            int32_t err = one_request(connfd);

            if (err)
            {
                break;
            }
        }

        close(connfd);
    }

    return 0;
}