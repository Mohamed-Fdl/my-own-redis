#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#define SERVER_PORT 1234
#define SERVER_HOST_ADRESS INADDR_LOOPBACK
#define YES 1
#define NO 0

static void die(char *msg);
static void errmsg(char *msg);

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

    char msg[] = "Hello,";
    write(fd, msg, strlen(msg));

    char rbuf[64];

    ssize_t n = read(fd, rbuf, sizeof(rbuf));

    if (n < 0)
    {
        die("read");
    }

    printf("Server says: %s\n", rbuf);
    close(fd);
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
