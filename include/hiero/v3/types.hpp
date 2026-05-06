#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace hiero::v3 {

// Identifies an account on the Hiero network (shard.realm.num format).
struct AccountId {
  uint64_t shard = 0;
  uint64_t realm = 0;
  uint64_t num = 0;

  [[nodiscard]] bool isSet() const noexcept { return num != 0; }
};

inline bool operator==(const AccountId &lhs, const AccountId &rhs) {
  return lhs.shard == rhs.shard && lhs.realm == rhs.realm && lhs.num == rhs.num;
}

inline std::string ToString(const AccountId &accountId) {
  return std::to_string(accountId.shard) + "." +
         std::to_string(accountId.realm) + "." + std::to_string(accountId.num);
}

// Request for a simple hbar transfer between two accounts.
struct TransferRequest {
  AccountId fromAccountId;
  AccountId toAccountId;
  int64_t amountTinybar = 0;
};

// Receipt returned after a successful transfer.
struct TransferReceipt {
  std::string transactionId;
};

// Request to look up an account's hbar balance.
struct BalanceRequest {
  AccountId accountId;
};

struct BalanceResponse {
  AccountId accountId;
  int64_t balanceTinybar = 0;
};

// Request to read account data from a mirror node.
struct MirrorAccountRequest {
  AccountId accountId;
};

struct MirrorAccountResponse {
  AccountId accountId;
  int64_t balanceTinybar = 0;
  std::string snapshotConsensusTimestamp;
};

} // namespace hiero::v3

namespace std {

template <> struct hash<hiero::v3::AccountId> {
  size_t operator()(const hiero::v3::AccountId &accountId) const noexcept {
    const size_t shardHash = std::hash<uint64_t>{}(accountId.shard);
    const size_t realmHash = std::hash<uint64_t>{}(accountId.realm);
    const size_t numHash = std::hash<uint64_t>{}(accountId.num);

    return shardHash ^ (realmHash << 1U) ^ (numHash << 2U);
  }
};

} // namespace std
