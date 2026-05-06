#include <iostream>
#include <memory>

#include "hiero/v3/client.hpp"
#include "hiero/v3/in_memory_transport.hpp"
#include "hiero/v3/keys.hpp"
#include "hiero/v3/transfer_builder.hpp"

int main() {
  using hiero::v3::AccountId;
  using hiero::v3::Client;
  using hiero::v3::InMemoryConsensusTransport;
  using hiero::v3::InMemoryMirrorTransport;
  using hiero::v3::LedgerState;
  using hiero::v3::PrivateKey;
  using hiero::v3::TransferTransactionBuilder;

  // 1. Set up accounts and state.
  const AccountId operatorId{0, 0, 2};
  const AccountId alice{0, 0, 1001};
  const AccountId bob{0, 0, 1002};

  auto state = std::make_shared<LedgerState>();
  auto consensus = std::make_shared<InMemoryConsensusTransport>(state);
  auto mirror = std::make_shared<InMemoryMirrorTransport>(state);

  consensus->setBalance(alice, 5000);
  consensus->setBalance(bob, 1000);

  // 2. Create client with operator.
  Client client(consensus, mirror, Client::Options{3});

  auto operatorKey = PrivateKey::generateForTest(42);
  client.setOperator(operatorId, operatorKey);

  std::cout << "Operator: " << ToString(operatorId)
            << "  key: " << operatorKey.toHex().substr(0, 16) << "...\n\n";

  // 3. Build a transfer transaction using the builder pattern.
  auto buildResult = TransferTransactionBuilder()
                         .addHbarTransfer(alice, bob, 1500)
                         .setMemo("Demo transfer via builder")
                         .build();

  if (!buildResult.ok()) {
    std::cerr << "Build failed: " << buildResult.error().message << '\n';
    return 1;
  }

  auto &tx = buildResult.value();

  std::cout << "Transaction type: " << tx.transactionType() << '\n';
  std::cout << "Body size: " << tx.body().size() << " bytes\n";

  // 4. Sign the transaction.
  tx.sign(operatorKey);

  std::cout << "Signed: " << (tx.isSigned() ? "yes" : "no") << '\n';
  std::cout << "Signature count: " << tx.signatures().size() << '\n';
  std::cout << "Signature[0] size: " << tx.signatures()[0].size()
            << " bytes\n\n";

  // 5. Execute against the client.
  auto result = tx.execute(client);

  if (!result.ok()) {
    std::cerr << "Execute failed: " << result.error().message << '\n';
    return 1;
  }

  std::cout << "Transfer transactionId: " << result.value().transactionId
            << '\n';

  // 6. Verify balances.
  auto aliceBalance = client.getBalance({alice});
  auto bobBalance = client.getBalance({bob});

  if (aliceBalance.ok()) {
    std::cout << ToString(alice)
              << " balance: " << aliceBalance.value().balanceTinybar << '\n';
  }

  if (bobBalance.ok()) {
    std::cout << ToString(bob)
              << " balance: " << bobBalance.value().balanceTinybar << '\n';
  }

  std::cout << "\nFull lifecycle (build -> sign -> execute) completed.\n";
  return 0;
}
