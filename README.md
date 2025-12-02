# Mimir

Mimir is a modern C++ build system that reads declarative YAML/TOML rules, builds a DAG of targets, computes file+command signatures, and executes only out-of-date commands. Named after the Norse figure of wisdom, it emphasizes clarity, correctness, and learning.

## Features

- Declarative build rules in YAML or TOML format
- Directed Acyclic Graph (DAG) of build targets
- SHA-based signature computation for files and commands
- Incremental builds - only rebuilds what changed
- Parallel execution with configurable job count
- Persistent local caching
- Cycle detection in dependency graphs
- Deterministic execution

## Building

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
mimir [OPTIONS] [COMMAND]

Commands:
  build       Build targets (default)
  clean       Clean cache

Options:
  -f FILE     Build file (default: build.yaml)
  -j N        Number of parallel jobs (default: 1)
  -h          Show help
```

## Build File Format

### YAML Format

```yaml
targets:
  - name: compile_main
    inputs:
      - main.c
    outputs:
      - main.o
    command: gcc -c main.c -o main.o
    dependencies: []

  - name: link_program
    inputs:
      - main.o
    outputs:
      - program
    command: gcc main.o -o program
    dependencies:
      - compile_main
```

### TOML Format

```toml
[target.compile_main]
name = "compile_main"
inputs = ["main.c"]
outputs = ["main.o"]
command = "gcc -c main.c -o main.o"
dependencies = []

[target.link_program]
name = "link_program"
inputs = ["main.o"]
outputs = ["program"]
command = "gcc main.o -o program"
dependencies = ["compile_main"]
```

## Architecture

- **Parser**: Reads YAML/TOML build rules and creates target objects
- **DAG**: Builds dependency graph and performs topological sorting
- **Signature**: Computes SHA-based signatures for files and commands
- **Cache**: Persists build signatures for incremental builds
- **Executor**: Executes build commands in correct order with parallel support

## Testing

```bash
cd build
ctest
```

## Example

See the `examples/` directory for sample build files and C programs.
