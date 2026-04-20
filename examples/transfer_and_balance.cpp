#include <iostream>
#include <memory>

#include "hiero/v3/client.hpp"
#include "hiero/v3/in_memory_transport.hpp"

int main() {
  using hiero::v3::BalanceRequest;
  using hiero::v3::Client;
  using hiero::v3::InMemoryConsensusTransport;
  using hiero::v3::InMemoryMirrorTransport;
  using hiero::v3::LedgerState;
  using hiero::v3::MirrorAccountRequest;
  using hiero::v3::TransferRequest;

  auto state = std::make_shared<LedgerState>();
  auto consensus = std::make_shared<InMemoryConsensusTransport>(state);
  auto mirror = std::make_shared<InMemoryMirrorTransport>(state);

  consensus->setBalance("0.0.1001", 1000);
  consensus->setBalance("0.0.1002", 100);

  Client client(consensus, mirror, Client::Options{3});

  auto transferResult =
      client.transfer(TransferRequest{"0.0.1001", "0.0.1002", 250});
  if (!transferResult.ok()) {
    std::cerr << "Transfer failed: " << transferResult.error().message << '\n';
    return 1;
  }

  auto fromBalance = client.getBalance(BalanceRequest{"0.0.1001"});
  auto toBalance = client.getBalance(BalanceRequest{"0.0.1002"});
  auto mirrorAccount =
      client.getMirrorAccountAsync(MirrorAccountRequest{"0.0.1002"}).get();

  if (!fromBalance.ok() || !toBalance.ok() || !mirrorAccount.ok()) {
    std::cerr << "Query failed\n";
    return 1;
  }

  std::cout << "Transfer transactionId: "
            << transferResult.value().transactionId << '\n';
  std::cout << "0.0.1001 balance: " << fromBalance.value().balanceTinybar
            << '\n';
  std::cout << "0.0.1002 balance: " << toBalance.value().balanceTinybar << '\n';
  std::cout << "Mirror timestamp: "
            << mirrorAccount.value().snapshotConsensusTimestamp << '\n';

  return 0;
}
