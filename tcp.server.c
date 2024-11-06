#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#define PORT 1234
#define HOST_ADRESS 0
#define YES 1
#define NO 0
#define K_MAX_MSG 4096

static void die(char *msg);
static void errmsg(char *msg);
static void processing(int connfd);
static int32_t one_request(int connfd);
static int32_t write_all(int fd, char *buf, size_t n);
static int32_t read_full(int fd, char *buf, size_t n);

int main(void)
{
    // // a message maximum size
    // const size_t k_max_msg = 4096;

    // socket file descriptor created: ipv4, tcp
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    // set socket option SO_REUSEADDR to 1
    int value = YES;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

    // bind to adress 0.0.0.0:1234
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(PORT);
    addr.sin_addr.s_addr = ntohl(HOST_ADRESS);

    int rv = bind(fd, (struct sockaddr *)&addr, sizeof(addr));

    if (rv)
    {
        die("bind()");
    }

    // listening to perform handshakes
    rv = listen(fd, SOMAXCONN);
    printf("[TCP SERVER LISTENING ON PORT %d]\n", PORT);

    while (true)
    {
        // accepting new connections in a loop
        struct sockaddr_in client_addr = {};
        socklen_t client_addr_len = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &client_addr_len);
        printf("[NEW CONNECTION ACCEPTED]: %d\n", connfd);

        if (connfd < 0)
        {
            // failure on accepting connection
            continue;
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
}

static void errmsg(char *msg)
{
    fprintf(stderr, "[ERROR]: %s\n", msg);
    return;
}

static void die(char *msg)
{
    int err = errno;
    fprintf(stderr, "[ERROR]: [%d] %s\n", err, msg);
    abort();
}

static void processing(int connfd)
{
    char rbuf[64];
    ssize_t n = read(connfd, rbuf, sizeof(rbuf));

    if (n < 0)
    {
        errmsg("read() error");
    }

    printf("Client says: %s", rbuf);

    char wbuf[] = "world\n";
    write(connfd, wbuf, strlen(wbuf));
}

static int32_t read_full(int fd, char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0)
        {
            return -1;
        }

        assert((size_t)(rv) <= n);

        n -= (size_t)rv;
        buf += rv;
    }

    return 0;
}

static int32_t write_all(int fd, char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0)
        {
            return -1;
        }

        assert((size_t)(rv) <= n);

        n -= (size_t)rv;
        buf += rv;
    }

    return 0;
}

static int32_t one_request(int connfd)
{
    errno = 0;
    char rbuf[4 + K_MAX_MSG + 1];
    uint32_t len;

    int32_t err = read_full(connfd, rbuf, 4);
    if (err)
    {
        if (errno == 0)
        {
            errmsg("EOF");
        }
        else
        {
            errmsg("read() error");
        }

        return err;
    }

    memcpy(&len, rbuf, 4);

    if (len > K_MAX_MSG)
    {
        errmsg("Request too long");
        return -1;
    }

    err = read_full(connfd, &rbuf[4], len);
    if (err)
    {
        errmsg("read() error");
        return -1;
    }

    rbuf[4 + K_MAX_MSG] = '\0';

    printf("Client Says: %s\n", &rbuf[4]);

    const char reply[] = "world";

    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);

    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);

    return write_all(connfd, wbuf, 4 + len);
}