# Hiero C++ V3 SDK Prototype

A minimal but runnable C++ prototype for the Hiero V3 SDK architecture. Built for architecture validation, mentorship discussion, and upstream documentation improvements.

## Why this exists

- Validate a V3-style SDK shape before broad migration.
- Demonstrate a full transaction lifecycle: `build -> sign -> execute`.
- Demonstrate query and mirror read paths.
- Test non-breaking API patterns (stable public types and isolated transport details).
- Provide concrete evidence for upstream documentation improvements.

## What is implemented

1. **Public API surface**
- `Result<T>` model for typed success/failure handling.
- Core request/response types for transfer, balance query, and mirror query.
- `Client` facade with sync and async APIs and operator account support.

2. **Transaction lifecycle**
- `TransferTransactionBuilder` for constructing transfers with a builder pattern.
- `BuiltTransaction<ResponseT>` as an immutable, signable transaction.
- Validation at build time (net-zero balance, valid accounts).
- Type-safe response propagation through build, sign, and execute.

3. **Key management**
- `PrivateKey` with deterministic test key generation.
- `SignerFunction` as a pluggable signer callback.
- `OperatorAccount` bundling an account ID with its signing key.
- Support for multiple signatures on a single transaction.

4. **Runtime and async execution**
- Retry-aware call path in `Client` for retriable errors.
- `RetryPolicy` with configurable backoff parameters.
- Pluggable executor interface (`IExecutor`).
- `SingleThreadExecutor` implementation.

5. **Transport abstraction and prototype backends**
- `IConsensusTransport` and `IMirrorTransport` interfaces.
- In-memory consensus transport for transfer and balance operations.
- In-memory mirror transport for account read operations.
- Thread-safe shared `LedgerState` for deterministic testing.

6. **Validation assets**
- Two example apps: basic transfer flow and full builder lifecycle.
- Two test suites: minimal V3 tests and builder lifecycle tests.
- Docker-based build, test, and lint workflow.

## Repository layout

```text
.
|-- CMakeLists.txt
|-- include/hiero/v3/
|   |-- client.hpp
|   |-- executor.hpp
|   |-- in_memory_transport.hpp
|   |-- keys.hpp
|   |-- policy.hpp
|   |-- result.hpp
|   |-- transaction.hpp
|   |-- transfer_builder.hpp
|   |-- transport.hpp
|   `-- types.hpp
|-- src/
|   |-- client/client.cpp
|   |-- keys/keys.cpp
|   |-- runtime/executor.cpp
|   |-- transactions/transfer_builder.cpp
|   `-- transport/in_memory_transport.cpp
|-- tests/
|   |-- minimal_v3_tests.cpp
|   `-- builder_lifecycle_tests.cpp
|-- examples/
|   |-- transfer_and_balance.cpp
|   `-- builder_sign_execute.cpp
|-- tooling/docker/dev.Dockerfile
|-- docker-compose.yml
|-- scripts/
|   |-- build-and-test.sh
|   `-- lint.sh
`-- docs/
    `-- architecture.md
```

## Prerequisites

- Docker Desktop (Windows, macOS, or Linux)
- Docker Compose v2 (`docker compose` command)

No local compiler setup is required for the default workflow.

## Docker-first workflow (recommended)

1. Build the dev container image:

```bash
docker compose build dev
```

2. Configure, build, and run all tests:

```bash
docker compose run --rm dev bash -c "cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j && ctest --test-dir build --output-on-failure"
```

3. Run the builder lifecycle example:

```bash
docker compose run --rm dev bash -c "cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j && ./build/builder_sign_execute"
```

4. Run format and lint checks:

```bash
docker compose run --rm dev bash -c "bash ./scripts/lint.sh"
```

## Optional local workflow (non-Docker)

If you have a local compiler toolchain already configured:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Documentation

- [Architecture and next steps](docs/architecture.md)

## Current limitations

- In-memory transports are prototype backends only. No real network integration yet.
- Signing uses a simplified stub. Real Ed25519 would require OpenSSL or libsodium.
- Error model is intentionally small for iteration speed.
- `BuiltTransaction::execute` uses a template specialization; a generic dispatch is planned.

## Next implementation targets

1. Add `TransactionId` generation from operator account and valid-start timestamp.
2. Generalize the transaction pipeline so new transaction types plug in cleanly.
3. Add transport adapters that map to real consensus and mirror interfaces.
4. Expand API surface with additional transaction and query families.
5. Strengthen tests with transport contract checks and failure scenarios.
6. Propose upstream doc improvements backed by prototype evidence.
