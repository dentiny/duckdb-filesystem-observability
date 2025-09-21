## Getting started
First step to getting started is to create your own repo from this template by clicking `Use this template`. Then clone your new repository using 
```sh
git clone --recurse-submodules https://github.com/dentiny/duckdb-filesystem-observability.git
```
Note that `--recurse-submodules` will ensure DuckDB is pulled which is required to build the extension.

## Building
### Managing dependencies
DuckDB extensions uses VCPKG for dependency management. Enabling VCPKG is very simple: follow the [installation instructions](https://vcpkg.io/en/getting-started) or just run the following:
```shell
cd <your-working-dir-not-the-plugin-repo>
git clone https://github.com/Microsoft/vcpkg.git
sh ./vcpkg/scripts/bootstrap.sh -disableMetrics
export VCPKG_TOOLCHAIN_PATH=`pwd`/vcpkg/scripts/buildsystems/vcpkg.cmake
```
Note: VCPKG is only required for extensions that want to rely on it for dependency management. If you want to develop an extension without dependencies, or want to do your own dependency management, just skip this step. Note that the example extension uses VCPKG to build with a dependency for instructive purposes, so when skipping this step the build may not work without removing the dependency.

### Build steps
Now to build the extension, run:
```sh
make <reldebug>
```
The main binaries that will be built are:
```sh
./build/release/duckdb
./build/release/test/unittest
./build/release/extension/<extension_name>/<extension_name>.duckdb_extension
```
- `duckdb` is the binary for the duckdb shell with the extension code automatically loaded. 
- `unittest` is the test runner of duckdb. Again, the extension is already linked into the binary.
- `<extension_name>.duckdb_extension` is the loadable binary as it would be distributed.

### Tips for speedy builds
DuckDB extensions currently rely on DuckDB's build system to provide easy testing and distributing. This does however come at the downside of requiring the template to build DuckDB and its unittest binary every time you build your extension. To mitigate this, we highly recommend installing [ccache](https://ccache.dev/) and [ninja](https://ninja-build.org/). This will ensure you only need to build core DuckDB once and allows for rapid rebuilds.

To build using ninja and ccache ensure both are installed and run:

```sh
GEN=ninja make
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

## Running the extension
To run the extension code, simply start the shell with `./build/release/duckdb`. This shell will have the extension pre-loaded.  

Now we can use the features from the extension directly in DuckDB. The template contains a single scalar function `quack()` that takes a string arguments and returns a string:
```
D SELECT * FROM duckdb_extensions() WHERE extension_name = 'observefs';
┌────────────────┬─────────┬───────────┬──────────────┬─────────────┬───────────┬───────────────────┬───────────────────┬────────────────┐
│ extension_name │ loaded  │ installed │ install_path │ description │  aliases  │ extension_version │   install_mode    │ installed_from │
│    varchar     │ boolean │  boolean  │   varchar    │   varchar   │ varchar[] │      varchar      │      varchar      │    varchar     │
├────────────────┼─────────┼───────────┼──────────────┼─────────────┼───────────┼───────────────────┼───────────────────┼────────────────┤
│ observefs      │ true    │ true      │ (BUILT-IN)   │             │ []        │                   │ STATICALLY_LINKED │                │
└────────────────┴─────────┴───────────┴──────────────┴─────────────┴───────────┴───────────────────┴───────────────────┴────────────────┘
```

## Running the tests
Different tests can be created for DuckDB extensions. The primary way of testing DuckDB extensions should be the SQL tests in `./test/sql`. These SQL tests can be run using:
```sh
make test-all
```
