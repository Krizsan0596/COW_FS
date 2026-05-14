#include "Dispatcher.hpp"
#include "FSObject.hpp"
#include "SnapshotManager.hpp"
#include "Symlink.hpp"
#include "Util.hpp"
#include <cstddef>
#include <exception>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <iostream>

enum class RoutedCommand {
    Read,
    Write,
    Rm,
    Slink,
    Hlink,
    Mkdir,
    Rmdir,
    Ls,
    Snapshot,
    Restore,
    Exit,
    Unknown
};

RoutedCommand parseCommand(const std::string& command) {
    if (command == "read") return RoutedCommand::Read;
    if (command == "write") return RoutedCommand::Write;
    if (command == "rm") return RoutedCommand::Rm;
    if (command == "slink") return RoutedCommand::Slink;
    if (command == "hlink") return RoutedCommand::Hlink;
    if (command == "mkdir") return RoutedCommand::Mkdir;
    if (command == "rmdir") return RoutedCommand::Rmdir;
    if (command == "ls") return RoutedCommand::Ls;
    if (command == "createSnapshot") return RoutedCommand::Snapshot;
    if (command == "restoreSnapshot") return RoutedCommand::Restore;
    if (command == "exit" or command == "quit") return RoutedCommand::Exit;
    return RoutedCommand::Unknown;
}

static bool parseToken(std::stringstream& ss, std::string& token) {
    ss >> std::ws;
    if (!ss) return false;

    if (ss.peek() == '"') {
        ss.get();
        std::getline(ss, token, '"');
        return ss.good() || ss.eof();
    } else {
        ss >> token;
        return !token.empty();
    }
}

static void checkExtraArgs(std::stringstream& ss, const std::string& cmd) {
    std::string extra;
    if (ss >> extra) throw std::runtime_error("Too many arguments for " + cmd);
}

std::string Dispatcher::stripTrailingSlashes(std::string path) const {
    while (path.size() > 1 && path.back() == '/') path.pop_back();
    return path;
}

Dispatcher::Dispatcher() : root(std::make_shared<Directory>("")), snapshotManager(std::make_unique<SnapshotManager>()) {}

void Dispatcher::route() {
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::stringstream lineStream(line);
        std::string command;
        if (!(lineStream >> command)) continue;

        try {
            switch (parseCommand(command)) {
                case RoutedCommand::Read: {
                    std::string path;
                    if (!parseToken(lineStream, path)) throw std::runtime_error("Missing path for read");
                    checkExtraArgs(lineStream, "read");
                    read(path);
                    break;
                }
                case RoutedCommand::Write: {
                    std::string path;
                    if (!parseToken(lineStream, path)) throw std::runtime_error("Missing path for write");
                    std::string data;
                    std::getline(lineStream, data);
                    if (!data.empty() && (data[0] == ' ' || data[0] == '\t')) {
                        data.erase(0, 1);
                    }
                    write(path, data);
                    break;
                }
                case RoutedCommand::Rm: {
                    std::string path;
                    if (!parseToken(lineStream, path)) throw std::runtime_error("Missing path for rm");
                    checkExtraArgs(lineStream, "rm");
                    rm(path);
                    break;
                }
                case RoutedCommand::Slink: {
                    std::string dstPath, srcPath;
                    if (!parseToken(lineStream, dstPath) || !parseToken(lineStream, srcPath))
                        throw std::runtime_error("Missing path for slink");
                    checkExtraArgs(lineStream, "slink");
                    slink(dstPath, srcPath);
                    break;
                }
                case RoutedCommand::Hlink: {
                    std::string dstPath, srcPath;
                    if (!parseToken(lineStream, dstPath) || !parseToken(lineStream, srcPath))
                        throw std::runtime_error("Missing path for hlink");
                    checkExtraArgs(lineStream, "hlink");
                    hlink(dstPath, srcPath);
                    break;
                }
                case RoutedCommand::Mkdir: {
                    std::string path;
                    if (!parseToken(lineStream, path)) throw std::runtime_error("Missing path for mkdir");
                    checkExtraArgs(lineStream, "mkdir");
                    mkdir(path);
                    break;
                }
                case RoutedCommand::Rmdir: {
                    std::string path;
                    if (!parseToken(lineStream, path)) throw std::runtime_error("Missing path for rmdir");
                    checkExtraArgs(lineStream, "rmdir");
                    rmdir(path);
                    break;
                }
                case RoutedCommand::Ls: {
                    std::string path;
                    if (!parseToken(lineStream, path)) throw std::runtime_error("Missing path for ls");
                    checkExtraArgs(lineStream, "ls");
                    ls(path);
                    break;
                }
                case RoutedCommand::Snapshot: {
                    checkExtraArgs(lineStream, "createSnapshot");
                    createSnapshot();
                    break;
                }
                case RoutedCommand::Restore: {
                    int index;
                    if (!(lineStream >> index)) throw std::runtime_error("Missing index for restore");
                    checkExtraArgs(lineStream, "restoreSnapshot");
                    restoreSnapshot(index);
                    break;
                }
                case RoutedCommand::Exit: {
                    return;
                }
                case RoutedCommand::Unknown:
                default:
                    throw std::runtime_error("Unknown command: " + command);
            }
        } catch (const FileSystemError& e) {
            std::cerr << command << ": " << e.path() << ": " << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cerr << command << ": " << e.what() << "\n";
        }
    }
}

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
            else if (dynamic_cast<File*>(currentObject.get())) {
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

void Dispatcher::write(const std::string& path, const std::string& data) {
    try {
        std::shared_ptr<FSObject> node;
        std::string normalizedPath = stripTrailingSlashes(path);
        try {
            node = resolvePath(normalizedPath);
        } catch (const std::runtime_error& e) {
            if (std::string(e.what()) != "No such file or directory") throw;

            size_t pos = normalizedPath.rfind('/');
            if (pos == std::string::npos) throw std::runtime_error("Only absolute paths allowed");

            std::string parent = (pos == 0) ? "/" : normalizedPath.substr(0, pos);
            std::string child = normalizedPath.substr(pos + 1);
            if (child.empty()) throw std::runtime_error("Invalid path");

            node = resolvePath(parent);
            node = node->resolve();
            if (auto dir = dynamic_cast<Directory*>(node.get())) {
                try {
                    dir->get(child);
                    throw std::runtime_error("Destination already exists");
                } catch (const std::runtime_error& inner) {
                    if (std::string(inner.what()) == "Destination already exists") throw;
                }
                node = dir->touch(child);
            } else {
                throw std::runtime_error("Parent is not a directory");
            }
        }
        node = node->resolve();
        if (auto file = dynamic_cast<File*>(node.get())) {
            file->write(data);
            return;
        }
        throw std::runtime_error("Is a directory");
    } catch (const std::runtime_error& e) {
        throw FileSystemError(e.what(), path);
    }
}

void Dispatcher::rm(const std::string& path) {
    try {
        std::string normalizedPath = stripTrailingSlashes(path);
        size_t pos = normalizedPath.rfind('/');
        if (pos == std::string::npos) throw std::runtime_error("Only absolute paths allowed");

        std::string parent = (pos == 0) ? "/" : normalizedPath.substr(0, pos);
        std::string child = normalizedPath.substr(pos + 1);
        if (child.empty()) throw std::runtime_error("Invalid path");

        std::shared_ptr<FSObject> node = resolvePath(parent);
        node = node->resolve();
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

        std::string parent = (pos == 0) ? "/" : normalizedPath.substr(0, pos);
        std::string child = normalizedPath.substr(pos + 1);
        if (child.empty()) throw std::runtime_error("Invalid path");

        std::shared_ptr<FSObject> node = resolvePath(parent);
        node = node->resolve();
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

        std::string parent = (pos == 0) ? "/" : normalizedPath.substr(0, pos);
        std::string child = normalizedPath.substr(pos + 1);
        if (child.empty()) throw std::runtime_error("Invalid path");

        std::shared_ptr<FSObject> node = resolvePath(parent);
        node = node->resolve();
        if (auto dir = dynamic_cast<Directory*>(node.get())) {
            try {
                dir->get(child);
                throw std::runtime_error("Directory already exists");
            } catch (const std::runtime_error& inner) {
                if (std::string(inner.what()) == "Directory already exists") throw;
                dir->mkdir(child);
                return;
            }
        }
        else throw std::runtime_error("No such file or directory");
    } catch (const std::runtime_error& e) {
        throw FileSystemError(e.what(), path);
    }
}

void Dispatcher::slink(const std::string& dstPath, const std::string& srcPath) {
    std::shared_ptr<FSObject> srcNode;
    try {
        srcNode = resolvePath(srcPath);
    } catch (const std::runtime_error& e) {
        throw FileSystemError(e.what(), srcPath);
    }
    try {
        std::string normalizedPath = stripTrailingSlashes(dstPath);
        size_t pos = normalizedPath.rfind('/');
        if (pos == std::string::npos) throw std::runtime_error("Only absolute paths allowed");

        std::string parent = (pos == 0) ? "/" : normalizedPath.substr(0, pos);
        std::string child = normalizedPath.substr(pos + 1);
        if (child.empty()) throw std::runtime_error("Invalid path");

        std::shared_ptr<FSObject> node = resolvePath(parent);
        node = node->resolve();
        if (auto dir = dynamic_cast<Directory*>(node.get())) {
            try {
                dir->get(child);
                throw std::runtime_error("Destination already exists");
            } catch (const std::runtime_error& inner) {
                if (std::string(inner.what()) == "Destination already exists") throw;
            }
            dir->ln(child, srcNode);
        } else {
            throw std::runtime_error("Parent is not a directory");
        }
    } catch (std::runtime_error& e) {
        throw FileSystemError(e.what(), dstPath);
    }

}

void Dispatcher::hlink(const std::string& dstPath, const std::string& srcPath) {
    std::shared_ptr<FSObject> srcNode;
    try {
        srcNode = resolvePath(srcPath);
        srcNode = srcNode->resolve();
        if (!dynamic_cast<File*>(srcNode.get())) throw std::runtime_error("Cannot hardlink directory");
    } catch (const std::runtime_error& e) {
        throw FileSystemError(e.what(), srcPath);
    }
    try {
        std::string normalizedPath = stripTrailingSlashes(dstPath);
        size_t pos = normalizedPath.rfind('/');
        if (pos == std::string::npos) throw std::runtime_error("Only absolute paths allowed");

        std::string parent = (pos == 0) ? "/" : normalizedPath.substr(0, pos);
        std::string child = normalizedPath.substr(pos + 1);
        if (child.empty()) throw std::runtime_error("Invalid path");

        std::shared_ptr<FSObject> node = resolvePath(parent);
        node = node->resolve();
        auto dir = dynamic_cast<Directory*>(node.get());
        if (!dir) throw std::runtime_error("Destination parent is not a directory");
        try {
            dir->get(child);
            throw std::runtime_error("Destination already exists");
        } catch (const std::runtime_error& inner) {
            if (std::string(inner.what()) == "Destination already exists") throw;
        }
        dir->touch(child, *dynamic_cast<File*>(srcNode.get()));
    } catch (const std::runtime_error& e) {
        throw FileSystemError(e.what(), dstPath);
    }
}

void Dispatcher::createSnapshot() const {
    snapshotManager->createSnapshot(*this);
}

void Dispatcher::restoreSnapshot(int index) {
    if (index < 0) throw std::runtime_error("Snapshot index must not be negative");
    snapshotManager->restoreSnapshot(static_cast<size_t>(index), *this);
}
