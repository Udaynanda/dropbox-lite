# Dropbox Lite - Distributed File Synchronization System

A high-performance distributed file synchronization system written in C++20, implementing core Dropbox/Google Drive functionality with content-defined chunking, delta sync, and deduplication.

[![C++](https://img.shields.io/badge/C++-20-blue.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

## 🎯 Project Overview

This project demonstrates production-grade distributed systems engineering by implementing a complete file synchronization system from scratch. It showcases algorithms used by real-world systems like Dropbox, Git, and rsync.

**Key Achievement**: Benchmarked on Apple M3 Pro achieving **270 MB/s chunking throughput**, **76% deduplication**, and **50-75% bandwidth reduction** for typical file edits.

## ✨ Features

### Core Functionality
- **Content-Defined Chunking (FastCDC)**: Variable-size chunks using Rabin-Karp rolling hash
- **Delta Synchronization**: Only transfer changed chunks (rsync-like efficiency)
- **Deduplication**: Content-addressable storage eliminates duplicate chunks
- **Conflict Resolution**: Automatic and manual strategies (last-write-wins, keep-both)
- **Real-time File Monitoring**: Platform-specific watchers (inotify/FSEvents)
- **Multi-threaded Transfers**: Concurrent uploads with thread pool

### Technical Implementation
- **gRPC Communication**: Bidirectional streaming with Protocol Buffers
- **SHA256 Hashing**: Cryptographic integrity verification
- **SQLite Metadata**: Efficient local state tracking with indexed queries
- **Compression**: zlib compression for bandwidth optimization
- **Rate Limiting**: Token bucket algorithm for bandwidth control

## 📊 Performance (Benchmarked on M3 Pro)

| Metric | Result |
|--------|--------|
| Chunking Throughput | 270 MB/s |
| SHA256 Hashing | 1,923 MB/s |
| Deduplication Savings | 76% (similar files) |
| Delta Sync Reduction | 50-75% (typical edits) |

See [BENCHMARKS.md](BENCHMARKS.md) for detailed performance analysis.

## 🏗️ Architecture

```
┌─────────────┐         gRPC          ┌─────────────┐
│   Client    │◄─────────────────────►│   Server    │
│             │                        │             │
│ FileWatcher │                        │  Storage    │
│ SyncEngine  │                        │  Manager    │
│ DeltaEngine │                        │             │
│ MetadataDB  │                        │ MetadataDB  │
└─────────────┘                        └─────────────┘
```

### Key Components

**Client Side:**
- `FileWatcher`: OS-specific file monitoring (inotify/FSEvents)
- `SyncEngine`: Orchestrates synchronization logic
- `DeltaEngine`: Computes file deltas for efficient transfers
- `UploadManager`: Multi-threaded upload queue
- `MetadataDB`: SQLite database for local file metadata

**Server Side:**
- `SyncService`: gRPC service implementation
- `StorageManager`: Content-addressable chunk storage
- `ConflictResolver`: Handles file conflicts

**Common:**
- `Chunker`: FastCDC content-defined chunking
- `Hash`: SHA256 and rolling hash implementations
- `Compression`: zlib compression/decompression
- `ThreadPool`: High-performance work queue
- `Metrics`: Performance monitoring

## 🚀 Quick Start

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
./bench_chunking    # Chunking performance
./bench_dedup       # Deduplication effectiveness
./bench_delta_sync  # Delta sync bandwidth reduction
```

### Run Server & Client

```bash
# Terminal 1: Start server
./build/dropbox_server_app ./storage 50051

# Terminal 2: Start client
./build/dropbox_client_app ./sync_folder localhost:50051 client1
```

## 🔬 Technical Deep Dive

### Content-Defined Chunking (FastCDC)

Uses Rabin-Karp rolling hash with adaptive masking for optimal chunk distribution:

```cpp
// Adaptive masking based on chunk size
if (chunk_size < normalized_size) {
    is_boundary = (hash & (kMask >> 1)) == 0;  // More aggressive
} else {
    is_boundary = (hash & kMask) == 0;         // Normal
}
```

**Why FastCDC?**
- Better chunk size distribution than traditional CDC
- Resilient to insertions/deletions (chunks don't shift)
- ~2x faster than traditional content-defined chunking

**Configuration:**
- Min chunk: 4 KB
- Avg chunk: 64 KB
- Max chunk: 1 MB

### Delta Synchronization

1. Client chunks local file → generates SHA256 hashes
2. Client sends hashes to server
3. Server identifies missing chunks
4. Client uploads only new chunks
5. Server reconstructs file from chunks

**Bandwidth Savings**: 50-75% for typical document edits

### Deduplication

Content-addressable storage using SHA256:
```
chunks/
  ab/abcdef123456...  (SHA256 hash as filename)
  cd/cdef789012...
```

Multiple files can reference the same chunk, achieving **76% storage savings** for similar files.

### Conflict Resolution

**Detection:**
```cpp
bool hasConflict = 
    local.hash != remote.hash &&
    local.version > 0 &&
    remote.version > 0;
```

**Strategies:**
1. **Last-Write-Wins**: Compare timestamps
2. **Keep Both**: Create conflict copy
3. **Manual**: User decides

## 📁 Project Structure

```
dropbox-lite/
├── proto/              # Protocol buffer definitions
├── include/            # Public headers
│   ├── common/         # Hash, chunker, compression, logger
│   ├── core/           # Metadata DB, delta engine, conflict resolver
│   ├── client/         # File watcher, sync engine, upload manager
│   └── server/         # Storage manager, sync service
├── src/                # Implementation files
├── benchmarks/         # Performance benchmarks
├── tests/              # Unit tests
└── CMakeLists.txt
```

## 🎓 What This Demonstrates

### Distributed Systems
- Eventual consistency model
- Conflict detection and resolution
- Version vectors for state management
- Heartbeat-based connection monitoring

### Algorithms & Data Structures
- FastCDC content-defined chunking
- Rabin-Karp rolling hash (O(1) updates)
- Content-addressable storage
- Token bucket rate limiting

### Systems Programming
- Modern C++20 (RAII, smart pointers, move semantics)
- Platform-specific APIs (inotify/FSEvents)
- Multi-threading and concurrency
- Zero-copy I/O where possible

### Performance Engineering
- Benchmarking and profiling
- Lock-free data structures
- Indexed database queries
- Streaming I/O (no full file in memory)

## 🔐 Security Considerations

**Current Implementation:**
- ✅ SHA256 integrity verification
- ✅ Chunk-level verification
- ❌ No encryption at rest
- ❌ No encryption in transit (insecure gRPC)
- ❌ No authentication/authorization

**Production Requirements:**
- Add TLS/SSL for gRPC
- Implement OAuth2/JWT authentication
- Add AES-256 encryption at rest
- Implement rate limiting per user
- Add audit logging

## 🧪 Testing

```bash
# Run unit tests (requires GoogleTest)
cd build
ctest --output-on-failure

# Run benchmarks
cd benchmarks
./bench_chunking
./bench_dedup
./bench_delta_sync
```

## 📈 Performance Characteristics

### Time Complexity
| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Chunk file | O(n) | n = file size |
| Hash chunk | O(n) | n = chunk size |
| Find chunk | O(1) | Hash table lookup |
| Sync query | O(log m) | m = file count, indexed |

### Space Complexity
| Component | Complexity | Notes |
|-----------|-----------|-------|
| Metadata | O(n) | n = file count |
| Chunk index | O(m) | m = unique chunks |
| Event queue | O(k) | k = pending events |

## 🚧 Future Enhancements

- [ ] End-to-end encryption
- [ ] Selective sync (ignore patterns)
- [ ] Bandwidth throttling UI
- [ ] File history/snapshots
- [ ] Shared folders
- [ ] Web interface
- [ ] Mobile clients

## 📝 Resume-Ready Description

> **Distributed File Synchronization System (C++)**
> 
> Engineered a distributed file sync system implementing core Dropbox functionality with FastCDC content-defined chunking, delta synchronization, and deduplication. Benchmarked on M3 Pro achieving 270 MB/s chunking throughput, 76% deduplication for similar files, and 50-75% bandwidth reduction for typical edits.
> 
> **Technical Stack**: C++20, gRPC, Protocol Buffers, SQLite, OpenSSL, zlib
> 
> **Key Achievements**:
> - Implemented FastCDC algorithm from research papers with Rabin-Karp rolling hash
> - Built content-addressable storage with cross-file deduplication
> - Designed eventual consistency model with conflict resolution
> - Optimized for performance with multi-threading and zero-copy I/O
> - Comprehensive benchmarking and performance analysis

## 📄 License

MIT License - feel free to use this in your portfolio!

## 🤝 Contributing

This is a portfolio project demonstrating distributed systems expertise. Suggestions and improvements are welcome!

## 📧 Contact

Built to demonstrate systems engineering skills for infrastructure and distributed systems roles.

---

**Note**: This is a proof-of-concept demonstrating distributed systems algorithms. Production deployment would require security hardening, comprehensive testing, and operational tooling.
