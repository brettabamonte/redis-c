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

// const size_t k_max_msg = 4096;

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

// static int32_t query(int fd, const char *text)
// {
//     uint32_t len = (uint32_t)strlen(text);

//     if (len > k_max_msg) {
//         return -1;
//     }

//     char wBuf[4 + k_max_msg];
//     memccpy(wBuf, &len, 4);
//     memccpy(&wBuf[4], text, len);

//     if(int32_t err = write_all(fd, wBuf, 4 + len)) {
//         return err;
//     }
// }

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

    if (rv)
    {
        die("connect()");
    }

    char msg[] = "hello";
    write(fd, msg, strlen(msg));

    char rBuf[64] = {};
    ssize_t n = read(fd, rBuf, sizeof(rBuf) - 1);
    if (n < 0)
    {
        die("read()");
    }

    printf("server says: %s\n", rBuf);
    close(fd);

    return 0;
}