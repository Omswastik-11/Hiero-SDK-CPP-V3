#include "hiero/v3/keys.hpp"

#include <algorithm>
#include <sstream>
#include <iomanip>

namespace hiero::v3 {

PrivateKey::PrivateKey(Bytes seed) : m_seed(seed) {}

PrivateKey PrivateKey::generateForTest(uint64_t id) {
  Bytes seed{};
  // Fill bytes from the id value for deterministic test keys.
  for (size_t i = 0; i < sizeof(id) && i < kKeyLength; ++i) {
    seed[i] = static_cast<uint8_t>((id >> (i * 8U)) & 0xFFU);
  }
  return PrivateKey(seed);
}

std::vector<uint8_t> PrivateKey::sign(const std::vector<uint8_t> &data) const {
  // Simplified HMAC-style stub: XOR first 64 bytes of data with key.
  // A real implementation would call Ed25519 sign from OpenSSL / libsodium.
  constexpr size_t kSigLength = 64;
  std::vector<uint8_t> signature(kSigLength, 0);

  for (size_t i = 0; i < kSigLength; ++i) {
    uint8_t keyByte = m_seed[i % kKeyLength];
    uint8_t dataByte = (i < data.size()) ? data[i] : 0;
    signature[i] = keyByte ^ dataByte;
  }

  return signature;
}

std::string PrivateKey::toHex() const {
  std::ostringstream oss;
  for (auto byte : m_seed) {
    oss << std::hex << std::setfill('0') << std::setw(2)
        << static_cast<int>(byte);
  }
  return oss.str();
}

} // namespace hiero::v3
