#pragma once

#include <function2/function2.hpp>

namespace exec {

// std::move_only_function should also work.
using Task = fu2::unique_function<void()>;

struct IExecutor {
    virtual ~IExecutor() = default;

    virtual void Submit(Task task) = 0;
};

}  // namespace exec
