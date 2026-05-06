#pragma once

#include <string>
#include <vector>

#include "hiero/v3/keys.hpp"
#include "hiero/v3/result.hpp"
#include "hiero/v3/types.hpp"

namespace hiero::v3 {

class Client;

// An immutable transaction that has been built and is ready to sign + execute.
// The ResponseT template keeps the response type visible through the whole
// lifecycle so we don't need unsafe casts anywhere.
template <typename ResponseT>
class BuiltTransaction {
public:
  BuiltTransaction(std::string transactionType,
                   std::vector<uint8_t> serializedBody)
      : m_transactionType(std::move(transactionType)),
        m_serializedBody(std::move(serializedBody)) {}

  /// Sign the transaction with a signer function.
  /// Multiple signers can be added by calling sign() multiple times.
  BuiltTransaction &sign(const SignerFunction &signer) {
    auto signature = signer(m_serializedBody);
    m_signatures.push_back(std::move(signature));
    return *this;
  }

  /// Sign the transaction with a PrivateKey directly.
  BuiltTransaction &sign(const PrivateKey &key) {
    return sign(makeSigner(key));
  }

  /// Check whether the transaction has been signed at least once.
  [[nodiscard]] bool isSigned() const noexcept {
    return !m_signatures.empty();
  }

  /// Execute the signed transaction against a Client and return a typed result.
  [[nodiscard]] Result<ResponseT> execute(Client &client);

  /// Return the transaction type name (for diagnostics).
  [[nodiscard]] const std::string &transactionType() const noexcept {
    return m_transactionType;
  }

  /// Return a reference to the serialized body bytes.
  [[nodiscard]] const std::vector<uint8_t> &body() const noexcept {
    return m_serializedBody;
  }

  /// Return the collected signatures.
  [[nodiscard]] const std::vector<std::vector<uint8_t>> &signatures() const noexcept {
    return m_signatures;
  }

private:
  std::string m_transactionType;
  std::vector<uint8_t> m_serializedBody;
  std::vector<std::vector<uint8_t>> m_signatures;
};

} // namespace hiero::v3
