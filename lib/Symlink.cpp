#include "Symlink.hpp"
#include "FSObject.hpp"
#include <memory>
#include <stdexcept>
#include <string>

#define MAX_DEPTH 5

Symlink::Symlink(const std::string& fileName, const std::shared_ptr<FSObject>& source) : FSObject(fileName), target(source) {}

Symlink::Symlink(const Symlink& other) : FSObject(other.getName()), target(other.target) {}

const std::weak_ptr<FSObject>& Symlink::getTarget() const {
    return target;
}

void Symlink::setTarget(const std::shared_ptr<FSObject>& source) {
    target = source;
}

std::shared_ptr<FSObject> Symlink::resolve(int depth) {
    if (depth > MAX_DEPTH) throw std::runtime_error("Too many levels of symbolic links\n");
    if (auto resolved = target.lock()) {
        return resolved->resolve(depth + 1);
    }
    else throw std::runtime_error("Target has been deleted, dangling symlink\n");
}
