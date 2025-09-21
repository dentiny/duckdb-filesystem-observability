# ObserveFS - DuckDB Filesystem Observability Extension

## What is ObserveFS?

`observefs` is a powerful DuckDB extension that provides comprehensive **filesystem observability** for your data operations. It transparently wraps httpfs (HTTP, S3, Hugging Face) with monitoring capabilities, giving you detailed insights into I/O performance, latency patterns, and usage metrics.

Whether you're optimizing data pipelines, debugging performance issues, or understanding access patterns, ObserveFS gives you the visibility you need.

## Usage
```sql
-- Install and load the ObserveFS extension
FORCE INSTALL observefs;
LOAD observefs;

-- Query remote data (automatically monitored)
SELECT count(*) FROM 'https://huggingface.co/datasets/open-r1/OpenR1-Math-220k/resolve/main/data/train-00003-of-00010.parquet';

-- View detailed performance metrics
COPY (SELECT observefs_get_profile()) TO '/tmp/output.txt';

-- Clear metrics for fresh analysis
SELECT observefs_clear();

-- Wrap ANY filesystem which is compatible with duckdb, for example, azure filesystem.
SELECT observefs_wrap_filesystem('AzureBlobStorageFileSystem');
```

The output includes comprehensive metrics:
- Operation-specific latency histograms (READ, OPEN, LIST)
- Quantile analysis (P50, P75, P90, P95, P99)
- Per-bucket performance breakdown
- Min/Max/Mean latency statistics

### Extension Integration

The extension extends DuckDB's httpfs functionality by wrapping HTTP filesystems with observability. It maintains compatibility with existing httpfs features while adding comprehensive I/O monitoring.
