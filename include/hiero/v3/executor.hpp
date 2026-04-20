#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace hiero::v3 {

class IExecutor {
public:
  virtual ~IExecutor() = default;

  virtual void execute(std::function<void()> task) = 0;
};

class InlineExecutor final : public IExecutor {
public:
  void execute(std::function<void()> task) override { task(); }
};

class SingleThreadExecutor final : public IExecutor {
public:
  SingleThreadExecutor();
  ~SingleThreadExecutor() override;

  SingleThreadExecutor(const SingleThreadExecutor &) = delete;
  SingleThreadExecutor &operator=(const SingleThreadExecutor &) = delete;

  void execute(std::function<void()> task) override;

private:
  void run();

  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::queue<std::function<void()>> m_tasks;
  bool m_stopping = false;
  std::thread m_worker;
};

} // namespace hiero::v3
