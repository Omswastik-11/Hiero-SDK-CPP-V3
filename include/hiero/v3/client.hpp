#pragma once

#include <exception>
#include <functional>
#include <future>
#include <memory>

#include "hiero/v3/executor.hpp"
#include "hiero/v3/result.hpp"
#include "hiero/v3/transport.hpp"
#include "hiero/v3/types.hpp"

namespace hiero::v3 {

class Client final {
public:
  struct Options {
    int maxAttempts = 3;
  };

  Client(std::shared_ptr<IConsensusTransport> consensusTransport,
         std::shared_ptr<IMirrorTransport> mirrorTransport, Options options,
         std::shared_ptr<IExecutor> executor =
             std::make_shared<SingleThreadExecutor>());

  Client(std::shared_ptr<IConsensusTransport> consensusTransport,
         std::shared_ptr<IMirrorTransport> mirrorTransport,
         std::shared_ptr<IExecutor> executor =
             std::make_shared<SingleThreadExecutor>());

  Result<TransferReceipt> transfer(const TransferRequest &request);
  Result<BalanceResponse> getBalance(const BalanceRequest &request);
  Result<MirrorAccountResponse>
  getMirrorAccount(const MirrorAccountRequest &request);

  std::future<Result<TransferReceipt>> transferAsync(TransferRequest request);
  std::future<Result<BalanceResponse>> getBalanceAsync(BalanceRequest request);
  std::future<Result<MirrorAccountResponse>>
  getMirrorAccountAsync(MirrorAccountRequest request);

private:
  template <typename Operation>
  auto callWithRetries(Operation &&operation) -> decltype(operation());

  template <typename TResult>
  std::future<TResult> submitAsync(std::function<TResult()> operation);

  static bool IsRetriable(ErrorCode code);

  std::shared_ptr<IConsensusTransport> m_consensus;
  std::shared_ptr<IMirrorTransport> m_mirror;
  std::shared_ptr<IExecutor> m_executor;
  Options m_options;
};

template <typename Operation>
auto Client::callWithRetries(Operation &&operation) -> decltype(operation()) {
  using TResult = decltype(operation());

  for (int attempt = 1; attempt <= m_options.maxAttempts; ++attempt) {
    auto result = operation();

    if (result.ok()) {
      return result;
    }

    if (attempt == m_options.maxAttempts || !IsRetriable(result.error().code)) {
      return result;
    }
  }

  return TResult::Fail(ErrorCode::kInternal, "Retry loop exited unexpectedly");
}

template <typename TResult>
std::future<TResult> Client::submitAsync(std::function<TResult()> operation) {
  auto promise = std::make_shared<std::promise<TResult>>();
  auto future = promise->get_future();

  m_executor->execute([promise, operation = std::move(operation)]() mutable {
    try {
      promise->set_value(operation());
    } catch (...) {
      promise->set_exception(std::current_exception());
    }
  });

  return future;
}

} // namespace hiero::v3
