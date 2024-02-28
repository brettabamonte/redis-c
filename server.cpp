#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

static void msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
}

static void do_something(int connfd)
{
    char rBuf[64] = {};
    ssize_t n = read(connfd, rBuf, sizeof(rBuf) - 1);
    if (n < 0)
    {
        msg("read() error");
        return;
    }

    printf("client says: %s\n", rBuf);

    char wBuf[] = "world";
    write(connfd, wBuf, strlen(wBuf));
}

static void die(const char *msg)
{
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
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

        do_something(connfd);
        close(connfd);
    }

    return 0;
}