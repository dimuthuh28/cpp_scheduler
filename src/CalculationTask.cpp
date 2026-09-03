#include "CalculationTask.h"

#include <sstream>

#include "Logger.h"

CalculationTask::CalculationTask(long n) : n_(n) {}

void CalculationTask::execute() {
    long sum = 0;
    for (long i = 1; i <= n_; ++i) {
        sum += i * i;
    }
    result_ = sum;

    std::ostringstream oss;
    oss << "CalculationTask(n=" << n_ << ") -> sum of squares = " << result_;
    Logger::info(oss.str());
}

long CalculationTask::result() const { return result_; }
