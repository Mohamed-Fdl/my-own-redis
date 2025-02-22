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
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <vector>
#include <netinet/ip.h>
#include <map>
#include <string>

#include "hashtable.h"

#define YES 1
#define NO 0
#define K_MAX_MSG 4096
#define POLL_TIMEOUT 1000
#define PORT 1234
#define HOST_ADDRESS 0
#define K_MAX_ARGS 1024

#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type, member) ); })

enum
{
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};

enum
{
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

struct Conn
{
    int fd = -1;
    uint32_t state = 0;

    size_t rbuf_size = 0;
    uint8_t rbuf[4 + K_MAX_MSG];

    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + K_MAX_MSG];
};

static struct
{
    HMap db;
} g_data;

struct Entry
{
    struct HNode node;
    std::string key;
    std::string val;
};


static void die(const char *msg);
static void errmsg(const char *msg);
static void fd_set_nb(int fd);
static void conn_put(std::vector<Conn *> &fd2conn, struct Conn *conn);
static int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd);
static void connection_io(struct Conn *conn);
static void state_res(struct Conn *conn);
static void state_req(struct Conn *conn);
static bool try_one_request(struct Conn *conn);
static bool try_fill_buffer(struct Conn *conn);
static bool try_flush_buffer(struct Conn *conn);
static uint32_t do_del(std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen);
static uint32_t do_set(std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen);
static uint32_t do_get(std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen);
static int32_t parse_req(const uint8_t *data, size_t len, std::vector<std::string> &cmd);
static int32_t do_request(const uint8_t *req, uint32_t reqlen, uint32_t *rescode, uint8_t *res, uint32_t *reslen);
static bool cmd_is(const std::string &word, const char *cmd);
static bool entry_eq(HNode *lhs, HNode *rhs);
static uint64_t str_hash(const uint8_t *data, size_t len);

int main()
{
    // socket file descriptor created: ipv4, tcp
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        die("socket()");
    }

    // set socket option SO_REUSEADDR to 1
    int value = YES;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

    // bind to adress 0.0.0.0:1234
    struct sockaddr_in addr = {};

    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(PORT);
    addr.sin_addr.s_addr = ntohl(HOST_ADDRESS);

    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));

    if (rv)
    {
        die("bind()");
    }

    // listening to perform handshakes
    rv = listen(fd, SOMAXCONN);
    if (rv)
    {
        die("listen()");
    }

    // map of all con* keyed by their file descriptors
    std::vector<Conn *> fd2conn;

    // set the fd non blocking
    fd_set_nb(fd);

    // list of all file descriptors for the poll call
    std::vector<struct pollfd> poll_args;

    while (true)
    {
        poll_args.clear();

        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);

        for (Conn *conn : fd2conn)
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

        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), POLL_TIMEOUT);
        if (rv < 0)
        {
            die("poll()");
        }

        for (size_t i = 1; i < poll_args.size(); ++i)
        {
            if (poll_args[i].revents)
            {
                Conn *conn = fd2conn[poll_args[i].fd];
                connection_io(conn);
                if (conn->state == STATE_END)
                {
                    fd2conn[conn->fd] = NULL;
                    (void)close(conn->fd);
                    free(conn);
                }
            }
        }

        if (poll_args[0].revents)
        {
            (void)accept_new_conn(fd2conn, fd);
        }
    }
    return 0;
}

static void errmsg(const char *msg)
{
    fprintf(stderr, "[ERROR]: %s\n", msg);
}

static void die(const char *msg)
{
    int err = errno;
    fprintf(stderr, "[ERROR]: [%d] %s\n", err, msg);
    abort();
}

static void fd_set_nb(int fd)
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

static void conn_put(std::vector<Conn *> &fd2conn, struct Conn *conn)
{
    if (fd2conn.size() <= (size_t)conn->fd)
    {
        fd2conn.resize(conn->fd + 1);
    }

    fd2conn[conn->fd] = conn;
}

static int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd)
{
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0)
    {
        errmsg("error accept()");
        return -1;
    }

    fd_set_nb(connfd);

    struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));
    if (!conn)
    {
        close(connfd);
        return -1;
    }

    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;

    conn_put(fd2conn, conn);
    return 0;
}

static void connection_io(struct Conn *conn)
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

static void state_req(struct Conn *conn)
{
    while (try_fill_buffer(conn))
    {
    }
}

static void state_res(struct Conn *conn)
{
    while (try_flush_buffer(conn))
    {
    }
}

static bool try_fill_buffer(struct Conn *conn)
{
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;

    do
    {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (rv < 0 && errno == EINTR);

    if (rv < 0 && errno == EAGAIN)
    {
        return false;
    }

    if (rv < 0)
    {
        errmsg("error read()");
        conn->state = STATE_END;
        return false;
    }

    if (rv == 0)
    {
        if (conn->rbuf_size > 0)
        {
            errmsg("unexpected EOF");
        }
        else
        {
            errmsg("EOF");
        }
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf));

    // try to process one request
    while (try_one_request(conn))
    {
    }

    return (conn->state == STATE_REQ);
}

static bool try_flush_buffer(struct Conn *conn)
{
    ssize_t rv = 0;

    do
    {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (rv < 0 && errno == EAGAIN);

    if (rv < 0 && errno == EAGAIN)
    {
        return false;
    }

    if (rv < 0)
    {
        errmsg("write() error");
        conn->state = STATE_END;
        return false;
    }

    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);

    if (conn->wbuf_sent == conn->wbuf_size)
    {
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }

    return false;
}

static bool try_one_request(struct Conn *conn)
{
    if (conn->rbuf_size < 4)
    {
        return false;
    }

    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);

    if (len > K_MAX_MSG)
    {
        errmsg("too long");
        conn->state = STATE_END;
        return false;
    }

    if (4 + len > conn->rbuf_size)
    {
        return false;
    }

    // &conn->wbuf[4 + 4] because 4 bit for response length & 4 bit for rescode
    // wlen is the response length
    uint32_t rescode = 0;
    uint32_t wlen = 0;
    int32_t err = do_request(&conn->rbuf[4], len, &rescode, &conn->wbuf[4 + 4], &wlen);

    if (err)
    {
        conn->state = STATE_END;
        return false;
    }

    // generating response
    wlen += 4;
    memcpy(&conn->wbuf[0], &wlen, 4);
    memcpy(&conn->wbuf[4], &rescode, 4);
    conn->wbuf_size = 4 + wlen;

    // remove the request & move the remain to the begining of the receive buffer
    size_t remain = conn->rbuf_size - (4 + len);
    if (remain)
    {
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain;

    // change connection state to response & process to send response
    conn->state = STATE_RES;
    state_res(conn);

    return (conn->state == STATE_REQ);
}

static bool cmd_is(const std::string &word, const char *cmd)
{
    return 0 == strcasecmp(word.c_str(), cmd);
}

static int32_t do_request(const uint8_t *req, uint32_t reqlen, uint32_t *rescode, uint8_t *res, uint32_t *reslen)
{
    std::vector<std::string> cmd;
    if (0 != parse_req(req, reqlen, cmd))
    {
        errmsg("bad req");
        return -1;
    }

    if (cmd.size() == 2 && cmd_is(cmd[0], "get"))
    {
        *rescode = do_get(cmd, res, reslen);
    }
    else if (cmd.size() == 3 && cmd_is(cmd[0], "set"))
    {
        *rescode = do_set(cmd, res, reslen);
    }
    else if (cmd.size() == 2 && cmd_is(cmd[0], "del"))
    {
        *rescode = do_del(cmd, res, reslen);
    }
    else
    {
        *rescode = RES_NX;
        const char *msg = "Unknown command";
        strcpy((char *)res, msg);
        *reslen = strlen(msg);
        return 0;
    }

    return 0;
}

static int32_t parse_req(const uint8_t *data, size_t len, std::vector<std::string> &cmd)
{
    if (len < 4)
    {
        return -1;
    }

    uint32_t n = 0;
    memcpy(&n, &data[0], 4);
    if (n > K_MAX_ARGS)
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

        cmd.push_back(std::string((char *)&data[pos + 4], sz));
        pos += 4 + sz;
    }

    if (pos != len)
    {
        return -1;
    }

    return 0;
}

static uint32_t do_get(std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen)
{
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if (!node)
    {
        return RES_NX;
    }

    const std::string &val = container_of(node, Entry, node)->val;
    assert(val.size() <= K_MAX_MSG);
    memcpy(res, val.data(), val.size());
    *reslen = (uint32_t)val.size();
    return RES_OK;
}

static uint32_t do_set(std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen)
{
    (void)res;
    (void)reslen;

    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if (node)
    {
        container_of(node, Entry, node)->val.swap(cmd[2]);
    }
    else
    {
        Entry *ent = new Entry();
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        ent->val.swap(cmd[2]);
        hm_insert(&g_data.db, &ent->node);
    }

    return RES_OK;
}

static uint32_t do_del(std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen)
{
    (void)res;
    (void)reslen;

    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_pop(&g_data.db, &key.node, &entry_eq);
    if (node)
    {
        delete container_of(node, Entry, node);
    }

    return RES_OK;
}

static bool entry_eq(HNode *lhs, HNode *rhs)
{
    struct Entry *le = container_of(lhs, Entry, node);
    struct Entry *re = container_of(rhs, Entry, node);
    return le->key == re->key;
}

static uint64_t str_hash(const uint8_t *data, size_t len)
{
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i < len; i++)
    {
        h = (h + data[i]) * 0x01000193;
    }

    return h;
}
