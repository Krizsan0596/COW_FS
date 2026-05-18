#pragma once

#include <memory>
#include <string>

class FSObject {
private:
    std::string name;
public:
    FSObject() = delete;
    explicit FSObject(const std::string& Name) : name(Name) {}
    virtual ~FSObject() = default;
    [[nodiscard]] virtual std::shared_ptr<FSObject> resolve(int depth = 0) = 0;
    [[nodiscard]] const std::string& getName() const noexcept { return name; }
};
