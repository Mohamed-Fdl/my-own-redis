#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#define PORT 1234
#define HOST_ADRESS 0
#define YES 1
#define NO 0

static void die(char *msg);
static void errmsg(char *msg);
static void processing(int connfd);

int main(void)
{
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
        // accepting new connection in a loop
        struct sockaddr_in client_addr = {};
        socklen_t client_addr_len = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &client_addr_len);
        printf("[NEW CONNECTION ACCEPTED]: %d\n", connfd);

        if (connfd < 0)
        {
            // failure on accepting connection
            continue;
        }

        processing(connfd);
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