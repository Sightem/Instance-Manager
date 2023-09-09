#include "utils/threadpool/ThreadPool.hpp"

ThreadPool::ThreadPool(size_t num_threads) {
	for (size_t i = 0; i < num_threads; ++i) {
		workers.emplace_back([this] {
			while (true) {
				std::optional<TaskWrapper> task_opt;

				{
					std::unique_lock<std::mutex> lock(this->queue_mutex);

					this->condition.wait(lock, [this] {
						return this->stop ||
						       (!this->tasks.empty());
					});

					if (this->stop && this->tasks.empty()) return;

					task_opt = std::move(this->tasks.front());
					this->tasks.pop();
				}
				if (task_opt) {
					(*task_opt)();
				}
			}
		});
	}
}

ThreadPool::~ThreadPool() {
	{
		std::scoped_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}
	condition.notify_all();
	for (std::thread& worker: workers) {
		worker.join();
	}
}