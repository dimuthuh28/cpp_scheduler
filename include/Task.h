#pragma once

// Base abstraction for a unit of work the scheduler can execute.
class Task {
public:
    virtual void execute() = 0;
    virtual ~Task() = default;
};
