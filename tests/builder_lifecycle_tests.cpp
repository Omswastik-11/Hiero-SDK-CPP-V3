#include <iostream>
#include <memory>
#include <string>

#include "hiero/v3/client.hpp"
#include "hiero/v3/executor.hpp"
#include "hiero/v3/in_memory_transport.hpp"
#include "hiero/v3/keys.hpp"
#include "hiero/v3/transfer_builder.hpp"

namespace {

int g_failures = 0;

void expect(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "[FAIL] " << message << '\n';
    ++g_failures;
  }
}

void testKeyGeneration() {
  auto key1 = hiero::v3::PrivateKey::generateForTest(1);
  auto key2 = hiero::v3::PrivateKey::generateForTest(2);
  auto key1Again = hiero::v3::PrivateKey::generateForTest(1);

  expect(key1.bytes() != key2.bytes(),
         "Different IDs should produce different keys");
  expect(key1.bytes() == key1Again.bytes(),
         "Same ID should produce the same key");
  expect(!key1.toHex().empty(), "Hex representation should not be empty");
}

void testKeySignature() {
  auto key = hiero::v3::PrivateKey::generateForTest(42);

  std::vector<uint8_t> data{0x01, 0x02, 0x03, 0x04};
  auto sig = key.sign(data);

  expect(sig.size() == 64, "Signature should be 64 bytes");

  // Same data should produce same signature.
  auto sig2 = key.sign(data);
  expect(sig == sig2, "Deterministic signing should produce same signature");
}

void testBuilderValidation_Empty() {
  auto result = hiero::v3::TransferTransactionBuilder().build();
  expect(!result.ok(), "Build with no transfers should fail");
  expect(result.error().code == hiero::v3::ErrorCode::kInvalidArgument,
         "Error should be kInvalidArgument");
}

void testBuilderValidation_NonZeroNet() {
  const hiero::v3::AccountId alice{0, 0, 100};

  auto result = hiero::v3::TransferTransactionBuilder()
                    .addTransfer(alice, 500)
                    .build();

  expect(!result.ok(),
         "Build with non-zero net amount should fail");
}

void testBuilderValidation_InvalidAccount() {
  const hiero::v3::AccountId invalid{0, 0, 0};
  const hiero::v3::AccountId valid{0, 0, 1};

  auto result = hiero::v3::TransferTransactionBuilder()
                    .addTransfer(invalid, -100)
                    .addTransfer(valid, 100)
                    .build();

  expect(!result.ok(), "Build with invalid account should fail");
}

void testBuilderSuccess() {
  const hiero::v3::AccountId alice{0, 0, 100};
  const hiero::v3::AccountId bob{0, 0, 200};

  auto result = hiero::v3::TransferTransactionBuilder()
                    .addHbarTransfer(alice, bob, 1000)
                    .build();

  expect(result.ok(), "Valid build should succeed");

  if (result.ok()) {
    expect(result.value().transactionType() == "CryptoTransfer",
           "Transaction type should be CryptoTransfer");
    expect(!result.value().body().empty(),
           "Serialized body should not be empty");
    expect(!result.value().isSigned(),
           "Newly built transaction should not be signed");
  }
}

void testSignAndExecuteLifecycle() {
  auto state = std::make_shared<hiero::v3::LedgerState>();
  auto consensus =
      std::make_shared<hiero::v3::InMemoryConsensusTransport>(state);
  auto mirror = std::make_shared<hiero::v3::InMemoryMirrorTransport>(state);

  const hiero::v3::AccountId alice{0, 0, 7001};
  const hiero::v3::AccountId bob{0, 0, 7002};

  consensus->setBalance(alice, 5000);
  consensus->setBalance(bob, 0);

  hiero::v3::Client client(consensus, mirror, hiero::v3::Client::Options{3},
                           std::make_shared<hiero::v3::InlineExecutor>());

  auto operatorKey = hiero::v3::PrivateKey::generateForTest(99);
  client.setOperator({0, 0, 2}, operatorKey);

  // Build.
  auto buildResult = hiero::v3::TransferTransactionBuilder()
                         .addHbarTransfer(alice, bob, 2000)
                         .build();

  expect(buildResult.ok(), "Build should succeed");
  if (!buildResult.ok()) return;

  auto &tx = buildResult.value();

  // Sign.
  tx.sign(operatorKey);
  expect(tx.isSigned(), "Transaction should be signed after sign()");

  // Execute.
  auto execResult = tx.execute(client);
  expect(execResult.ok(), "Execute should succeed");

  // Verify balances.
  auto aliceBalance = client.getBalance({alice});
  auto bobBalance = client.getBalance({bob});

  expect(aliceBalance.ok() && aliceBalance.value().balanceTinybar == 3000,
         "Alice balance should be 3000 after transfer");
  expect(bobBalance.ok() && bobBalance.value().balanceTinybar == 2000,
         "Bob balance should be 2000 after transfer");
}

void testExecuteWithoutSignFails() {
  auto state = std::make_shared<hiero::v3::LedgerState>();
  auto consensus =
      std::make_shared<hiero::v3::InMemoryConsensusTransport>(state);
  auto mirror = std::make_shared<hiero::v3::InMemoryMirrorTransport>(state);

  const hiero::v3::AccountId alice{0, 0, 8001};
  const hiero::v3::AccountId bob{0, 0, 8002};

  consensus->setBalance(alice, 1000);
  consensus->setBalance(bob, 0);

  hiero::v3::Client client(consensus, mirror, hiero::v3::Client::Options{1},
                           std::make_shared<hiero::v3::InlineExecutor>());

  auto buildResult = hiero::v3::TransferTransactionBuilder()
                         .addHbarTransfer(alice, bob, 500)
                         .build();

  expect(buildResult.ok(), "Build should succeed");
  if (!buildResult.ok()) return;

  auto &tx = buildResult.value();

  // Execute without signing — should fail.
  auto execResult = tx.execute(client);
  expect(!execResult.ok(),
         "Execute without signing should fail");
  expect(execResult.error().code == hiero::v3::ErrorCode::kInvalidArgument,
         "Error should be kInvalidArgument for unsigned tx");
}

void testMultipleSignatures() {
  auto key1 = hiero::v3::PrivateKey::generateForTest(1);
  auto key2 = hiero::v3::PrivateKey::generateForTest(2);

  const hiero::v3::AccountId alice{0, 0, 9001};
  const hiero::v3::AccountId bob{0, 0, 9002};

  auto buildResult = hiero::v3::TransferTransactionBuilder()
                         .addHbarTransfer(alice, bob, 100)
                         .build();

  expect(buildResult.ok(), "Build should succeed");
  if (!buildResult.ok()) return;

  auto &tx = buildResult.value();
  tx.sign(key1);
  tx.sign(key2);

  expect(tx.signatures().size() == 2,
         "Transaction should have exactly 2 signatures");
  expect(tx.signatures()[0] != tx.signatures()[1],
         "Different keys should produce different signatures");
}

void testOperatorAccountSetup() {
  auto state = std::make_shared<hiero::v3::LedgerState>();
  auto consensus =
      std::make_shared<hiero::v3::InMemoryConsensusTransport>(state);
  auto mirror = std::make_shared<hiero::v3::InMemoryMirrorTransport>(state);

  hiero::v3::Client client(consensus, mirror, hiero::v3::Client::Options{1},
                           std::make_shared<hiero::v3::InlineExecutor>());

  expect(!client.hasOperator(), "Client should not have operator initially");

  auto key = hiero::v3::PrivateKey::generateForTest(10);
  client.setOperator({0, 0, 2}, key);

  expect(client.hasOperator(), "Client should have operator after setOperator");
  expect(client.operatorAccount()->accountId.num == 2,
         "Operator account num should be 2");
}

} // namespace

int main() {
  testKeyGeneration();
  testKeySignature();
  testBuilderValidation_Empty();
  testBuilderValidation_NonZeroNet();
  testBuilderValidation_InvalidAccount();
  testBuilderSuccess();
  testSignAndExecuteLifecycle();
  testExecuteWithoutSignFails();
  testMultipleSignatures();
  testOperatorAccountSetup();

  if (g_failures > 0) {
    std::cerr << "Builder/signing test failures: " << g_failures << '\n';
    return 1;
  }

  std::cout << "All builder and signing lifecycle tests passed\n";
  return 0;
}
