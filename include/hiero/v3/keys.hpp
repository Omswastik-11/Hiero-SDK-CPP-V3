#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace hiero::v3 {

// Simplified Ed25519 private key for the prototype.
// In production this would use OpenSSL or libsodium.
class PrivateKey final {
public:
  static constexpr size_t kKeyLength = 32;
  using Bytes = std::array<uint8_t, kKeyLength>;

  /// Construct from raw 32-byte seed.
  explicit PrivateKey(Bytes seed);

  /// Generate a deterministic test key from an integer id.
  static PrivateKey generateForTest(uint64_t id);

  /// Sign arbitrary data and return a 64-byte signature stub.
  [[nodiscard]] std::vector<uint8_t> sign(const std::vector<uint8_t> &data) const;

  /// Return the raw bytes of the key seed.
  [[nodiscard]] const Bytes &bytes() const noexcept { return m_seed; }

  /// Return a hex-encoded representation (for debugging).
  [[nodiscard]] std::string toHex() const;

private:
  Bytes m_seed{};
};

// Callback type that signs arbitrary data and returns a signature.
using SignerFunction = std::function<std::vector<uint8_t>(const std::vector<uint8_t> &)>;

// Helper: wraps a PrivateKey into a SignerFunction.
inline SignerFunction makeSigner(const PrivateKey &key) {
  return [key](const std::vector<uint8_t> &data) { return key.sign(data); };
}

} // namespace hiero::v3
