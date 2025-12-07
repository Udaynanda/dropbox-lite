# Distributed File Synchronization System

A high-performance file synchronization system written in C++20, implementing content-defined chunking, delta sync, and deduplication. Think Dropbox, but built from scratch to understand how distributed storage systems work.

## Overview

This project implements the core algorithms used by systems like Dropbox, Google Drive, and rsync:
- **FastCDC chunking** for efficient file splitting
- **Delta synchronization** to minimize bandwidth usage
- **Content-addressable storage** for deduplication
- **gRPC** for client-server communication

## Performance

Benchmarked on Apple M3 Pro:
- Chunking throughput: **280 MB/s**
- SHA256 hashing: **1,923 MB/s**
- Deduplication: **76% storage savings** for similar files
- Delta sync: **50-75% bandwidth reduction** for typical edits

See [BENCHMARKS.md](BENCHMARKS.md) for detailed measurements.

## Features

- **Content-Defined Chunking**: Uses FastCDC algorithm with Rabin-Karp rolling hash
- **Delta Sync**: Only transfers changed chunks, not entire files
- **Deduplication**: Identical chunks stored once, referenced by multiple files
- **Real-time Monitoring**: Platform-specific file watchers (inotify on Linux, FSEvents on macOS)
- **Conflict Resolution**: Handles concurrent edits with configurable strategies
- **Multi-threaded**: Parallel chunk uploads with thread pool

## Architecture

```
Client                          Server
  │                               │
  ├─ FileWatcher ────────────────┤
  ├─ SyncEngine                  ├─ SyncService
  ├─ DeltaEngine                 ├─ StorageManager
  └─ MetadataDB                  └─ MetadataDB
       │                               │
       └──────── gRPC/HTTP2 ──────────┘
```

**Client Components:**
- `FileWatcher`: Monitors filesystem changes
- `SyncEngine`: Orchestrates synchronization
- `DeltaEngine`: Computes file deltas
- `MetadataDB`: SQLite database for local state

**Server Components:**
- `SyncService`: gRPC service implementation
- `StorageManager`: Content-addressable chunk storage
- `MetadataDB`: Tracks file versions and chunks

**Common:**
- `Chunker`: FastCDC implementation
- `Hash`: SHA256 and rolling hash
- `ThreadPool`: Concurrent operations
- `Compression`: zlib compression

## Quick Start

### Prerequisites

```bash
# macOS
brew install cmake grpc protobuf sqlite openssl zlib

# Ubuntu/Debian
sudo apt-get install cmake build-essential libgrpc++-dev \
    libprotobuf-dev protobuf-compiler-grpc libsqlite3-dev \
    libssl-dev zlib1g-dev
```

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Run Benchmarks

```bash
cd build/benchmarks
./bench_chunking    # Test chunking performance
./bench_dedup       # Test deduplication
./bench_delta_sync  # Test delta sync efficiency
```

### Run Server & Client

```bash
# Terminal 1: Start server
./build/dropbox_server_app ./storage 50051

# Terminal 2: Start client
./build/dropbox_client_app ./sync_folder localhost:50051 client1
```

## How It Works

### Content-Defined Chunking

Traditional fixed-size chunking breaks when you insert data at the beginning of a file - all subsequent chunks change. FastCDC uses a rolling hash to find chunk boundaries based on content, not position.

```cpp
// Simplified chunking logic
for each byte in file:
    rolling_hash.update(byte)
    if rolling_hash matches pattern:
        create_chunk()
```

This means inserting a paragraph at the start of a document only changes 1-2 chunks, not the entire file.

### Delta Synchronization

1. Client chunks local file and computes SHA256 hashes
2. Client sends hashes to server
3. Server identifies which chunks it already has
4. Client uploads only the missing chunks
5. Server reconstructs file from chunks

For a 10 MB file with 5% changes, this transfers ~2.6 MB instead of 10 MB.

### Deduplication

Chunks are stored using their SHA256 hash as the filename:
```
chunks/
  ab/abcdef123456...
  cd/cdef789012...
```

If two files have identical chunks, the chunk is stored once and referenced twice. This saves ~76% storage for similar files (like document versions).

## Technical Details

**Algorithms:**
- FastCDC for content-defined chunking
- Rabin-Karp rolling hash (O(1) updates)
- SHA256 for integrity verification
- Token bucket for rate limiting

**Concurrency:**
- Thread pool for parallel operations
- Lock-free metrics collection
- RAII-based transaction management

**Network:**
- gRPC with bidirectional streaming
- Protocol Buffers for serialization
- HTTP/2 multiplexing

**Storage:**
- SQLite for metadata (indexed queries)
- Content-addressable chunk storage
- Atomic file operations

## Project Structure

```
dropbox-lite/
├── proto/              # Protocol buffer definitions
├── include/            # Header files
│   ├── common/         # Hash, chunker, compression
│   ├── core/           # Metadata, delta engine
│   ├── client/         # File watcher, sync engine
│   └── server/         # Storage, sync service
├── src/                # Implementation
├── benchmarks/         # Performance tests
├── tests/              # Unit tests
└── CMakeLists.txt
```

## Limitations

This is a proof-of-concept demonstrating distributed systems algorithms. Production use would require:

- **Security**: TLS encryption, authentication, authorization
- **Testing**: Comprehensive integration and chaos testing
- **Monitoring**: Metrics, logging, distributed tracing
- **Scalability**: Horizontal scaling, load balancing
- **Reliability**: Retry logic, circuit breakers, health checks

## Why I Built This

I wanted to understand how systems like Dropbox work under the hood. This project taught me:
- How content-defined chunking enables efficient delta sync
- Trade-offs between consistency and availability in distributed systems
- Performance optimization techniques (profiling, benchmarking)
- Systems programming in modern C++

## License

MIT License - see [LICENSE](LICENSE) file.

## Acknowledgments

- FastCDC algorithm from "FastCDC: a Fast and Efficient Content-Defined Chunking Approach for Data Deduplication"
- Inspired by Dropbox, rsync, and Git's delta compression
