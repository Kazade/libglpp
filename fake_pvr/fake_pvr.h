#pragma once

#include <memory>
#include <vector>
#include <utility>

struct BaseCallLogEntry {
    std::string name;
};

template<typename... Args>
struct CallLogEntry : public BaseCallLogEntry {
    CallLogEntry(const std::string& name, const Args&& args) {
        this->name = name;
        this->args = std::make_tuple(args);
    }

    std::tuple<Args...> args;
};

class CallLog {
    template<typename... Args>
    static bool check_call(
        uint32_t which, const std::string& expected_name, Args&& expected_args) {

        // See if the call at that point is the same type
        auto call = (which < calls_.size()) ? calls_[which] : nullptr;
        auto existing = std::dynamic_pointer_cast<CallLogEntry<Args>>(call);

        // If not (or it doesn't exist), return false
        if(!existing) {
            return false;
        }

        // Compare
        return expected_name == existing->name &&
                compare_tuples(existing->args, std::make_tuple(expected_args));
    }

    template<typename... Args>
    static void log(const std::string& expected_name, const Args&& expected_args) {
        auto call = std::make_shared<CallLogEntry<Args>(expected_name, expected_args);
        calls_.push_back(call);
    }

    static void reset() {
        calls_.clear();
    }

    static uint32_t count(const std::string& func) {
        std::count_if(calls_.begin(), calls_.end(), [](const std::shared_ptr<BaseCallLogEntry>& call) -> bool {
            return call->name == func;
        });
    }

private:
    template<typename T>
    bool compare_tuples(const T& lhs, const T& rhs) {
        return lhs == rhs;
    }

    template<typename T, typename U>
    bool compare_tuples(const T& lhs, const U& rhs) {
        // Always false if the types are different
        return false;
    }

    static std::vector<std::shared_ptr<BaseCallLogEntry>> calls_;
};


template <typename R, typename... Args>
struct FakeFunction {
    std::string name;

    FakeFunction(const std::string& name):
        name(name) {}

    R operator()(Args&& args...) {
        CallLog::log(name, std::forward<Args>(args)...);
    }
};

