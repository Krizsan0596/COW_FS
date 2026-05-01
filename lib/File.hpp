#pragma once

#include "FSObject.hpp"

#include <memory>

class Inode;

class File : public FSObject {
private:
    std::shared_ptr<Inode> inode;

public:
    std::shared_ptr<FSObject> resolve(int depth) override;
};
