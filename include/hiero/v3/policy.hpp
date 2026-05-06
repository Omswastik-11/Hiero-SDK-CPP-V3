#pragma once

#include <chrono>
#include <cstdint>

namespace hiero::v3 {

// Configurable retry and backoff settings for the V3 runtime.
// Unlike V2, these are explicitly visible to the developer.
struct RetryPolicy {
  /// Maximum number of retry attempts.
  int maxAttempts = 3;

  /// Initial backoff delay after the first failure.
  std::chrono::milliseconds initialBackoff{250};

  /// Maximum backoff delay (caps exponential growth).
  std::chrono::milliseconds maxBackoff{8000};

  /// Multiplier applied to backoff on each retry (exponential backoff).
  double backoffMultiplier = 2.0;

  /// Overall deadline for the entire operation (0 = no deadline).
  std::chrono::milliseconds deadline{0};
};

/// Default policy used when none is specified.
inline RetryPolicy defaultRetryPolicy() { return RetryPolicy{}; }

} // namespace hiero::v3
