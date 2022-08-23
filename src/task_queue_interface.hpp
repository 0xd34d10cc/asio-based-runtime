#pragma once

#include <functional>
#include <chrono>

class TaskQueueInterface {
 public:
  using TaskFn = std::function<void()>;
  using Clock = std::chrono::system_clock;
  using TimePoint = Clock::time_point;

  virtual ~TaskQueueInterface() = default;

  virtual void run_soon(TaskFn task) = 0;
  virtual void run_at(TimePoint at, TaskFn task) = 0;
};