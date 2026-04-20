#include "hiero/v3/executor.hpp"

#include <stdexcept>

namespace hiero::v3 {

SingleThreadExecutor::SingleThreadExecutor() : m_worker([this]() { run(); }) {}

SingleThreadExecutor::~SingleThreadExecutor() {
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stopping = true;
  }

  m_cv.notify_one();

  if (m_worker.joinable()) {
    m_worker.join();
  }
}

void SingleThreadExecutor::execute(std::function<void()> task) {
  {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_stopping) {
      throw std::runtime_error("Cannot schedule work on a stopped executor");
    }

    m_tasks.push(std::move(task));
  }

  m_cv.notify_one();
}

void SingleThreadExecutor::run() {
  for (;;) {
    std::function<void()> task;

    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cv.wait(lock, [this]() { return m_stopping || !m_tasks.empty(); });

      if (m_stopping && m_tasks.empty()) {
        return;
      }

      task = std::move(m_tasks.front());
      m_tasks.pop();
    }

    try {
      task();
    } catch (...) {
      // Keep executor alive even if a task fails.
    }
  }
}

} // namespace hiero::v3
