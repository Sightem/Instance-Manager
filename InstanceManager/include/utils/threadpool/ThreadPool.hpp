#pragma once
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

class ThreadPool {
public:
	void SubmitTask(const std::string& id,
	                std::function<void(const std::atomic<bool>&)> task,
	                std::function<void()> callback = nullptr) {
		{
			std::unique_lock lock(smtx);
			if (tasks_map.find(id) == tasks_map.end()) {
				tasks_map[id] = TaskInfo();
				if (callback) {
					callbacks_map[id] = callback;
				}
				termination_flags[id] = std::make_shared<std::atomic<bool>>(false);
			}
			tasks_map[id].total += 1;
		}

		auto flag = termination_flags[id];
		futures.push_back(
		        std::async(std::launch::async, [this, id, task, flag]() {
			        task(*flag);

			        std::unique_lock lock(smtx);
			        tasks_map[id].completed += 1;
			        if (tasks_map[id].total == tasks_map[id].completed) {
				        if (callbacks_map.find(id) != callbacks_map.end()) {
					        callbacks_map[id]();// Call the callback
					        callbacks_map.erase(id);
				        }
				        tasks_map.erase(id);
				        termination_flags.erase(id);
			        }
		        }));

		CleanupFutures();
	}

	void SubmitTask(const std::string& id,
	                std::function<void()> simpleTask,
	                std::function<void()> callback = nullptr) {
		auto wrappedTask = [simpleTask](const std::atomic<bool>&) {
			simpleTask();
		};
		SubmitTask(id, wrappedTask, callback);
	}

	void TerminateTask(const std::string& id) {
		std::shared_lock lock(smtx);
		if (termination_flags.find(id) != termination_flags.end()) {
			*termination_flags[id] = true;
		}
	}

	int GetCompletedCount(const std::string& id) const {
		std::shared_lock lock(smtx);
		if (tasks_map.find(id) != tasks_map.end()) {
			return tasks_map.at(id).completed;
		}
		return 0;
	}

	int GetTotalCount(const std::string& id) const {
		std::shared_lock lock(smtx);
		if (tasks_map.find(id) != tasks_map.end()) {
			return tasks_map.at(id).total;
		}
		return 0;
	}

private:
	struct TaskInfo {
		int total = 0;
		int completed = 0;
	};

	void CleanupFutures() {
		std::unique_lock lock(smtx);
		futures.erase(std::remove_if(futures.begin(), futures.end(),
		                             [](const std::future<void>& fut) {
			                             return fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
		                             }),
		              futures.end());
	}

	std::unordered_map<std::string, TaskInfo> tasks_map;
	std::unordered_map<std::string, std::function<void()>> callbacks_map;
	std::unordered_map<std::string, std::shared_ptr<std::atomic<bool>>> termination_flags;
	std::vector<std::future<void>> futures;
	mutable std::shared_mutex smtx;
};