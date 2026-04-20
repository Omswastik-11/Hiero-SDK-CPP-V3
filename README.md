# Hiero C++ V3 Minimal Prototype

This repository contains a first minimal C++ V3 SDK prototype intended for architecture validation, mentorship discussion, and iterative documentation improvements.

The prototype is intentionally small but runnable end-to-end.

## Why this exists

- Validate a V3-style SDK shape before broad migration.
- Demonstrate one transaction path, one query path, and one mirror read path.
- Test non-breaking API patterns (stable public types and isolated transport details).
- Provide concrete evidence for upstream documentation improvements.

## What is implemented now

1. Public API surface
- `Result<T>` model for typed success/failure handling.
- Core request/response types for transfer, balance query, and mirror query.
- `Client` facade with sync and async APIs.

2. Runtime and async execution
- Retry-aware call path in `Client` for retriable errors.
- Pluggable executor interface.
- `SingleThreadExecutor` implementation (avoids direct `std::async` dependency as the only strategy).

3. Transport abstraction and prototype backends
- `IConsensusTransport` and `IMirrorTransport` interfaces.
- In-memory consensus transport for transfer and balance operations.
- In-memory mirror transport for account read operations.

4. Validation assets
- Example app: transfer and balance flow.
- Minimal test suite with retry and async coverage.
- Docker-based build, test, and lint workflow.

## Repository layout

```text
.
|-- CMakeLists.txt
|-- include/hiero/v3/
|   |-- client.hpp
|   |-- executor.hpp
|   |-- in_memory_transport.hpp
|   |-- result.hpp
|   |-- transport.hpp
|   `-- types.hpp
|-- src/
|   |-- client/client.cpp
|   |-- runtime/executor.cpp
|   `-- transport/in_memory_transport.cpp
|-- tests/minimal_v3_tests.cpp
|-- examples/transfer_and_balance.cpp
|-- tooling/docker/dev.Dockerfile
|-- docker-compose.yml
|-- scripts/
|   |-- build-and-test.sh
|   `-- lint.sh
`-- docs/
	|-- architecture.md
	`-- v3-cpp-prototype-foundation-and-plan.md
```

## Prerequisites

- Docker Desktop (Windows)
- Docker Compose v2 (`docker compose` command)

No local compiler setup is required for the default workflow.

## Docker-first workflow (recommended)

1. Build the dev container image:

```bash
docker compose build dev
```

2. Configure, build, and run tests:

```bash
docker compose run --rm dev bash -lc "bash ./scripts/build-and-test.sh"
```

3. Run format and lint checks:

```bash
docker compose run --rm dev bash -lc "bash ./scripts/lint.sh"
```

4. Run the example app:

```bash
docker compose run --rm dev bash -lc "cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j && ./build/transfer_and_balance"
```

## Optional local workflow (non-Docker)

If you have a local compiler toolchain already configured:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Documentation

- [Prototype foundation and execution plan](docs/v3-cpp-prototype-foundation-and-plan.md)
- [Current architecture and next steps](docs/architecture.md)

## Current limitations

- In-memory transports are prototype backends only.
- No real network integration in this first slice.
- Error model is intentionally small for iteration speed.
- No packaging/distribution artifacts yet.

## Next implementation targets

1. Add transport adapters that map to real consensus and mirror interfaces.
2. Expand API surface with additional transaction and query families.
3. Strengthen tests with transport contract checks and failure scenarios.
4. Propose upstream doc improvements backed by prototype evidence.
