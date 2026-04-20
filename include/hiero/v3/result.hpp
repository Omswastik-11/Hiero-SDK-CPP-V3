#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace hiero::v3 {

enum class ErrorCode {
  kInvalidArgument,
  kInsufficientBalance,
  kNotFound,
  kNetwork,
  kInternal
};

struct Error {
  ErrorCode code;
  std::string message;
};

template <typename T> class Result {
public:
  static Result Ok(T value) { return Result(std::move(value)); }

  static Result Fail(ErrorCode code, std::string message) {
    return Result(Error{code, std::move(message)});
  }

  bool ok() const noexcept { return m_ok; }

  explicit operator bool() const noexcept { return ok(); }

  const T &value() const {
    if (!m_ok) {
      throw std::logic_error("Attempted to access value() on failed Result");
    }

    return *m_value;
  }

  T &value() {
    if (!m_ok) {
      throw std::logic_error("Attempted to access value() on failed Result");
    }

    return *m_value;
  }

  const Error &error() const {
    if (m_ok) {
      throw std::logic_error(
          "Attempted to access error() on successful Result");
    }

    return *m_error;
  }

private:
  explicit Result(T value)
      : m_ok(true), m_value(std::move(value)), m_error(std::nullopt) {}

  explicit Result(Error error)
      : m_ok(false), m_value(std::nullopt), m_error(std::move(error)) {}

  bool m_ok;
  std::optional<T> m_value;
  std::optional<Error> m_error;
};

} // namespace hiero::v3
