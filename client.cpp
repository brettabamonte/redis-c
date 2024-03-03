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
#include <string>
#include <vector>

// FUTURE UPDATES:

const size_t k_max_msg = 4096;

static void msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
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

static void die(const char *msg)
{
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static int32_t send_req(int fd, const std::vector<std::string> &cmd)
{
    uint32_t len = 4;

    for (const std::string &s : cmd)
    {
        len += 4 + s.size();
    }

    if (len > k_max_msg)
    {
        return -1;
    }

    char wBuf[4 + k_max_msg];
    memcpy(wBuf, &len, 4);
    uint32_t n = cmd.size();
    memcpy(&wBuf[4], &n, len);
    size_t cur = 8;

    for (const std::string &s : cmd)
    {
        uint32_t p = (uint32_t)s.size();
        memcpy(&wBuf[cur], &p, 4);
        memcpy(&wBuf[cur + 4], s.data(), s.size());
        cur += 4 + s.size();
    }

    return write_all(fd, wBuf, 4 + len);
}

static int32_t read_res(int fd)
{
    uint32_t len = 0;
    char rBuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(fd, rBuf, 4);
    if (err)
    {
        if (errno == 0)
        {
            msg("EOF");
        }
        else
        {
            msg("read() error");
        }
    }

    memcpy(&len, rBuf, 4);
    if (len > k_max_msg)
    {
        msg("msg too long");
        return -1;
    }

    err = read_full(fd, &rBuf[4], len);

    if (err)
    {
        msg("read() error for body");
        return err;
    }

    rBuf[4 + len] = '\0';
    printf("server says: %s\n", &rBuf[4]);

    return 0;
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
    {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));

    const char *query_list[4] = {"hello1", "hello2", "hello3", "brett says hi"};
    for (size_t i = 0; i < 4; ++i)
    {
        int32_t err = send_req(fd, query_list[i]);
        if (err)
        {
            goto L_DONE;
        }
    }

    for (size_t i = 0; i < 4; ++i)
    {
        int32_t err = read_res(fd);
        if (err)
        {
            goto L_DONE;
        }
    }

L_DONE:
    close(fd);
    return 0;
}