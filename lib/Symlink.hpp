#pragma once

#include "FSObject.hpp"

#include <memory>

class Symlink : public FSObject {
private:
    std::weak_ptr<FSObject> target;

public:
    std::shared_ptr<FSObject> resolve(int depth) override;
};
