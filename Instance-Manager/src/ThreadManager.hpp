#include <unordered_map>
#include <functional>
#include <future>
#include <mutex>
#include <vector>
#include <shared_mutex>

class ThreadManager {
public:
    void submit_task(const std::string& id, std::function<void()> task, std::function<void()> callback = nullptr) {
        {
            std::unique_lock lock(smtx);
            if (tasks_map.find(id) == tasks_map.end()) {
                tasks_map[id] = TaskInfo();
                if (callback) {
                    callbacks_map[id] = callback;
                }
            }
            tasks_map[id].total += 1;
        }

        futures.push_back(
            std::async(std::launch::async, [this, id, task]() {
                task();

                // Update the completed tasks count and check if we should call the callback
                std::unique_lock lock(smtx);
                tasks_map[id].completed += 1;
                if (tasks_map[id].total == tasks_map[id].completed && callbacks_map.find(id) != callbacks_map.end()) {
                    callbacks_map[id]();  // Call the callback
                }
            })
        );
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
    // Structure to represent task info
    struct TaskInfo {
        int total = 0;
        int completed = 0;
    };

    std::unordered_map<std::string, TaskInfo> tasks_map;
    std::unordered_map<std::string, std::function<void()>> callbacks_map;
    std::vector<std::future<void>> futures;
    mutable std::shared_mutex smtx;
};
