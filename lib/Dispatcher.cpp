#include "Dispatcher.hpp"
#include "FSObject.hpp"
#include "Symlink.hpp"
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

Dispatcher::Dispatcher() : root(std::make_shared<Directory>("")) {}

std::shared_ptr<FSObject> Dispatcher::resolvePath(const std::string& path) const {
    if (path.empty()) throw std::logic_error("Path must not be empty");
    if (path[0] != '/') throw std::runtime_error("Only absolute paths allowed");

    std::shared_ptr<FSObject> currentObject = root;
    std::stringstream pathStream(path);
    std::string item;
    int depth = 0;
    std::shared_ptr<FSObject> visited[MAX_PATH_DEPTH];
    while (std::getline(pathStream, item, '/')) {
        if (item == "") continue;
        if (item == ".") continue;
        if (item == "..") {
            if (depth == 0) throw std::runtime_error("No such file or directory");
            currentObject = visited[--depth];
            continue;
        }
        if (depth >= MAX_PATH_DEPTH) throw std::runtime_error("Maximum path depth exceeded");
        visited[depth++] = currentObject;
        if (auto symlink = dynamic_cast<Symlink*>(currentObject.get())) {
            currentObject = symlink->resolve();
        }
        if (auto dir = dynamic_cast<Directory*>(currentObject.get())) {
            currentObject = dir->get(item);
        }
        else if (auto file = dynamic_cast<File*>(currentObject.get())) {
            throw std::runtime_error("No such file or directory");
        }
    }
    return currentObject;
}
