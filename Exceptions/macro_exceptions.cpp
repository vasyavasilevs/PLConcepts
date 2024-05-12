#include <cstdlib>
#include <iostream>

#include "macro_exceptions.h"

namespace exceptions {
// Global objects definitions.
std::jmp_buf ERROR_HANDLER_BUF;

std::array<ExceptionHandler, static_cast<size_t>(error::NUM_ERRORS)> ERROR_HANDLERS;

DoubleExceptionHandler *DOUBLE_ERROR_HANDLER = nullptr;

std::stack<ObjectFrameScope *> EXCEPTION_STACK_CLEANUP;

void ClearStack() {
    while (!EXCEPTION_STACK_CLEANUP.empty()) {
        auto *top = EXCEPTION_STACK_CLEANUP.top();
        if (top->DeleteOnException()) {
            return;
        }
        EXCEPTION_STACK_CLEANUP.pop();
    }
}

void HandleSecondException(int first, int second) {
    if (DOUBLE_ERROR_HANDLER) {
        DOUBLE_ERROR_HANDLER(static_cast<error>(first), static_cast<error>(second));
    } else {
        std::cerr << "Terminating after receiving an unhandled double exception: "
                  << " first " << first << ", second " << second << std::endl;
        std::abort();
    }
}
}   // namespace exceptions
