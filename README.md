# Source

- Beej's Network Programming: https://beej.us/guide/bgnet/html/

# Notes

## Sockets

- socket is a way programs can communicate by using file descriptors (fd)
- fd are integers representing an opened file
- we can set up file descriptors using the socket() system call
- we send infos with send() and recv()
- 2 types of internet sockets: "Stream Sockets" and "Datagram Sockets"
- 2 types of internet sockets: SOCK_STREAM and SOCK_DGRAM
- stream sockets use TCP and datagram sockets use UDP
- UDP opposite to TCP is unreliable

## IP Adresses

- IPV4, 4 bytes, 0 to 255, 192.234.1.76
- we quickly ran about of thems but remedy this by using NAT (Network Adress Translation)
- so IPV6, 16 bytes was born and offer 79 MILLION BILLION TRILLION
- ipv6 loopback adresse is "::1"

### PORT Numbers

- 16 bit number as local adress of the connection
- if IP adresses are adresses for a home, the port number is like the number of a room in thos house
- ports number under 1024 are considered special and require OS permission to be use

## Structs

- a socket file descriptor is just an integer
- the addrinfo have the following structure

```
struct addrinfo {
    int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
    int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
    int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM
    int              ai_protocol;  // use 0 for "any"
    size_t           ai_addrlen;   // size of ai_addr in bytes
    struct sockaddr *ai_addr;      // struct sockaddr_in or _in6
    char            *ai_canonname; // full canonical hostname

    struct addrinfo *ai_next;      // linked list, next node
};

```

- the socket adress info for ipv4 looks like that

```
struct sockaddr_in {
    short int          sin_family;  // Address family, AF_INET
    unsigned short int sin_port;    // Port number
    struct in_addr     sin_addr;    // Internet address
    unsigned char      sin_zero[8]; // Same size as struct sockaddr
};

struct in_addr {
    uint32_t s_addr; // that's a 32-bit int (4 bytes)
};

```

- the socket adress info for ipv6 looks like that

```
struct sockaddr_in6 {
    u_int16_t       sin6_family;   // address family, AF_INET6
    u_int16_t       sin6_port;     // port number, Network Byte Order
    u_int32_t       sin6_flowinfo; // IPv6 flow information
    struct in6_addr sin6_addr;     // IPv6 address
    u_int32_t       sin6_scope_id; // Scope ID
};

struct in6_addr {
    unsigned char   s6_addr[16];   // IPv6 address
};

```
