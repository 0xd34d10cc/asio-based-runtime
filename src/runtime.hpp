#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <thread>

#include <asio/io_context.hpp>
#include <asio/io_context_strand.hpp>
#include <asio/basic_waitable_timer.hpp>
#include <asio/post.hpp>

#include "task_queue_interface.hpp"

class Runtime {
 public:
  using TaskFn = TaskQueueInterface::TaskFn;
  using Clock = TaskQueueInterface::Clock;
  using TimePoint = TaskQueueInterface::TimePoint;
  using Timer = asio::basic_waitable_timer<Clock>;

  Runtime(std::size_t n_threads = 0)
      : m_context(std::make_shared<asio::io_context>()) {
    if (n_threads == 0) {
      n_threads = std::thread::hardware_concurrency();
    }

    assert(n_threads > 0);
    m_threads.resize(n_threads - 1);
  }

  void run() {
    for (auto& thread : m_threads) {
      assert(!thread.joinable());
      thread = std::thread([this] { do_work(); });
    }

    do_work();

    for (auto& thread : m_threads) {
      thread.join();
    }
  }

  void stop() { m_context->stop(); }

  void run_soon(TaskFn task) { asio::post(*m_context, std::move(task)); }

  void run_at(TimePoint at, TaskFn task) {
    auto timer = std::make_unique<Timer>(*m_context, at);
    auto* ptr = timer.get();
    auto handler = [t = std::move(timer), f = std::move(task)](const auto& ec) {
      if (ec) {
        // operation aborted
        return;
      }

      f();
    };

    ptr->async_wait(std::move(handler));
  }

  std::shared_ptr<TaskQueueInterface> create_queue() {
    class TaskQueue : public TaskQueueInterface,
                      public std::enable_shared_from_this<TaskQueue> {
     public:
      TaskQueue(std::shared_ptr<asio::io_context> context)
          : m_context(std::move(context)), m_strand(*m_context) {}

      void run_soon(TaskFn task) override {
        asio::post(m_strand, std::move(task));
      }

      void run_at(TimePoint at, TaskFn task) override {
        auto timer = std::make_unique<Timer>(*m_context, at);
        auto* ptr = timer.get();

        std::weak_ptr<TaskQueue> self = shared_from_this();
        auto handler = [t = std::move(timer), f = std::move(task),
                        s = std::move(self)](const auto& ec) {
          if (ec) {
            // operation aborted
            return;
          }

          if (auto q = s.lock()) {
            asio::post(q->m_strand, std::move(f));
          }
        };

        ptr->async_wait(std::move(handler));
      }

     private:
      std::shared_ptr<asio::io_context> m_context;
      asio::io_context::strand m_strand;
    };

    return std::make_shared<TaskQueue>(m_context);
  }

 private:
  void do_work() {
    using WorkGuard =
        asio::executor_work_guard<asio::io_context::executor_type>;
    WorkGuard work(m_context->get_executor());
    m_context->run();
  }

  std::shared_ptr<asio::io_context> m_context;
  std::vector<std::thread> m_threads;
};