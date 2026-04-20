#pragma once

#include <cstdint>
#include <string>

namespace hiero::v3 {

using AccountId = std::string;

struct TransferRequest {
  AccountId fromAccountId;
  AccountId toAccountId;
  int64_t amountTinybar = 0;
};

struct TransferReceipt {
  std::string transactionId;
};

struct BalanceRequest {
  AccountId accountId;
};

struct BalanceResponse {
  AccountId accountId;
  int64_t balanceTinybar = 0;
};

struct MirrorAccountRequest {
  AccountId accountId;
};

struct MirrorAccountResponse {
  AccountId accountId;
  int64_t balanceTinybar = 0;
  std::string snapshotConsensusTimestamp;
};

} // namespace hiero::v3
