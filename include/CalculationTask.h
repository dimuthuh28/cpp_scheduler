#pragma once

#include "Task.h"

// Task that computes the sum of squares in [1, n] and logs the result.
// Exists to demonstrate a CPU-bound task alongside PrintTask's I/O-bound one.
class CalculationTask : public Task {
public:
    explicit CalculationTask(long n);
    void execute() override;
    long result() const;

private:
    long n_;
    long result_ = 0;
};
