# ObserveFS - Filesystem Observability Extension for DuckDB

[![DuckDB Extension](https://img.shields.io/badge/DuckDB-Extension-blue.svg)](https://duckdb.org/community_extensions/extensions/observefs.html)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

## 🚀 What is ObserveFS?

ObserveFS is a powerful DuckDB extension that provides comprehensive **filesystem observability** for your data operations. It transparently wraps any filesystem (local, HTTP, S3, Hugging Face) with monitoring capabilities, giving you detailed insights into I/O performance, latency patterns, and usage metrics.

Whether you're optimizing data pipelines, debugging performance issues, or understanding access patterns, ObserveFS gives you the visibility you need.

## ✨ Key Features

### 📊 Comprehensive I/O Monitoring
- **Operation Tracking**: Monitor `READ`, `OPEN`, and `LIST` operations across all supported filesystems
- **Latency Measurement**: Precise millisecond-level latency tracking using RAII guards
- **Bucket-wise Analysis**: Separate metrics for different storage buckets/paths

### 📈 Advanced Statistical Analysis
- **Histogram Distribution**: Detailed latency distribution analysis with configurable buckets
- **Quantile Estimation**: P50, P75, P90, P95, and P99 latency percentiles
- **Adaptive Algorithms**: Switches between in-memory and P² quantile estimation for large datasets
- **Outlier Detection**: Automatic identification and tracking of performance outliers

### 🌐 Multi-Filesystem Support
- **HTTP/HTTPS**: Monitor web-based data access
- **Amazon S3**: Track cloud storage performance
- **Hugging Face**: Observe model and dataset access patterns
- **Local Files**: Analyze local filesystem operations
- **Extensible**: Easy to add support for new filesystem types

### 🔧 Developer-Friendly API
- **Zero Configuration**: Works out-of-the-box with existing DuckDB queries
- **Non-Intrusive**: Transparent wrapper that doesn't change your code
- **Thread-Safe**: Concurrent query support with mutex-protected metrics
- **SQL Integration**: Query metrics directly using SQL functions

## 🚀 Quick Start

### Installation

```sql
-- Install from DuckDB Community Extensions
INSTALL observefs FROM community;
LOAD observefs;
```

### Basic Usage

Once loaded, ObserveFS automatically monitors all filesystem operations. Run your normal DuckDB queries and collect metrics:

```sql
-- Your normal data operations (automatically monitored)
SELECT * FROM 'https://example.com/data.parquet';
SELECT * FROM 's3://my-bucket/dataset.csv';

-- View performance metrics
SELECT observefs_get_profile();

-- Clear metrics for a fresh start
SELECT observefs_clear();
```

## 📊 Understanding the Metrics

### Sample Output

```
Current filesystem: observability-HTTPFileSystem
Overall latency:

open operation histogram is Max latency = 245.000000 millisec
Min latency = 89.000000 millisec
Mean latency = 156.333333 millisec
Distribution latency [80.000000, 90.000000) millisec: 11.11 %
Distribution latency [240.000000, 250.000000) millisec: 22.22 %

open operation quantile is
P50 latency 134.000000 millisec
P75 latency 189.500000 millisec
P90 latency 234.100000 millisec
P95 latency 239.550000 millisec
P99 latency 244.110000 millisec

read operation histogram is Max latency = 1205.000000 millisec
Min latency = 23.000000 millisec
Mean latency = 299.750000 millisec

  Bucket: my-s3-bucket
  Latency:
read operation histogram is Max latency = 892.000000 millisec
Min latency = 145.000000 millisec
Mean latency = 518.500000 millisec
```

### Metrics Breakdown

- **Overall Latency**: Aggregated metrics across all operations and filesystems
- **Operation-Specific Stats**: Separate tracking for different I/O types
- **Bucket Analysis**: Per-bucket/path performance breakdown
- **Distribution Analysis**: Histogram showing latency ranges and frequencies
- **Quantile Analysis**: Percentile-based performance insights

## 🎯 Use Cases

### 🔍 Performance Debugging
```sql
-- Monitor a slow query
SELECT * FROM 's3://large-dataset/data.parquet' WHERE date > '2024-01-01';

-- Check if network or storage is the bottleneck
SELECT observefs_get_profile();
```

### 📊 Data Pipeline Optimization
```sql
-- Compare performance across different data sources
SELECT * FROM 'https://api.example.com/data.json';
SELECT * FROM 's3://cache-bucket/data.parquet';

-- Analyze which source is faster
SELECT observefs_get_profile();
```

### 🌐 Multi-Cloud Analysis
```sql
-- Test data access across regions/providers
SELECT * FROM 's3://us-west-bucket/dataset.csv';
SELECT * FROM 's3://eu-west-bucket/dataset.csv';

-- Compare cross-region latencies
SELECT observefs_get_profile();
```

### 🤖 ML Workflow Monitoring
```sql
-- Monitor Hugging Face model loading
SELECT * FROM 'hf://datasets/squad/train.parquet';

-- Track data loading performance
SELECT observefs_get_profile();
```

## 🔧 Advanced Configuration

### Latency Heuristics

ObserveFS uses intelligent defaults for different operation types:

- **OPEN operations**: 0-1000ms range, 100 buckets
- **READ operations**: 0-1000ms range, 100 buckets
- **LIST operations**: 0-3000ms range, 100 buckets

Values outside these ranges are tracked as outliers.

### Quantile Estimation

The extension automatically switches between algorithms based on data volume:
- **Small datasets**: In-memory exact quantiles
- **Large datasets**: P² algorithm for memory-efficient estimation

## 🛠️ Building from Source

```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/your-repo/duckdb-filesystem-observability.git

# Install dependencies (requires VCPKG)
export VCPKG_TOOLCHAIN_PATH=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
make

# Run tests
make test
```

## 🤝 Contributing

We welcome contributions! Areas for improvement:

- **New Filesystem Support**: Add observability for additional storage systems
- **Metric Types**: Extend beyond latency (throughput, error rates, etc.)
- **Visualization**: Integration with monitoring dashboards
- **Performance**: Optimize overhead for high-throughput scenarios

## 📚 Technical Architecture

ObserveFS uses a **wrapper pattern** that:

1. **Intercepts** all filesystem operations via DuckDB's FileSystem interface
2. **Measures** operation latency using RAII guards and steady clocks
3. **Aggregates** metrics using thread-safe collectors and histograms
4. **Provides** SQL functions for metric retrieval and management

The design ensures minimal performance overhead while providing comprehensive observability.

## 🔗 Related Projects

- [DuckDB](https://duckdb.org/) - The analytical database engine
- [DuckDB Community Extensions](https://duckdb.org/community_extensions/) - Extension ecosystem
- [DuckDB HTTPFS](https://duckdb.org/docs/extensions/httpfs) - HTTP/S3 filesystem support

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

**Ready to gain visibility into your data access patterns?** Install ObserveFS today and start optimizing your DuckDB workflows!

```sql
INSTALL observefs FROM community;
LOAD observefs;
-- Your data operations are now being monitored! 🎉
```