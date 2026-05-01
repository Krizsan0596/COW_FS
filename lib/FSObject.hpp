#pragma once

#include <memory>
#include <string>

class FSObject {
private:
    std::string name;

public:
    FSObject() = default;
    virtual ~FSObject() = default;
    virtual std::shared_ptr<FSObject> resolve(int depth) = 0;
};
