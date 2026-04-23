#pragma once

#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

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

  [[nodiscard]] bool ok() const noexcept {
    return std::holds_alternative<T>(m_state);
  }

  explicit operator bool() const noexcept { return ok(); }

  const T &value() const {
    if (!ok()) {
      throw std::logic_error("Attempted to access value() on failed Result");
    }

    return std::get<T>(m_state);
  }

  T &value() {
    if (!ok()) {
      throw std::logic_error("Attempted to access value() on failed Result");
    }

    return std::get<T>(m_state);
  }

  const Error &error() const {
    if (ok()) {
      throw std::logic_error(
          "Attempted to access error() on successful Result");
    }

    return std::get<Error>(m_state);
  }

private:
  explicit Result(T value) : m_state(std::move(value)) {}

  explicit Result(Error error) : m_state(std::move(error)) {}

  std::variant<T, Error> m_state;
};

} // namespace hiero::v3
