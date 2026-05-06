# Architecture: Current Prototype and Next Steps

## 1) Purpose

This file documents:
- the current architecture implemented in this repository
- what is intentionally out of scope for the first prototype
- concrete next steps for expanding toward a fuller V3 SDK design

---

## 2) Current architecture (implemented)

The prototype follows a layered model:

1. Public API layer
- `Client` facade with operator account support
- `Result<T>` success/error return type
- typed request/response models (`TransferRequest`, `BalanceRequest`, `MirrorAccountRequest`, etc.)
- `TransferTransactionBuilder` for constructing transfer transactions
- `BuiltTransaction<ResponseT>` as an immutable, signable transaction

2. Key management layer
- `PrivateKey` for Ed25519 key representation (stubbed for prototype)
- `SignerFunction` as a pluggable signer callback
- `OperatorAccount` bundling an account ID with its signing key

3. Runtime layer
- retry-aware execution path inside `Client`
- `RetryPolicy` with configurable backoff parameters
- pluggable executor interface
- `SingleThreadExecutor` for async task dispatch

4. Transport abstraction layer
- `IConsensusTransport`
- `IMirrorTransport`

5. Prototype backend layer
- `InMemoryConsensusTransport`
- `InMemoryMirrorTransport`
- shared `LedgerState` for deterministic tests and demos

### Component diagram

```mermaid
flowchart LR
    APP[Application] --> CLIENT[Client API]

    CLIENT --> BUILDER[TransferTransactionBuilder]
    CLIENT --> RESULT[Result mapping]
    CLIENT --> RETRY[Retry policy]
    CLIENT --> EXEC[Executor interface]
    CLIENT --> KEYS[Keys / Signing]

    BUILDER --> BUILT_TX[BuiltTransaction]
    BUILT_TX --> CLIENT

    CLIENT --> CONS_IF[IConsensusTransport]
    CLIENT --> MIRROR_IF[IMirrorTransport]

    CONS_IF --> CONS_MEM[InMemoryConsensusTransport]
    MIRROR_IF --> MIRROR_MEM[InMemoryMirrorTransport]

    CONS_MEM --> LEDGER[(LedgerState)]
    MIRROR_MEM --> LEDGER
```

---

## 3) Request flow (current)

### Transaction lifecycle: build -> sign -> execute

```mermaid
sequenceDiagram
    participant App as Application
    participant Builder as TransferTransactionBuilder
    participant TX as BuiltTransaction
    participant Client as Client
    participant Cons as IConsensusTransport

    App->>Builder: addTransfer(from, amount)
    App->>Builder: addTransfer(to, amount)
    App->>Builder: build()
    Builder-->>App: Result<BuiltTransaction>

    App->>TX: sign(operatorKey)
    App->>TX: execute(client)
    TX->>Client: submit to consensus
    Client->>Cons: submitTransfer(request)
    Cons-->>Client: Result<TransferReceipt>
    Client-->>TX: Result<TransferReceipt>
    TX-->>App: Result<TransferReceipt>
```

### Transfer and follow-up reads (direct API)

```mermaid
sequenceDiagram
    participant App as Application
    participant Client as Client
    participant Cons as IConsensusTransport
    participant Mirror as IMirrorTransport

    App->>Client: transfer(from, to, amount)
    Client->>Client: validate request
    Client->>Client: callWithRetries(...)
    Client->>Cons: submitTransfer(...)
    Cons-->>Client: Result<TransferReceipt>
    Client-->>App: Result<TransferReceipt>

    App->>Client: getBalance(account)
    Client->>Cons: getBalance(...)
    Cons-->>Client: Result<BalanceResponse>
    Client-->>App: Result<BalanceResponse>

    App->>Client: getMirrorAccount(account)
    Client->>Mirror: getAccount(...)
    Mirror-->>Client: Result<MirrorAccountResponse>
    Client-->>App: Result<MirrorAccountResponse>
```

---

## 4) Current design choices

1. Stable public contract first
- Public API uses typed models and `Result<T>` rather than exposing backend protocol types.

2. Transport isolation
- Consensus and mirror paths are represented as interfaces so backend implementations can change without breaking API shape.

3. Two-step transaction lifecycle
- Mutable builder collects fields, validates at `build()` time, and produces an immutable `BuiltTransaction`.
- `BuiltTransaction` carries the response type (`ResponseT`) through signing and execution without unsafe casts.

4. Explicit signing
- `PrivateKey` and `SignerFunction` let the developer control exactly which keys sign a transaction.
- Multiple signatures are supported by calling `sign()` more than once.

5. Operator identity
- `OperatorAccount` pairs an account ID with its key, wired into the Client for default signing.

6. Async abstraction
- Async methods use an executor interface rather than hard-coding `std::async` as the only execution model.

7. Testability by design
- In-memory backend allows deterministic unit and integration-style tests for the first vertical slice.

---

## 5) Current limitations

- No real network transport adapters yet (in-memory only).
- `BuiltTransaction::execute` currently has a template specialization for `TransferReceipt`; a generic dispatch is needed for more transaction types.
- Error code set is intentionally small.
- No wire-level serialization/protobuf mapping in this repository yet.
- Signing uses a simplified stub; real Ed25519 would require OpenSSL or libsodium.

---

## 6) Next steps roadmap

### Phase A: transport and protocol integration
- add real consensus adapter behind `IConsensusTransport`
- add real mirror adapter behind `IMirrorTransport`
- keep protocol-specific classes private to adapter implementation

### Phase B: API expansion
- add at least one additional transaction family (e.g. AccountCreate)
- add additional query family
- generalize the BuiltTransaction dispatch so specializations are not needed per type

### Phase C: runtime hardening
- integrate `RetryPolicy` with actual backoff timing
- add explicit timeout/deadline handling in runtime policy
- expand async strategy options (single thread, thread pool, external scheduler)

### Phase D: quality and compatibility
- add transport contract tests
- add failure-mode matrix tests
- define non-breaking evolution policy (deprecation and versioning guidance)

### Phase E: documentation and upstream contribution
- propose C++ guideline improvements in `sdk-collaboration-hub`
- provide evidence-backed examples from prototype behavior
- align docs with parity-template baseline expectations

---

## 7) Short-term milestone (what success looks like next)

Next milestone is successful when:
- real transport adapter interfaces are wired (or mocked to exact contracts)
- one additional transaction/query is added without API churn
- `BuiltTransaction::execute` works generically without per-type specializations
- test coverage includes retry and async failure scenarios
- documentation updates include concrete before/after proposals upstream
