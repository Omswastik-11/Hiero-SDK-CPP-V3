#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "hiero/v3/transport.hpp"

namespace hiero::v3 {

struct LedgerState {
  mutable std::mutex mutex;
  std::unordered_map<AccountId, int64_t> balances;
  uint64_t transactionCounter = 0;
};

class InMemoryConsensusTransport final : public IConsensusTransport {
public:
  explicit InMemoryConsensusTransport(std::shared_ptr<LedgerState> state);

  void setBalance(const AccountId &accountId, int64_t amountTinybar);

  Result<TransferReceipt>
  submitTransfer(const TransferRequest &request) override;
  Result<BalanceResponse> getBalance(const BalanceRequest &request) override;

private:
  std::shared_ptr<LedgerState> m_state;
};

class InMemoryMirrorTransport final : public IMirrorTransport {
public:
  explicit InMemoryMirrorTransport(std::shared_ptr<LedgerState> state);

  Result<MirrorAccountResponse>
  getAccount(const MirrorAccountRequest &request) override;

private:
  std::shared_ptr<LedgerState> m_state;
};

} // namespace hiero::v3
