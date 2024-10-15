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