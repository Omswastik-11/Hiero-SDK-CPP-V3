#include "hiero/v3/transfer_builder.hpp"

#include <cstring>
#include <numeric>

namespace hiero::v3 {

Result<BuiltTransaction<TransferReceipt>>
TransferTransactionBuilder::build() const {
  // Validate: at least one transfer entry.
  if (m_transfers.empty()) {
    return Result<BuiltTransaction<TransferReceipt>>::Fail(
        ErrorCode::kInvalidArgument,
        "Transfer transaction requires at least one transfer entry");
  }

  // Validate: all account IDs are set.
  for (const auto &t : m_transfers) {
    if (!t.accountId.isSet()) {
      return Result<BuiltTransaction<TransferReceipt>>::Fail(
          ErrorCode::kInvalidArgument,
          "All transfer entries must have a valid accountId");
    }
  }

  // Validate: net amount sums to zero (conservation of value).
  int64_t netAmount = 0;
  for (const auto &t : m_transfers) {
    netAmount += t.amountTinybar;
  }

  if (netAmount != 0) {
    return Result<BuiltTransaction<TransferReceipt>>::Fail(
        ErrorCode::kInvalidArgument,
        "Transfer amounts must net to zero (debits + credits = 0)");
  }

  // Serialize into a simple byte representation.
  // In a real SDK this would produce a protobuf TransactionBody.
  std::vector<uint8_t> body;
  body.reserve(m_transfers.size() * sizeof(int64_t) * 4);

  for (const auto &t : m_transfers) {
    auto pushU64 = [&body](uint64_t val) {
      for (int i = 0; i < 8; ++i) {
        body.push_back(static_cast<uint8_t>((val >> (i * 8U)) & 0xFFU));
      }
    };

    pushU64(t.accountId.shard);
    pushU64(t.accountId.realm);
    pushU64(t.accountId.num);
    pushU64(static_cast<uint64_t>(t.amountTinybar));
  }

  return Result<BuiltTransaction<TransferReceipt>>::Ok(
      BuiltTransaction<TransferReceipt>("CryptoTransfer", std::move(body)));
}

} // namespace hiero::v3
