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
#include <vector>
#include <map>
#include <string>

#define SERVER_PORT 1234
#define SERVER_HOST_ADRESS INADDR_LOOPBACK
#define YES 1
#define NO 0
#define K_MAX_MSG 4096

static void die(const char *msg);
static void errmsg(const char *msg);
static int32_t write_all(int fd, char *buf, size_t n);
static int32_t read_full(int fd, char *buf, size_t n);
static int32_t send_req(int fd, const std::vector<std::string> &cmd);
static int32_t read_res(int fd);

int main(int argc, char **argv)
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

    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));

    if (rv)
    {
        die("connect()");
    }

    std::vector<std::string> cmd;
    for (int i = 1; i < argc; ++i)
    {
        cmd.push_back(argv[i]);
    }

    int err = send_req(fd, cmd);
    if (err)
    {
        printf("err %i", err);
        goto L_DONE;
    }

    err = read_res(fd);
    if (err)
    {
        printf("err %i", err);
        goto L_DONE;
    }

L_DONE:
    printf("About closing the connection...\n");
    close(fd);
    return 0;
}

static void errmsg(const char *msg)
{
    fprintf(stderr, "[ERROR]: %s\n", msg);
    return;
}

static void die(const char *msg)
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

static int32_t send_req(int fd, const std::vector<std::string> &cmd)
{
    uint32_t len = 4;
    for (const std::string &s : cmd)
    {
        len += 4 + s.size();
    }

    if (len > K_MAX_MSG)
    {
        return -1;
    }

    char wbuf[4 + K_MAX_MSG];
    memcpy(&wbuf[0], &len, 4);
    uint32_t n = cmd.size();
    memcpy(&wbuf[4], &n, 4);

    size_t cur = 4 + 4;

    for (const std::string &s : cmd)
    {
        uint32_t p = (uint32_t)s.size();
        memcpy(&wbuf[cur], &p, 4);
        memcpy(&wbuf[cur + 4], s.data(), s.size());
        cur += 4 + s.size();
    }

    return write_all(fd, wbuf, 4 + len);
}

static int32_t read_res(int fd)
{
    // 4 bytes header
    char rbuf[4 + K_MAX_MSG + 1];
    errno = 0;
    int32_t err = read_full(fd, rbuf, 4);
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

    uint32_t len = 0;
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

    uint32_t rescode = 0;
    if (len < 4)
    {
        errmsg("bad response");
        return -1;
    }

    // do something
    memcpy(&rescode, &rbuf[4], 4);
    printf("Server says: [%u], %.*s\n", rescode, len - 4, &rbuf[4 + 4]);
    return 0;
}