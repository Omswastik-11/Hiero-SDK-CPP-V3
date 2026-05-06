#pragma once

#include <cstdint>
#include <vector>

#include "hiero/v3/result.hpp"
#include "hiero/v3/transaction.hpp"
#include "hiero/v3/types.hpp"

namespace hiero::v3 {

// Builder for constructing a CryptoTransfer transaction.
//
// Usage:
//   auto tx = TransferTransactionBuilder()
//       .addTransfer(alice, -500)
//       .addTransfer(bob, 500)
//       .build();
//   tx.sign(operatorKey);
//   auto result = tx.execute(client);
//
// The builder validates inputs at build() time and produces an immutable
// BuiltTransaction that carries the response type through signing and execution.
class TransferTransactionBuilder final {
public:
  struct Transfer {
    AccountId accountId;
    int64_t amountTinybar = 0;
  };

  /// Add a debit or credit entry to this transfer.
  /// Negative amounts are debits, positive are credits.
  TransferTransactionBuilder &addTransfer(const AccountId &accountId,
                                          int64_t amountTinybar) {
    m_transfers.push_back({accountId, amountTinybar});
    return *this;
  }

  /// Convenience: add a sender->receiver transfer pair.
  TransferTransactionBuilder &addHbarTransfer(const AccountId &from,
                                              const AccountId &to,
                                              int64_t amountTinybar) {
    m_transfers.push_back({from, -amountTinybar});
    m_transfers.push_back({to, amountTinybar});
    return *this;
  }

  /// Set a memo on this transaction.
  TransferTransactionBuilder &setMemo(std::string memo) {
    m_memo = std::move(memo);
    return *this;
  }

  /// Validate inputs and produce an immutable BuiltTransaction.
  /// Returns an error if validation fails (e.g. transfers don't net to zero).
  [[nodiscard]] Result<BuiltTransaction<TransferReceipt>> build() const;

  /// Read-only access to the transfer list for testing.
  [[nodiscard]] const std::vector<Transfer> &transfers() const noexcept {
    return m_transfers;
  }

private:
  std::vector<Transfer> m_transfers;
  std::string m_memo;
};

} // namespace hiero::v3
