#include "hiero/v3/client.hpp"
#include "hiero/v3/transaction.hpp"

#include <stdexcept>
#include <utility>

namespace hiero::v3 {

Client::Client(std::shared_ptr<IConsensusTransport> consensusTransport,
               std::shared_ptr<IMirrorTransport> mirrorTransport,
               Options options, std::shared_ptr<IExecutor> executor)
    : m_consensus(std::move(consensusTransport)),
      m_mirror(std::move(mirrorTransport)), m_executor(std::move(executor)),
      m_options(options) {
  if (!m_consensus) {
    throw std::invalid_argument("consensusTransport cannot be null");
  }

  if (!m_mirror) {
    throw std::invalid_argument("mirrorTransport cannot be null");
  }

  if (!m_executor) {
    throw std::invalid_argument("executor cannot be null");
  }

  if (m_options.maxAttempts < 1) {
    m_options.maxAttempts = 1;
  }
}

Client::Client(std::shared_ptr<IConsensusTransport> consensusTransport,
               std::shared_ptr<IMirrorTransport> mirrorTransport,
               std::shared_ptr<IExecutor> executor)
    : Client(std::move(consensusTransport), std::move(mirrorTransport),
             Options{}, std::move(executor)) {}

void Client::setOperator(AccountId accountId, PrivateKey key) {
  m_operator = OperatorAccount{accountId, std::move(key)};
}

Result<TransferReceipt> Client::transfer(const TransferRequest &request) {
  if (!request.fromAccountId.isSet() || !request.toAccountId.isSet()) {
    return Result<TransferReceipt>::Fail(
        ErrorCode::kInvalidArgument,
        "Transfer requires fromAccountId and toAccountId");
  }

  if (request.amountTinybar <= 0) {
    return Result<TransferReceipt>::Fail(ErrorCode::kInvalidArgument,
                                         "Transfer amount must be positive");
  }

  return callWithRetries(
      [this, &request]() { return m_consensus->submitTransfer(request); });
}

Result<BalanceResponse> Client::getBalance(const BalanceRequest &request) {
  if (!request.accountId.isSet()) {
    return Result<BalanceResponse>::Fail(ErrorCode::kInvalidArgument,
                                         "Balance query requires accountId");
  }

  return callWithRetries(
      [this, &request]() { return m_consensus->getBalance(request); });
}

Result<MirrorAccountResponse>
Client::getMirrorAccount(const MirrorAccountRequest &request) {
  if (!request.accountId.isSet()) {
    return Result<MirrorAccountResponse>::Fail(
        ErrorCode::kInvalidArgument, "Mirror query requires accountId");
  }

  return callWithRetries(
      [this, &request]() { return m_mirror->getAccount(request); });
}

std::future<Result<TransferReceipt>>
Client::transferAsync(TransferRequest request) {
  return submitAsync<Result<TransferReceipt>>(
      [this, request = std::move(request)]() { return transfer(request); });
}

std::future<Result<BalanceResponse>>
Client::getBalanceAsync(BalanceRequest request) {
  return submitAsync<Result<BalanceResponse>>(
      [this, request = std::move(request)]() { return getBalance(request); });
}

std::future<Result<MirrorAccountResponse>>
Client::getMirrorAccountAsync(MirrorAccountRequest request) {
  return submitAsync<Result<MirrorAccountResponse>>(
      [this, request = std::move(request)]() {
        return getMirrorAccount(request);
      });
}

bool Client::IsRetriable(ErrorCode code) {
  switch (code) {
  case ErrorCode::kNetwork:
  case ErrorCode::kInternal:
    return true;
  case ErrorCode::kInvalidArgument:
  case ErrorCode::kInsufficientBalance:
  case ErrorCode::kNotFound:
    return false;
  }

  return false;
}

// --- BuiltTransaction<TransferReceipt>::execute specialization ---

template <>
Result<TransferReceipt>
BuiltTransaction<TransferReceipt>::execute(Client &client) {
  if (!isSigned()) {
    return Result<TransferReceipt>::Fail(
        ErrorCode::kInvalidArgument,
        "Transaction must be signed before execution");
  }

  // Decode the serialized body back into a TransferRequest.
  // In a real SDK the transport layer would accept raw bytes.
  // For the PoC, we re-parse our simple encoding.
  if (m_serializedBody.size() < 32) {
    return Result<TransferReceipt>::Fail(ErrorCode::kInvalidArgument,
                                         "Malformed transaction body");
  }

  auto readU64 = [this](size_t offset) -> uint64_t {
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
      val |= static_cast<uint64_t>(m_serializedBody[offset + i]) << (i * 8U);
    }
    return val;
  };

  // Reconstruct the first debit and first credit as a simple transfer.
  // A full implementation would pass the raw bytes to the transport.
  AccountId fromAccount{};
  AccountId toAccount{};
  int64_t amount = 0;

  size_t entryCount = m_serializedBody.size() / 32;
  for (size_t i = 0; i < entryCount; ++i) {
    size_t off = i * 32;
    AccountId acct{readU64(off), readU64(off + 8), readU64(off + 16)};
    auto amt = static_cast<int64_t>(readU64(off + 24));

    if (amt < 0) {
      fromAccount = acct;
      amount = -amt;
    } else if (amt > 0) {
      toAccount = acct;
    }
  }

  TransferRequest request{fromAccount, toAccount, amount};
  return client.consensus().submitTransfer(request);
}

} // namespace hiero::v3
