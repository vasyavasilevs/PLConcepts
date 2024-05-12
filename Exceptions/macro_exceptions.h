#pragma once

#include <cassert>
#include <csetjmp>
#include <functional>
#include <stack>
#include <type_traits>
#include <utility>

#include <memory>

namespace exceptions {
// Types declarations.
enum class error {
    NO_ERRORS = 0,
    IO_ERROR,
    MATH_ERROR,
    NUM_ERRORS = MATH_ERROR,
};

using ExceptionHandler = std::function<void(error)>;
using DoubleExceptionHandler = void(error, error);

class ObjectFrameScope;

void ClearStack();
void HandleSecondException(int first, int second);

// Global objects declarations.
extern std::jmp_buf ERROR_HANDLER_BUF;
extern std::array<ExceptionHandler, static_cast<size_t>(error::NUM_ERRORS)> ERROR_HANDLERS;
extern DoubleExceptionHandler *DOUBLE_ERROR_HANDLER;
extern std::stack<ObjectFrameScope *> EXCEPTION_STACK_CLEANUP;

static_assert(std::is_same_v<std::invoke_result<decltype(setjmp), std::jmp_buf>::type, int>, "");

class ObjectFrameScope {
public:
    template <typename F>
    explicit ObjectFrameScope(F &&deleter, bool doCleanStack = false) : deleter_(std::move(deleter)) {
        if (doCleanStack) {
            ClearStack();
        }
        EXCEPTION_STACK_CLEANUP.push(this);
    }

    ObjectFrameScope(const ObjectFrameScope &) = delete;
    ObjectFrameScope &operator=(const ObjectFrameScope &) = delete;
    ObjectFrameScope(ObjectFrameScope &&) = delete;
    ObjectFrameScope &operator=(ObjectFrameScope &&) = delete;

    ~ObjectFrameScope() {
        assert(!EXCEPTION_STACK_CLEANUP.empty());
        assert(EXCEPTION_STACK_CLEANUP.top() == this);
        EXCEPTION_STACK_CLEANUP.pop();
    }

    bool DeleteOnException() {
        return deleter_();
    }

private:
    std::function<bool()> deleter_;
};

}   // namespace exceptions

// Macros definitions.
#define TRY                                                             \
    exceptions::ObjectFrameScope _TRY_SENTINEL([](){ return true; });   \
    int _SETJMP_RESULT = 0;                                             \
    if ((_SETJMP_RESULT = setjmp(exceptions::ERROR_HANDLER_BUF)) == 0)

#define CATCH(error_status)                                                                                                     \
    else if (_SETJMP_RESULT == static_cast<int>(error_status))                                                                  \
        if (volatile auto _TRY_SECOND_SENTINEL = std::make_unique<exceptions::ObjectFrameScope>([](){ return true; }, true);    \
            (_SETJMP_RESULT = setjmp(exceptions::ERROR_HANDLER_BUF)) != 0) {                                                    \
            exceptions::HandleSecondException(static_cast<int>(error_status), _SETJMP_RESULT);                                  \
        } else

#define THROW(status) std::longjmp(exceptions::ERROR_HANDLER_BUF, static_cast<int>(status))

#define SET_UNEXPECTED_HANDLER(handler) exceptions::DOUBLE_ERROR_HANDLER = handler

#define AUTO_OBJECT(class_name, object_name, ...)                                           \
    auto object_name = class_name(__VA_ARGS__);                                             \
    volatile auto _OBJECT_FRAME_##class_name##object_name                                   \
        = std::make_unique<exceptions::ObjectFrameScope>([obj = &object_name]() mutable {   \
        static_assert(std::is_same_v<decltype(obj), class_name *>, "");                     \
        obj->~class_name();                                                                 \
        return false;                                                                       \
    })
