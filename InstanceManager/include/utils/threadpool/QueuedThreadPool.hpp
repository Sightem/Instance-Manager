#pragma once
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

class QueuedThreadManager {
public:
	using TaskFunc = std::function<void()>;
	using CallbackFunc = std::function<void()>;

	// Constructor
	QueuedThreadManager() : isRunning(true) {
		workerThread = std::thread(&QueuedThreadManager::run, this);
	}

	// Destructor
	~QueuedThreadManager() {
		isRunning = false;
		cv.notify_all();
		if (workerThread.joinable()) {
			workerThread.join();
		}
	}

	void SubmitTask(const std::string& taskId, const TaskFunc& task, const CallbackFunc& callback = nullptr) {
		std::lock_guard lock(mtx);

		// Add task to queue and map
		taskQueue.push(task);
		taskMap[taskId] = task;

		// If callback is provided, add it to the callback map
		if (callback) {
			callbackMap[taskId] = callback;
		}

		cv.notify_one();// Notify the worker thread
	}


private:
	std::queue<TaskFunc> taskQueue;
	std::unordered_map<std::string, TaskFunc> taskMap;
	std::unordered_map<std::string, CallbackFunc> callbackMap;
	std::thread workerThread;
	std::mutex mtx;
	std::condition_variable cv;
	bool isRunning;

	void run() {
		while (isRunning) {
			TaskFunc currentTask;
			std::string currentTaskId;

			{
				std::unique_lock lock(mtx);

				// Wait for a task to be available or for the thread to stop
				cv.wait(lock, [this]() {
					return !taskQueue.empty() || !isRunning;
				});

				// If no tasks and thread should stop, break
				if (taskQueue.empty() && !isRunning) {
					break;
				}

				// Get the next task
				currentTask = taskQueue.front();
				taskQueue.pop();

				// Find the task's ID from the task map (this is inefficient but simplifies the design for now)
				for (const auto& pair: taskMap) {
					if (pair.second.target_type() == currentTask.target_type()) {// Comparing target types of std::functions
						currentTaskId = pair.first;
						break;
					}
				}
			}

			// Execute the task
			if (currentTask) {
				currentTask();
			}

			// Execute the callback if it exists
			{
				std::lock_guard<std::mutex> lock(mtx);
				if (callbackMap.find(currentTaskId) != callbackMap.end()) {
					callbackMap[currentTaskId]();
					callbackMap.erase(currentTaskId);// Remove the callback
				}

				taskMap.erase(currentTaskId);// Remove the task
			}
		}
	}
};
