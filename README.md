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
- we quickly ran about of thems but remediate by using NAT (Network Adress Translation)
- so IPV6 was born anf offer 79 MILLION BILLION TRILLION
