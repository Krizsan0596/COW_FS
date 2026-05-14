#pragma once

#include "FSObject.hpp"
#include "Directory.hpp"

#include <memory>

class Symlink : public FSObject {
private:
    std::weak_ptr<FSObject> target;
public:
    Symlink() = delete;
    Symlink(const std::string& destination, const std::shared_ptr<FSObject>& source);
    Symlink(const Symlink& other);
    std::shared_ptr<FSObject> resolve(int depth = 0) override;
    friend Directory::Directory(const Directory& other);
};
