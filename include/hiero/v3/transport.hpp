#pragma once

#include "hiero/v3/result.hpp"
#include "hiero/v3/types.hpp"

namespace hiero::v3 {

// Interface for talking to consensus nodes (transactions + queries).
class IConsensusTransport {
public:
  virtual ~IConsensusTransport() = default;

  virtual Result<TransferReceipt>
  submitTransfer(const TransferRequest &request) = 0;
  virtual Result<BalanceResponse> getBalance(const BalanceRequest &request) = 0;
};

// Interface for reading data from mirror nodes.
class IMirrorTransport {
public:
  virtual ~IMirrorTransport() = default;

  virtual Result<MirrorAccountResponse>
  getAccount(const MirrorAccountRequest &request) = 0;
};

} // namespace hiero::v3
