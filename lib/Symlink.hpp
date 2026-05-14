#pragma once

#include "FSObject.hpp"

#include <memory>

class Symlink : public FSObject {
private:
    std::weak_ptr<FSObject> target;
public:
    Symlink() = delete;
    Symlink(const std::string& destination, const std::shared_ptr<FSObject>& source);
    Symlink(const Symlink& other);
    const std::weak_ptr<FSObject>& getTarget() const;
    void setTarget(const std::shared_ptr<FSObject>& source);
    std::shared_ptr<FSObject> resolve(int depth = 0) override;
};
