# Build my own Redis step by step
My own implementation of the an in-memory database like Redis.Both server and client are written in cpp.

## Motivation
- learn new things
- master the fundamentals of networking when it comes to build software
- make a projects that is more than a basic CRUD app

## Quick Start
Make sure you have the g++ compiler

![Demo Server Gif](/demo-server.gif)
![Demo Client Gif](/demo-client.gif)

```bash
# get project
git clone https://github.com/Mohamed-Fdl/my-own-redis.git
cd cd my-own-redis

# build the client and server
g++ -Wall -Wextra -O2 -g server.cpp -g hashtable.cpp -o server.out
g++ -Wall -Wextra -O2 -g client.cpp -o client.out

# Run the server
server.out

# Client basics commands
client.out set x y

```

## Usage

```bash
# Run the server
server.out

# setting value
client.out set x y

# getting value (outputs y)
client.out get x

# deleting value
client.out del x
```

## 🤝Contributing

### Clone the repo

```bash
git clone https://github.com/Mohamed-Fdl/my-own-redis.git
cd my-own-redis
```

### Submit a pull request

If you'd like to contribute, please fork the repository and open a pull request to the `main` branch.

