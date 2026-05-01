#include "Symlink.hpp"
#include "FSObject.hpp"
#include <memory>
#include <stdexcept>

#define MAX_DEPTH 5

Symlink::Symlink(const std::shared_ptr<FSObject>& source) : target(source) {}

std::shared_ptr<FSObject> Symlink::resolve(int depth) {
    if (depth > 5) throw std::runtime_error("Too many levels of symbolic links\n");
    if (auto resolved = target.lock()) {
        return resolved->resolve(depth + 1);
    }
    else throw std::runtime_error("Target has been deleted, dangling symlink\n");
}
