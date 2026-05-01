#pragma once

#include "FSObject.hpp"

#include <memory>

class Symlink : public FSObject {
private:
    const std::weak_ptr<FSObject> target;
public:
    Symlink() = delete;
    Symlink(const std::shared_ptr<FSObject>& source);
    std::shared_ptr<FSObject> resolve(int depth) override;
};
