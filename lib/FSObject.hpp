#pragma once

#include <memory>
#include <string>

class FSObject {
private:
    std::string name;
public:
    FSObject() = delete;
    FSObject(const std::string& Name) : name(Name) {}
    virtual ~FSObject() = default;
    virtual std::shared_ptr<FSObject> resolve(int depth) = 0;
    std::string getName() const { return name; }
};
