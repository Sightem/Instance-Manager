#include <unordered_map>
#include <functional>
#include <future>
#include <mutex>
#include <vector>
#include <shared_mutex>
#include <queue>

class ThreadManager {
public:
    void submit_task(const std::string& id,
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
                            callbacks_map[id]();  // Call the callback
                            callbacks_map.erase(id);
                        }
                        tasks_map.erase(id);
                        termination_flags.erase(id);
                    }
                    })
            );

            cleanup_futures();
    }

    void submit_task(const std::string& id,
        std::function<void()> simpleTask,
        std::function<void()> callback = nullptr) {
        auto wrappedTask = [simpleTask](const std::atomic<bool>&) {
            simpleTask();
        };
        submit_task(id, wrappedTask, callback);
    }

    void terminate_task(const std::string& id) {
        std::shared_lock lock(smtx);
        if (termination_flags.find(id) != termination_flags.end()) {
            *termination_flags[id] = true;
        }
    }

    int get_completed_count(const std::string& id) const {
        std::shared_lock lock(smtx);
        if (tasks_map.find(id) != tasks_map.end()) {
            return tasks_map.at(id).completed;
        }
        return 0;
    }

    int get_total_count(const std::string& id) const {
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

    void cleanup_futures() {
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

    void submit_task(const std::string& taskId, const TaskFunc& task, const CallbackFunc& callback = nullptr) {
        std::lock_guard lock(mtx);

        // Add task to queue and map
        taskQueue.push(task);
        taskMap[taskId] = task;

        // If callback is provided, add it to the callback map
        if (callback) {
            callbackMap[taskId] = callback;
        }

        cv.notify_one(); // Notify the worker thread
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
                for (const auto& pair : taskMap) {
                    if (pair.second.target_type() == currentTask.target_type()) { // Comparing target types of std::functions
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
                    callbackMap.erase(currentTaskId); // Remove the callback
                }

                taskMap.erase(currentTaskId); // Remove the task
            }
        }
    }


};
