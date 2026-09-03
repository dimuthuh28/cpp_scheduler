#pragma once

#include <string>

#include "Task.h"

// Simple task that logs a message when executed.
class PrintTask : public Task {
public:
    explicit PrintTask(std::string message);
    void execute() override;

private:
    std::string message_;
};
