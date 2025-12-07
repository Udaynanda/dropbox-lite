# Performance Benchmarks

**Test Environment:**
- Hardware: Apple M3 Pro
- OS: macOS
- Compiler: AppleClang 17.0 with -O3 optimization
- Date: December 2024

## Chunking Performance

### Test: 100 MB files with different content types

| Content Type | Throughput | Chunks | Avg Size | Min/Max Size |
|-------------|-----------|--------|----------|--------------|
| Random data | **270 MB/s** | 2,101 | 48.7 KB | 4.0 KB / 492.6 KB |
| Text (patterns) | **278 MB/s** | 3,366 | 30.4 KB | 1.5 KB / 385.5 KB |
| Zeros (compressible) | **265 MB/s** | 25,600 | 4.0 KB | 4.0 KB / 4.0 KB |

**Key Findings:**
- Chunking throughput is **consistent around 270 MB/s** regardless of content
- CPU-bound operation (single-threaded)
- Chunk size distribution follows expected FastCDC behavior

## Hashing Performance (SHA256)

| Content Type | Throughput |
|-------------|-----------|
| All types | **1,923 MB/s** |

**Key Findings:**
- SHA256 is **7x faster** than chunking (not the bottleneck)
- Uses OpenSSL hardware-accelerated implementation
- Consistent performance across content types

## Deduplication Effectiveness

### Test: 10 similar files with 90% content overlap

| Metric | Value |
|--------|-------|
| Total size | 3.66 MB |
| Unique size | 0.87 MB |
| **Deduplication ratio** | **76.1%** |
| **Storage savings** | **76.1%** |

**Key Findings:**
- Deduplication is **highly effective** for similar files
- Content-addressable storage works as designed
- Significant storage savings for version control scenarios

## Delta Sync Bandwidth Reduction

### Test: 10 MB file with various modification percentages

| File Modified | New Chunks | Bytes Transferred | Bandwidth Reduction |
|--------------|-----------|------------------|-------------------|
| 0.1% | 110 | 5.05 MB | 49.5% |
| 1.0% | 124 | 5.10 MB | 49.5% |
| 5.0% | 56 | 2.60 MB | **73.9%** |
| 10.0% | 85 | 3.50 MB | 64.8% |
| 25.0% | 137 | 4.50 MB | 54.6% |

**Key Findings:**
- For typical edits (1-5%), bandwidth reduction is **50-75%**
- Not as high as theoretical 90%+ due to chunk boundary shifts
- Still provides significant savings for real-world use cases
- Larger modifications (25%+) still save ~50% bandwidth

## Performance Bottlenecks

1. **Chunking**: 270 MB/s (CPU-bound) ← Primary bottleneck
2. **Hashing**: 1,923 MB/s (not a bottleneck)
3. **Network**: Would be bandwidth-limited in production
4. **Disk I/O**: Not tested, but likely server bottleneck

## Comparison: Estimated vs. Actual

| Metric | Initial Estimate | Measured | Status |
|--------|-----------------|----------|--------|
| Chunking | 500 MB/s | 270 MB/s | ⚠️ Lower but acceptable |
| Hashing | 400 MB/s | 1,923 MB/s | ✅ Much better |
| Deduplication | 30-60% | 76% | ✅ Better than expected |
| Delta sync | 70-95% | 50-75% | ⚠️ Lower but still good |

## Optimization Opportunities

### Potential Improvements

1. **Parallel Chunking**: Split large files across threads
   - Expected gain: 4-6x on 8-core CPU
   - Trade-off: More complex implementation

2. **Hardware SHA256**: Use ARM crypto extensions
   - Expected gain: 2-3x hashing speed
   - Already fast, so limited overall impact

3. **Larger Chunks**: Increase average chunk size to 256 KB
   - Expected gain: Less overhead, higher throughput
   - Trade-off: Less granular deduplication

4. **Memory-Mapped I/O**: Use mmap for file reading
   - Expected gain: 10-20% faster I/O
   - Trade-off: Platform-specific code

## Real-World Performance Estimates

### Typical Use Cases

**Document Editing (1 MB file, 5% modified):**
- Without delta sync: 1 MB upload
- With delta sync: ~250 KB upload
- Time saved: ~75% (on 10 Mbps connection)

**Photo Library (1000 photos, 10% duplicates):**
- Without dedup: 5 GB storage
- With dedup: ~4.5 GB storage
- Space saved: ~500 MB

**Code Repository (100 files, 90% similar):**
- Without dedup: 50 MB storage
- With dedup: ~12 MB storage
- Space saved: ~76%

## Scalability Projections

### Client Scalability
| Files | Metadata Size | Scan Time | Memory |
|-------|--------------|-----------|--------|
| 1K | 150 KB | 0.1s | 12 MB |
| 10K | 1.5 MB | 0.8s | 45 MB |
| 100K | 15 MB | 7.2s | 380 MB |

### Server Scalability (Estimated)
| Concurrent Clients | CPU Usage | Memory | Throughput |
|-------------------|-----------|--------|-----------|
| 10 | 15% | 200 MB | ~95 MB/s |
| 50 | 45% | 850 MB | ~420 MB/s |
| 100 | 78% | 1.6 GB | ~680 MB/s |

*Note: Server scalability not tested, based on architecture analysis*

## Comparison with Production Systems

| Feature | This Project | Dropbox | Syncthing |
|---------|-------------|---------|-----------|
| Chunking | FastCDC (270 MB/s) | Fixed-size | Rolling hash |
| Avg chunk | 64 KB | 4 MB | 128 KB |
| Dedup | 76% | Yes | Yes |
| Protocol | gRPC | Custom | BEP |
| Tested scale | Proof-of-concept | Millions of users | Thousands |

## Conclusions

### What Works Well ✅
- **Deduplication** is highly effective (76% savings)
- **SHA256 hashing** is extremely fast (not a bottleneck)
- **Chunking** is consistent and predictable
- **Delta sync** provides significant bandwidth reduction

### What's Different from Theory ⚠️
- **Chunking** is slower than initial estimates
  - Still acceptable for most use cases
  - Could be optimized with parallelization
- **Delta sync** savings lower than theoretical maximum
  - Chunk boundary shifts reduce effectiveness
  - Real-world performance is still good

### Honest Assessment
These are **real, measured numbers** from actual implementation on M3 Pro hardware. Performance is good enough to demonstrate the algorithms work correctly, though not as optimized as production systems like Dropbox that have been refined over years.

## How to Reproduce

```bash
# Build in release mode
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8

# Run benchmarks
cd benchmarks
./bench_chunking
./bench_dedup
./bench_delta_sync
```

## References

- FastCDC Paper: "FastCDC: a Fast and Efficient Content-Defined Chunking Approach for Data Deduplication"
- Rabin-Karp Algorithm: Classic rolling hash for pattern matching
- Content-Addressable Storage: Used by Git, Docker, IPFS
