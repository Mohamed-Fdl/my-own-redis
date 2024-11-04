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
