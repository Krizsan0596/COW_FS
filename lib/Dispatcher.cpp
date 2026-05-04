#include "Dispatcher.hpp"
#include "FSObject.hpp"
#include "Symlink.hpp"
#include "Util.hpp"
#include <cstddef>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>

namespace {
std::string stripTrailingSlashes(std::string path) {
    while (path.size() > 1 && path.back() == '/') path.pop_back();
    return path;
}
}

Dispatcher::Dispatcher() : root(std::make_shared<Directory>("")) {}

std::shared_ptr<FSObject> Dispatcher::resolvePath(const std::string& path) const {
    try {
        std::string normalizedPath = stripTrailingSlashes(path);
        if (normalizedPath.empty()) throw std::logic_error("Path must not be empty");
        if (normalizedPath[0] != '/') throw std::runtime_error("Only absolute paths allowed");

        std::shared_ptr<FSObject> currentObject = root;
        std::stringstream pathStream(normalizedPath);
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
    } catch (const std::runtime_error& e) {
        throw FileSystemError(e.what(), path);
    }
}

void Dispatcher::ls(const std::string& path) const {
    try {
        std::shared_ptr<FSObject> node = resolvePath(path);
        node = node->resolve();
        if (auto dir = dynamic_cast<Directory*>(node.get())) {
            dir->list();
            return;
        }
        throw std::runtime_error("Not a directory");
    } catch (const std::runtime_error& e) {
        throw FileSystemError(e.what(), path);
    }
}

void Dispatcher::read(const std::string& path) const {
    try {
        std::shared_ptr<FSObject> node = resolvePath(path);
        node = node->resolve();
        if (auto file = dynamic_cast<File*>(node.get())) {
            std::cout << file->read() << "\n";
            return;
        }
        throw std::runtime_error("Is a directory");
    } catch (const std::runtime_error& e) {
        throw FileSystemError(e.what(), path);
    }
}

// void Dispatcher::write(const std::string& path, const std::string& data) {
//     std::shared_ptr<FSObject> node = resolvePath(path);
//     node = node->resolve();
//     if (auto file = dynamic_cast<File*>(node.get())) {
//         file->write(data);
//         return;
//     }
//     throw std::runtime_error("Is a directory");
// }

void Dispatcher::rm(const std::string& path) {
    try {
        std::string normalizedPath = stripTrailingSlashes(path);
        size_t pos = normalizedPath.rfind('/');
        if (pos == std::string::npos) throw std::runtime_error("Only absolute paths allowed");

        std::string parent = (pos == 0) ? normalizedPath.substr(0, pos) : normalizedPath.substr(0, 1);
        std::string child = normalizedPath.substr(pos + 1);

        std::shared_ptr<FSObject> node = resolvePath(parent);
        if (auto dir = dynamic_cast<Directory*>(node.get())) {
            dir->removeFile(child);
            return;
        }
        throw std::runtime_error("No such file or directory");
    } catch (const std::runtime_error& e) {
        throw FileSystemError(e.what(), path);
    }
}

void Dispatcher::rmdir(const std::string& path) {
    try {
        std::string normalizedPath = stripTrailingSlashes(path);
        size_t pos = normalizedPath.rfind('/');
        if (pos == std::string::npos) throw std::runtime_error("Only absolute paths allowed");

        std::string parent = (pos == 0) ? normalizedPath.substr(0, pos) : normalizedPath.substr(0, 1);
        std::string child = normalizedPath.substr(pos + 1);

        std::shared_ptr<FSObject> node = resolvePath(parent);
        if (auto dir = dynamic_cast<Directory*>(node.get())) {
            dir->removeDir(child);
            return;
        }
        throw std::runtime_error("No such file or directory");
    } catch (const std::runtime_error& e) {
        throw FileSystemError(e.what(), path);
    }
}

void Dispatcher::mkdir(const std::string& path) {
    try {
        std::string normalizedPath = stripTrailingSlashes(path);
        size_t pos = normalizedPath.rfind('/');
        if (pos == std::string::npos) throw std::runtime_error("Only absolute paths allowed");

        std::string parent = (pos == 0) ? normalizedPath.substr(0, pos) : normalizedPath.substr(0, 1);
        std::string child = normalizedPath.substr(pos + 1);

        std::shared_ptr<FSObject> node = resolvePath(parent);
        node = node->resolve();
        if (auto dir = dynamic_cast<Directory*>(node.get())) {
            if (dir->get(child)) throw std::runtime_error("Directory already exists");
            dir->mkdir(child);
            return;
        }
        else throw std::runtime_error("No such file or directory");
    } catch (const std::runtime_error& e) {
        throw FileSystemError(e.what(), path);
    }
}

void Dispatcher::slink(const std::string& dstPath, const std::string& srcPath) {
    try {
        std::shared_ptr<FSObject> srcNode = resolvePath(srcPath);
        std::string normalizedPath = stripTrailingSlashes(dstPath);
    } catch (const std::runtime_error& e) {
        throw FileSystemError(e.what(), srcPath);
    }
}
