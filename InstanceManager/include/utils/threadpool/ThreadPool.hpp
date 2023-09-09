#pragma once
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

using Callback = std::function<void()>;


class TaskWrapper {
	struct ITask {
		virtual ~ITask() = default;
		virtual void execute() = 0;
	};

	template<typename Func>
	struct TaskHolder : ITask {
		explicit TaskHolder(Func&& func) : func_(std::move(func)) {}
		void execute() override { func_(); }
		Func func_;
	};

	std::unique_ptr<ITask> task_;

public:
	template<class Func>
	explicit TaskWrapper(Func&& func) {
		task_ = std::make_unique<TaskHolder<Func>>(std::move(func));
	}

	void operator()() {
		task_->execute();
	}
};

class ThreadPool {
public:
	explicit ThreadPool(size_t num_threads);

	~ThreadPool();

	template<class F, class... Args>
	auto SubmitTask(Callback cb, F&& f, Args&&... args)
	        -> std::future<std::invoke_result_t<F, Args...>>;

	template<class F, class... Args>
	auto SubmitTask(F&& f, Args&&... args)
	        -> std::future<std::invoke_result_t<F, Args...>> {
		return SubmitTask(nullptr, std::forward<F>(f), std::forward<Args>(args)...);
	}

private:
	std::vector<std::thread> workers;
	std::queue<TaskWrapper> tasks;

	std::mutex queue_mutex;
	std::condition_variable condition;
	bool stop = false;
};

template<class F, class... Args>
auto ThreadPool::SubmitTask(Callback cb, F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
	using return_type = std::invoke_result_t<F, Args...>;

	auto args_tuple = std::tuple(std::forward<Args>(args)...);

	auto task_function = [f = std::forward<F>(f), cb, args_tuple = std::tuple{std::forward<Args>(args)...}]() -> return_type {
		if constexpr (std::is_same_v<return_type, void>) {
			std::apply(f, args_tuple);
			if (cb) cb();
		} else {
			auto result = std::apply(f, args_tuple);
			if (cb) cb();
			return result;
		}
	};


	std::packaged_task<return_type()> packaged(std::move(task_function));
	std::future<return_type> result = packaged.get_future();

	{
		std::scoped_lock<std::mutex> lock(queue_mutex);
		if (stop) {
			throw std::runtime_error("SubmitTask on stopped ThreadPool");
		}
		tasks.emplace(std::move(packaged));
	}

	condition.notify_one();
	return result;
}