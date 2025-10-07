# 0.3.2

## Changed

- Upgrade duckdb to v1.4.1, and duckdb-httpfs and extension-ci-tools to latest

# 0.3.1

## Added

- Add a SQL function to list all registered filesystems ([#38])

[#38]: https://github.com/dentiny/duckdb-filesystem-observability/pull/38

## Fixed

- Fix bug that httpfs filesystems are registered twice, one as unwrapped raw filesystem instance, one wrapped in observefs 

[#38]: https://github.com/dentiny/duckdb-filesystem-observability/pull/38

# 0.3.0

## Added

- Add read request size metrics ([#30])

[#30]: https://github.com/dentiny/duckdb-filesystem-observability/pull/30

## Fixed

- Fix HTTP request stats report ([#35])

[#35]: https://github.com/dentiny/duckdb-filesystem-observability/pull/35

- Fix quantile calculation when there're small number of data points ([#36])

[#36]: https://github.com/dentiny/duckdb-filesystem-observability/pull/36

# 0.2.0

## Added

- Support wrapping ANY duckdb compatible filesystems ([#24])

[#24]: https://github.com/dentiny/duckdb-filesystem-observability/pull/24

## Improved

- Rewrite quantile estimation for better accuracy ([#17], [#18], [#19])

[#17]: https://github.com/dentiny/duckdb-filesystem-observability/pull/17
[#18]: https://github.com/dentiny/duckdb-filesystem-observability/pull/18
[#19]: https://github.com/dentiny/duckdb-filesystem-observability/pull/19

- Refine display output if there's no interested IO operations happening in the current filesystem ([#25])

[#25]: https://github.com/dentiny/duckdb-filesystem-observability/pull/25
