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

#define SERVER_PORT 1234
#define SERVER_HOST_ADRESS INADDR_LOOPBACK
#define YES 1
#define NO 0
#define K_MAX_MSG 4096

static void die(char *msg);
static void errmsg(char *msg);
static int32_t write_all(int fd, char *buf, size_t n);
static int32_t read_full(int fd, char *buf, size_t n);
static int32_t query(int fd, char *text);

int main(void)
{
    // socket file descriptor created: ipv4, tcp
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0)
    {
        die("socket()");
    }

    // bind to adress 0.0.0.0:1234
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(SERVER_PORT);
    addr.sin_addr.s_addr = ntohl(SERVER_HOST_ADRESS);

    int rv = connect(fd, (struct sockaddr *)&addr, sizeof(addr));

    if (rv)
    {
        die("connect()");
    }

    int32_t err = query(fd, "hello1");
    if (err)
    {
        goto L_DONE;
    }

    err = query(fd, "hello2");
    if (err)
    {
        goto L_DONE;
    }

    err = query(fd, "hello3");
    if (err)
    {
        goto L_DONE;
    }

L_DONE:
    close(fd);
    return 0;
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

static int32_t query(int fd, char *text)
{
    uint32_t len = (uint32_t)strlen(text);
    if (len > K_MAX_MSG)
    {
        errmsg("Request too long");
        return -1;
    }
    char wbuf[4 + len];

    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], text, len);

    uint32_t err = write_all(fd, wbuf, 4 + len);

    if (err)
    {
        return err;
    }

    char rbuf[4 + K_MAX_MSG + 1];
    errno = 0;
    err = read_full(fd, rbuf, 4);
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
    memcpy(&len, rbuf, 4); // assume little endian
    if (len > K_MAX_MSG)
    {
        errmsg("too long");
        return -1;
    }
    // reply body
    err = read_full(fd, &rbuf[4], len);
    if (err)
    {
        errmsg("read() error");
        return err;
    }
    // do something
    rbuf[4 + len] = '\0';
    printf("server says: %s\n", &rbuf[4]);
    return 0;
}