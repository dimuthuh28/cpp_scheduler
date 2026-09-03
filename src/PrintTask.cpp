#include "PrintTask.h"

#include "Logger.h"

PrintTask::PrintTask(std::string message) : message_(std::move(message)) {}

void PrintTask::execute() { Logger::info(message_); }
