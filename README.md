# SlenKV - High-Performance Key-Value Store

A lightweight, high-performance key-value storage system written in C, featuring custom network protocol, multi-threading architecture, and excellent performance metrics.

## 🚀 Features

- **High Performance**: 100K+ operations per second
- **Custom Protocol**: Length-prefixed binary protocol for efficient data transfer
- **Multi-threaded**: Separated network I/O, command processing, and response handling
- **Zero-copy Parsing**: Optimized command parsing with minimal memory allocation
- **TTL Support**: Built-in key expiration mechanism
- **Client SDK**: Complete C client library with connection pooling
- **Memory Efficient**: Custom hash table with load balancing and rehashing

## 📊 Performance

```
Benchmark Results:
- Operations: 100,000 SET commands
- Throughput: 107,918 ops/sec
- Data Transfer: 10.29 MB/sec  
- Latency: <1ms average
```

## 🏗️ Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   Client SDK    │───▶│  Network Server   │───▶│  Command Parser │
│                 │    │  (epoll-based)   │    │                 │
└─────────────────┘    └──────────────────┘    └─────────────────┘
                                │                        │
                                ▼                        ▼
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│ Response Thread │◀───│  Event Queue     │    │   Hash Table    │
│                 │    │  (Thread-safe)   │    │   Storage       │
└─────────────────┘    └──────────────────┘    └─────────────────┘
```

## 🛠️ Build & Installation

### Prerequisites
- GCC 4.8+ or Clang 3.3+
- POSIX-compliant system (Linux/macOS)
- pthread library

### Build
```bash
git clone https://github.com/yourusername/slenkv.git
cd slenkv
make
```

### Run Server
```bash
./slenkv --port 7000 --max-connections 10000
```

## 📝 Protocol

SlenKV uses a simple length-prefixed protocol:

```
<length>:<data>\n

Example:
9:SET k1 v1
7:GET k1
7:DEL k1
```

## 💻 Usage

### Server Commands
- `SET key value [expire_ms]` - Set key-value pair with optional TTL
- `GET key` - Retrieve value by key
- `DEL key` - Delete key-value pair
- `EXPIRE key time_ms` - Set TTL for existing key

### Client SDK Example
```c
#include "kvclient.h"

int main() {
    // Create client connection
    KVClient *client = kv_client_create("127.0.0.1", 7000, 5000, 0);
    if (!client) {
        fprintf(stderr, "Failed to connect\n");
        return -1;
    }
    
    // Set key-value
    if (kv_set(client, "hello", 5, "world", 5) > 0) {
        if (kv_send(client) > 0) {
            printf("SET success\n");
        }
    }
    
    // Get value
    if (kv_get(client, "hello", 5) > 0) {
        if (kv_send(client) > 0) {
            printf("Response: %s\n", client->buffer + client->read_offset);
        }
    }
    
    kv_client_destroy(&client);
    return 0;
}
```

### Benchmark Tool
```bash
# Run performance test
./benchmark --host 127.0.0.1 --port 7000 --threads 3 --operations 100000
```

## 🔧 Configuration

Server supports the following command-line options:
- `--port <port>` - Server port (default: 7000)
- `--backlog <num>` - Connection backlog (default: 1024)
- `--max-connections <num>` - Maximum concurrent connections (default: 100000)

## 📈 Optimization Features

- **Zero-copy Command Parsing**: Minimizes memory allocation during request processing
- **Event-driven I/O**: Uses Linux epoll for high-concurrency network handling
- **Lock-free Data Structures**: Optimized for multi-threaded access patterns
- **Connection Pooling**: Client SDK supports connection reuse and pooling
- **Memory Pool**: Pre-allocated buffers reduce malloc/free overhead

## 🧪 Testing

Run the test suite:
```bash
make test
./run_tests.sh
```

Performance profiling with Valgrind:
```bash
valgrind --tool=callgrind ./slenkv
```

## 🤝 Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📋 Roadmap

- [ ] **Persistence**: Add AOF and RDB persistence options
- [ ] **Clustering**: Master-slave replication and sharding
- [ ] **Data Types**: Support for Lists, Sets, and Hash maps
- [ ] **Lua Scripting**: Server-side script execution
- [ ] **Monitoring**: Built-in metrics and health endpoints
- [ ] **Configuration**: Runtime configuration management

## 🐛 Known Issues

- Single-threaded command processing (planned for v2.0)
- Limited to string data types currently
- No built-in authentication mechanism

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🏆 Acknowledgments

- Inspired by Redis architecture and protocol design
- Uses optimized hash table implementation based on DJB2 algorithm
- Network layer leverages Linux epoll for high performance


**Star ⭐ this repository if you find it useful!**