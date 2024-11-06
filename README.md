# Build my own Redis step by step

## Client-Server Background

```mermaid
  sequenceDiagram
    Client->>Server: Request
    alt
    Server->>Client: Response
    end
```

## TCP/IP Model

```
top
  /\    | App |     message or whatever
  ||    | TCP |     byte stream
  ||    | IP  |     packets
  ||    | ... |
bottom
```

- ip packet = sender's adress + receiver's adress + data
- tcp listen on a particular adress and port

## Protocol Parsing

- we need to implement a sort of protocol to let client & server agree with it
- example

```
+-----+------+-----+------+--------
| len | msg1 | len | msg2 | more...
+-----+------+-----+------+--------
```

- the protocol have the following parts: a 4-byte little-endian integer indicating the length of the request, and the variable-length request
- TCP read/write syscalls can return less than number of bytes you specified to read or write
