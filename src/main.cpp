#include <iostream>
#include <memory>
#include <atomic>

#include "runtime.hpp"

static std::atomic<std::uint64_t> counter{0};
constexpr std::size_t n_tasks = 100'000;

using Clock = TaskQueueInterface::Clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;
using TaskFn = TaskQueueInterface::TaskFn;

static void run_periodic_at(TimePoint at, Duration period,
                            Runtime& rt, TaskFn task) {
  const auto next = at + period;
  rt.run_at(at, [&rt, next, period, t = std::move(task)] () mutable {
    t();
    run_periodic_at(next, period, rt, std::move(t));
  });
}

static void run_periodic(Duration period,
                         Runtime& rt, TaskFn task) {
  run_periodic_at(Clock::now(), period, rt, std::move(task));
}

void start_timers(Runtime& rt) {
  for (std::size_t i = 0; i < n_tasks; ++i) {
    run_periodic(std::chrono::seconds(1), rt, [] { ++counter; });
  }
}

void do_increment(Runtime& rt) {
  rt.run_soon([&rt] {
    ++counter;
    do_increment(rt);
  });
}

void start_immediate_tasks(Runtime& rt) {
  for (std::size_t i = 0; i < n_tasks; ++i) {
    do_increment(rt);
  }
}

int main() {
  std::iostream::sync_with_stdio(false);

  Runtime runtime;
  Clock::time_point start;
  Clock::time_point end;

  std::uint64_t counted = 0;
  runtime.run_soon([&runtime, &start, &end, &counted] {
    start_timers(runtime);
    start = Clock::now();
    counter = 0;

    runtime.run_at(start + std::chrono::seconds(10),
                   [&runtime, &counted, &end] {
                     end = Clock::now();
                     counted = counter;
                     runtime.stop();
                   });
  });

  runtime.run();
  const auto seconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count() /
      1000.0;

  std::cout << "Ran for " << seconds << "s" << std::endl;
  std::cout << counted / seconds << " tasks/s" << std::endl;
  return 0;
}