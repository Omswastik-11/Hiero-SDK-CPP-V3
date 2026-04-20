#include "hiero/v3/client.hpp"

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

Result<TransferReceipt> Client::transfer(const TransferRequest &request) {
  if (request.fromAccountId.empty() || request.toAccountId.empty()) {
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
  if (request.accountId.empty()) {
    return Result<BalanceResponse>::Fail(ErrorCode::kInvalidArgument,
                                         "Balance query requires accountId");
  }

  return callWithRetries(
      [this, &request]() { return m_consensus->getBalance(request); });
}

Result<MirrorAccountResponse>
Client::getMirrorAccount(const MirrorAccountRequest &request) {
  if (request.accountId.empty()) {
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

} // namespace hiero::v3
