# ObserveFS - DuckDB Filesystem Observability Extension


## What is ObserveFS?

ObserveFS is a powerful DuckDB extension that provides comprehensive **filesystem observability** for your data operations. It transparently wraps httpfs (HTTP, S3, Hugging Face) with monitoring capabilities, giving you detailed insights into I/O performance, latency patterns, and usage metrics.

Whether you're optimizing data pipelines, debugging performance issues, or understanding access patterns, ObserveFS gives you the visibility you need.

## Usage

```sql
-- Install and load the ObserveFS extension
FORCE INSTALL observefs;
LOAD observefs;

-- Query remote data (automatically monitored)
SELECT count(*) FROM 'https://huggingface.co/datasets/open-r1/OpenR1-Math-220k/resolve/main/data/train-00003-of-00010.parquet';

-- View detailed performance metrics
SELECT observefs_get_profile();

-- Clear metrics for fresh analysis
SELECT observefs_clear();
```

The output includes comprehensive metrics:
- Operation-specific latency histograms (READ, OPEN, LIST)
- Quantile analysis (P50, P75, P90, P95, P99)
- Per-bucket performance breakdown
- Min/Max/Mean latency statistics

## Getting started
Clone this repository using:
```sh
git clone --recurse-submodules https://github.com/dentiny/duckdb-filesystem-observability.git
```
Note that `--recurse-submodules` will ensure DuckDB is pulled which is required to build the extension.

## Building

### Prerequisites

The easiest way to build and develop ObserveFS is using the provided dev container, which includes all necessary dependencies and tools.

### Build steps

Once you're in the dev container (see Development Environment section below), build the extension:

```bash
# Build the extension (release version)
make

# Faster parallel build using all available CPU cores
CMAKE_BUILD_PARALLEL_LEVEL=$(nproc) make

# Or build with faster ninja generator
GEN=ninja make

# Combine both for maximum speed
CMAKE_BUILD_PARALLEL_LEVEL=$(nproc) GEN=ninja make
```

The main binaries that will be built are:
```bash
./build/release/duckdb
./build/release/test/unittest
./build/release/extension/observefs/observefs.duckdb_extension
```

- `duckdb` is the binary for the duckdb shell with the ObserveFS extension automatically loaded
- `unittest` is the test runner for SQL-based tests
- `observefs.duckdb_extension` is the loadable extension binary

### Build Configurations

Different build configurations are available:
```bash
make release    # Release build (default)
make debug      # Debug build
make reldebug   # Release with debug info

# For faster builds, use parallel compilation
CMAKE_BUILD_PARALLEL_LEVEL=$(nproc) make release
CMAKE_BUILD_PARALLEL_LEVEL=$(nproc) make debug
```

## Running the extension
To run the extension code from a development build, start the shell with `./build/release/duckdb`. This shell will have the ObserveFS extension pre-loaded and ready to use.

## Running the tests

ObserveFS includes both SQL-based integration tests and C++ unit tests.

### SQL Integration Tests
SQL-based tests are located in the `test/sql/` directory and can be run with:
```bash
make test
```

### Unit Tests
C++ unit tests are available for core components:
```bash
# Run all unit tests
make test_unit

# Run specific unit test binaries
./build/release/extension/observefs/test_histogram
./build/release/extension/observefs/test_quantile_estimator
./build/release/extension/observefs/test_string_utils
./build/release/extension/observefs/test_no_destructor
```

Note: All commands should be run inside the dev container environment.

## Architecture Overview

### Core Components

1. **ObservabilityFileSystem** (`src/observability_filesystem.cpp`)
   - Main wrapper filesystem that intercepts all I/O operations
   - Delegates actual operations to an internal filesystem while collecting metrics
   - Implements the DuckDB FileSystem interface

2. **MetricsCollector** (`src/metrics_collector.cpp`)
   - Centralized metrics collection and aggregation
   - Manages overall and per-bucket latency histograms
   - Thread-safe operations using mutexes

3. **OperationLatencyCollector** (`src/operation_latency_collector.cpp`)
   - Tracks latency statistics for different I/O operations (READ, WRITE, OPEN, etc.)
   - Uses histogram-based latency tracking with quantile estimation

4. **ObservefsExtension** (`src/observefs_extension.cpp`)
   - Main extension entry point
   - Loads and integrates with the httpfs extension
   - Registers the observability filesystem

### Key Data Structures

- **Histogram** (`src/histogram.cpp`) - General-purpose histogram implementation
- **QuantileEstimator** (`src/quantile_estimator.cpp`) - Efficient quantile computation
- **QuantileLite** (`src/quantilelite.cpp`) - Lightweight quantile estimation
- **FileSystemRefRegistry** (`src/filesystem_ref_registry.cpp`) - Manages filesystem references

### Extension Integration

The extension extends DuckDB's httpfs functionality by wrapping HTTP filesystems with observability. It maintains compatibility with existing httpfs features while adding comprehensive I/O monitoring.

## Distribution

ObserveFS can be distributed through the [DuckDB community extensions repository](https://github.com/duckdb/community-extensions). Once available, it can be installed with:

```SQL
INSTALL observefs FROM community;
LOAD observefs;
```

For development builds, the extension can be loaded directly:
```SQL
LOAD '/path/to/observefs.duckdb_extension';
```

## Development Environment Setup

### Code Formatting

Format all C++ code and CMake files:
```bash
make format-all
```

### Development Services

The dev container includes additional services for testing:
- **MinIO**: S3-compatible object storage for testing S3 functionality
  - S3 API: `http://localhost:29000`
  - Web UI: `http://localhost:29001`
  - Credentials: `minioadmin` / `minioadmin`
- **Fake GCS**: Google Cloud Storage emulator for testing GCS functionality
  - API: `http://localhost:24443`

### Recommended Development Environment

#### VS Code with Dev Containers (Recommended)

The easiest way to get started with development is using VS Code with dev containers. This provides a consistent, pre-configured development environment with all necessary dependencies.

1. **Prerequisites**:
   - Install [VS Code](https://code.visualstudio.com/)
   - Install [Docker](https://www.docker.com/get-started)
   - Install the [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

2. **Setup**:
   ```bash
   git clone --recurse-submodules https://github.com/dentiny/duckdb-filesystem-observability.git
   cd duckdb-filesystem-observability
   code .
   ```

3. **Open in Dev Container**:
   - VS Code will detect the dev container configuration
   - Click "Reopen in Container" when prompted, or use `Ctrl+Shift+P` → "Dev Containers: Reopen in Container"
   - The container will automatically build with all dependencies (VCPKG, build tools, etc.)

4. **Development Workflow**:
   ```bash
   # Build the extension (fast parallel build)
   CMAKE_BUILD_PARALLEL_LEVEL=$(nproc) GEN=ninja make

   # Run tests
   make test

   # Run unit tests
   make test_unit

   # Format code
   make format-all
   ```

The dev container includes:
- All build dependencies (VCPKG, OpenSSL, CURL)
- C++ development tools and extensions
- Pre-configured CMake and debugging support
- Git and other development utilities