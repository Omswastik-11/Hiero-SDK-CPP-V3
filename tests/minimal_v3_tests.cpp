#include <iostream>
#include <memory>
#include <string>

#include "hiero/v3/client.hpp"
#include "hiero/v3/executor.hpp"
#include "hiero/v3/in_memory_transport.hpp"
#include "hiero/v3/transport.hpp"

namespace {

int g_failures = 0;

void expect(bool condition, const std::string &message) {
  if (!condition) {
    std::cerr << "[FAIL] " << message << '\n';
    ++g_failures;
  }
}

class FlakyConsensusTransport final : public hiero::v3::IConsensusTransport {
public:
  FlakyConsensusTransport(std::shared_ptr<hiero::v3::IConsensusTransport> inner,
                          int failuresBeforeSuccess)
      : m_inner(std::move(inner)),
        m_failuresBeforeSuccess(failuresBeforeSuccess) {}

  hiero::v3::Result<hiero::v3::TransferReceipt>
  submitTransfer(const hiero::v3::TransferRequest &request) override {
    ++m_attempts;

    if (m_attempts <= m_failuresBeforeSuccess) {
      return hiero::v3::Result<hiero::v3::TransferReceipt>::Fail(
          hiero::v3::ErrorCode::kNetwork, "Transient network error");
    }

    return m_inner->submitTransfer(request);
  }

  hiero::v3::Result<hiero::v3::BalanceResponse>
  getBalance(const hiero::v3::BalanceRequest &request) override {
    return m_inner->getBalance(request);
  }

  int attempts() const { return m_attempts; }

private:
  std::shared_ptr<hiero::v3::IConsensusTransport> m_inner;
  int m_failuresBeforeSuccess;
  int m_attempts = 0;
};

void testSuccessfulTransferAndQueries() {
  auto state = std::make_shared<hiero::v3::LedgerState>();
  auto consensus =
      std::make_shared<hiero::v3::InMemoryConsensusTransport>(state);
  auto mirror = std::make_shared<hiero::v3::InMemoryMirrorTransport>(state);

  const hiero::v3::AccountId accountA{0, 0, 2001};
  const hiero::v3::AccountId accountB{0, 0, 2002};

  consensus->setBalance(accountA, 1000);
  consensus->setBalance(accountB, 200);

  hiero::v3::Client client(consensus, mirror, hiero::v3::Client::Options{3},
                           std::make_shared<hiero::v3::InlineExecutor>());

  auto transfer = client.transfer({accountA, accountB, 300});
  expect(transfer.ok(), "transfer should succeed");

  auto fromBalance = client.getBalance({accountA});
  auto toBalance = client.getBalance({accountB});
  auto mirrorView = client.getMirrorAccount({accountB});

  expect(fromBalance.ok(), "from account balance query should succeed");
  expect(toBalance.ok(), "to account balance query should succeed");
  expect(mirrorView.ok(), "mirror account query should succeed");

  if (fromBalance.ok()) {
    expect(fromBalance.value().balanceTinybar == 700,
           "from account balance should be debited");
  }

  if (toBalance.ok()) {
    expect(toBalance.value().balanceTinybar == 500,
           "to account balance should be credited");
  }
}

void testInsufficientBalance() {
  auto state = std::make_shared<hiero::v3::LedgerState>();
  auto consensus =
      std::make_shared<hiero::v3::InMemoryConsensusTransport>(state);
  auto mirror = std::make_shared<hiero::v3::InMemoryMirrorTransport>(state);

  const hiero::v3::AccountId accountA{0, 0, 3001};
  const hiero::v3::AccountId accountB{0, 0, 3002};

  consensus->setBalance(accountA, 10);
  consensus->setBalance(accountB, 0);

  hiero::v3::Client client(consensus, mirror, hiero::v3::Client::Options{2},
                           std::make_shared<hiero::v3::InlineExecutor>());

  auto result = client.transfer({accountA, accountB, 999});
  expect(!result.ok(),
         "transfer should fail when amount exceeds source balance");

  if (!result.ok()) {
    expect(result.error().code == hiero::v3::ErrorCode::kInsufficientBalance,
           "error code should be insufficient balance");
  }
}

void testAsyncTransfer() {
  auto state = std::make_shared<hiero::v3::LedgerState>();
  auto consensus =
      std::make_shared<hiero::v3::InMemoryConsensusTransport>(state);
  auto mirror = std::make_shared<hiero::v3::InMemoryMirrorTransport>(state);

  const hiero::v3::AccountId accountA{0, 0, 4001};
  const hiero::v3::AccountId accountB{0, 0, 4002};

  consensus->setBalance(accountA, 50);
  consensus->setBalance(accountB, 10);

  hiero::v3::Client client(consensus, mirror);

  auto future = client.transferAsync({accountA, accountB, 25});
  auto result = future.get();
  expect(result.ok(), "async transfer should succeed");

  auto updated = client.getBalance({accountB});
  if (updated.ok()) {
    expect(updated.value().balanceTinybar == 35,
           "destination account should reflect async transfer");
  }
}

void testRetryOnNetworkFailure() {
  auto state = std::make_shared<hiero::v3::LedgerState>();
  auto consensusBase =
      std::make_shared<hiero::v3::InMemoryConsensusTransport>(state);
  auto flakyConsensus =
      std::make_shared<FlakyConsensusTransport>(consensusBase, 1);
  auto mirror = std::make_shared<hiero::v3::InMemoryMirrorTransport>(state);

  const hiero::v3::AccountId accountA{0, 0, 5001};
  const hiero::v3::AccountId accountB{0, 0, 5002};

  consensusBase->setBalance(accountA, 100);
  consensusBase->setBalance(accountB, 0);

  hiero::v3::Client client(flakyConsensus, mirror,
                           hiero::v3::Client::Options{3},
                           std::make_shared<hiero::v3::InlineExecutor>());

  auto result = client.transfer({accountA, accountB, 20});
  expect(result.ok(), "transfer should succeed after one retriable failure");
  expect(flakyConsensus->attempts() == 2,
         "retriable path should attempt transfer twice");
}

} // namespace

int main() {
  testSuccessfulTransferAndQueries();
  testInsufficientBalance();
  testAsyncTransfer();
  testRetryOnNetworkFailure();

  if (g_failures > 0) {
    std::cerr << "Test failures: " << g_failures << '\n';
    return 1;
  }

  std::cout << "All minimal V3 prototype tests passed\n";
  return 0;
}
