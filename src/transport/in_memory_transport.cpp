#include "hiero/v3/in_memory_transport.hpp"

#include <stdexcept>
#include <string>

namespace hiero::v3 {

namespace {

bool isValidAccountId(const AccountId &accountId) { return !accountId.empty(); }

std::string makeTimestamp(uint64_t counter) {
  return std::to_string(1700000000ULL + counter);
}

} // namespace

InMemoryConsensusTransport::InMemoryConsensusTransport(
    std::shared_ptr<LedgerState> state)
    : m_state(std::move(state)) {
  if (!m_state) {
    throw std::invalid_argument("state cannot be null");
  }
}

void InMemoryConsensusTransport::setBalance(const AccountId &accountId,
                                            int64_t amountTinybar) {
  if (!isValidAccountId(accountId)) {
    throw std::invalid_argument("accountId cannot be empty");
  }

  std::lock_guard<std::mutex> lock(m_state->mutex);
  m_state->balances[accountId] = amountTinybar;
}

Result<TransferReceipt>
InMemoryConsensusTransport::submitTransfer(const TransferRequest &request) {
  if (!isValidAccountId(request.fromAccountId) ||
      !isValidAccountId(request.toAccountId)) {
    return Result<TransferReceipt>::Fail(ErrorCode::kInvalidArgument,
                                         "Transfer requires valid account IDs");
  }

  if (request.amountTinybar <= 0) {
    return Result<TransferReceipt>::Fail(ErrorCode::kInvalidArgument,
                                         "Transfer amount must be positive");
  }

  std::lock_guard<std::mutex> lock(m_state->mutex);

  auto fromIt = m_state->balances.find(request.fromAccountId);
  if (fromIt == m_state->balances.end()) {
    return Result<TransferReceipt>::Fail(ErrorCode::kNotFound,
                                         "Source account not found");
  }

  if (fromIt->second < request.amountTinybar) {
    return Result<TransferReceipt>::Fail(ErrorCode::kInsufficientBalance,
                                         "Insufficient balance");
  }

  fromIt->second -= request.amountTinybar;
  m_state->balances[request.toAccountId] += request.amountTinybar;

  ++m_state->transactionCounter;

  return Result<TransferReceipt>::Ok(
      TransferReceipt{"tx-" + std::to_string(m_state->transactionCounter)});
}

Result<BalanceResponse>
InMemoryConsensusTransport::getBalance(const BalanceRequest &request) {
  if (!isValidAccountId(request.accountId)) {
    return Result<BalanceResponse>::Fail(ErrorCode::kInvalidArgument,
                                         "Balance request requires accountId");
  }

  std::lock_guard<std::mutex> lock(m_state->mutex);

  auto it = m_state->balances.find(request.accountId);
  if (it == m_state->balances.end()) {
    return Result<BalanceResponse>::Fail(ErrorCode::kNotFound,
                                         "Account not found");
  }

  return Result<BalanceResponse>::Ok(
      BalanceResponse{request.accountId, it->second});
}

InMemoryMirrorTransport::InMemoryMirrorTransport(
    std::shared_ptr<LedgerState> state)
    : m_state(std::move(state)) {
  if (!m_state) {
    throw std::invalid_argument("state cannot be null");
  }
}

Result<MirrorAccountResponse>
InMemoryMirrorTransport::getAccount(const MirrorAccountRequest &request) {
  if (!isValidAccountId(request.accountId)) {
    return Result<MirrorAccountResponse>::Fail(
        ErrorCode::kInvalidArgument, "Mirror request requires accountId");
  }

  std::lock_guard<std::mutex> lock(m_state->mutex);

  auto it = m_state->balances.find(request.accountId);
  if (it == m_state->balances.end()) {
    return Result<MirrorAccountResponse>::Fail(ErrorCode::kNotFound,
                                               "Account not found on mirror");
  }

  return Result<MirrorAccountResponse>::Ok(MirrorAccountResponse{
      request.accountId,
      it->second,
      makeTimestamp(m_state->transactionCounter),
  });
}

} // namespace hiero::v3
