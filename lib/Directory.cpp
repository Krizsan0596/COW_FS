#include "Directory.hpp"
#include "FSObject.hpp"
#include "File.hpp"
#include "Symlink.hpp"
#include "Util.hpp"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>

Directory::Directory(const std::string& dirName)
    : FSObject(dirName), size(0), capacity(8), contents(std::make_unique<std::shared_ptr<FSObject>[]>(capacity)) {}

Directory::Directory(const Directory& other)
    : FSObject(other),
      std::enable_shared_from_this<Directory>(),
      size(other.size),
      capacity(other.capacity),
      contents(std::make_unique<std::shared_ptr<FSObject>[]>(capacity)) {

    auto hardlink_map = makeRemapArray<hardlink_remap>();

    auto symlink_map = makeRemapArray<symlink_remap>();

    for (std::size_t i = 0; i < size; i++) {
        if (!other.contents[i]) {
            contents[i].reset();
            continue;
        }

        if (Directory* dir = dynamic_cast<Directory*>(other.contents[i].get())) {
            contents[i] = std::make_shared<Directory>(*dir);
        } else if (File* file = dynamic_cast<File*>(other.contents[i].get())) {
            bool hlink = false;
            for (size_t j = 0; j < hardlink_map.count; j++) {
                if (file->inode == hardlink_map.data[j].oldInode) {
                    contents[i] = std::make_shared<File>(file->getName(), *hardlink_map.data[j].newFile.get());
                    hlink = true;
                    break;
                }
            }
            if (!hlink) {
                contents[i] = std::make_shared<File>(*file);
                if (hardlink_map.capacity == hardlink_map.count) resizeRemapArray(hardlink_map);
                hardlink_map.data[hardlink_map.count++] = {file->inode, std::static_pointer_cast<File>(contents[i])};
            }
        } else if (Symlink* symlink = dynamic_cast<Symlink*>(other.contents[i].get())) {
            contents[i] = std::make_shared<Symlink>(*symlink);
        } else {
            throw std::logic_error("Unknown FSObject type during Directory copy");
        }

        if (symlink_map.capacity == symlink_map.count) resizeRemapArray(symlink_map);
        symlink_map.data[symlink_map.count++] = {other.contents[i], contents[i]};
    }
    
    auto pendingDirs = makeRemapArray<Directory*>();
    pendingDirs.data[pendingDirs.count++] = this;
    
    while (pendingDirs.count > 0) {
        auto currentDir = pendingDirs.data[--pendingDirs.count];
        for (size_t i = 0; i < currentDir->size; i++) {
            bool done = false;
            auto item = currentDir->contents[i];
            if (auto symlink = dynamic_cast<Symlink*>(item.get())) {
                for (size_t j = 0; j < symlink_map.count; j++) {
                    if (auto target = symlink->target.lock())
                        if (target == symlink_map.data[j].oldTarget) {
                            symlink->target = symlink_map.data[j].newTarget;
                            done = true;
                            break;
                        }
                }
                if (done) continue;
            }
            if (auto dir = dynamic_cast<Directory*>(item.get())) {
                if (pendingDirs.count == pendingDirs.capacity) resizeRemapArray(pendingDirs);
                pendingDirs.data[pendingDirs.count++] = dir;
            }
        }
    }
}

void Directory::resizeContents() {
    const std::size_t newCapacity = growByHalf(capacity);
    contents = resizeArray(std::move(contents), size, newCapacity);
    capacity = newCapacity;
}

std::shared_ptr<File> Directory::touch(const std::string& child) {
    if (size == capacity) {
        resizeContents();
    }
    std::shared_ptr<File> new_file = std::make_shared<File>(child);
    contents[size++] = new_file;
    return new_file;
}

std::shared_ptr<File> Directory::touch(const std::string& child, const File& source) {
    if (size == capacity) {
        resizeContents();
    }
    std::shared_ptr<File> new_file = std::make_shared<File>(child, source);
    contents[size++] = new_file;
    return new_file;
}

void Directory::mkdir(const std::string& child) {
    if (size == capacity) {
        resizeContents();
    }
    contents[size++] = std::make_shared<Directory>(child);
}

void Directory::ln(const std::string& child, const std::shared_ptr<FSObject>& target) {
    if (size == capacity) {
        resizeContents();
    }
    contents[size++] = std::make_shared<Symlink>(child, target);
}

std::shared_ptr<FSObject> Directory::resolve(int) {
    return shared_from_this();
}

void Directory::list() {
    for (size_t i = 0; i < size; i++) {
        if (!contents[i]) continue;
        if (auto dir = dynamic_cast<Directory*>(contents[i].get())) {
            std::cout << dir->getName() << "/\n";
        }
        else if (auto link = dynamic_cast<Symlink*>(contents[i].get())) {
            if (dynamic_cast<Directory*>(link->resolve().get())) {
                std::cout << link->getName() << "/\n";
            }
            else std::cout << contents[i]->getName() << '\n';
        }
        else std::cout << contents[i]->getName() << '\n';
    }
}

void Directory::removeDir(const std::string& child) {
    for (size_t i = 0; i < size; ++i) {
        if (!contents[i]) continue;
        if (contents[i]->getName() == child) {
            if (!dynamic_cast<Directory*>(contents[i].get()))
                throw std::runtime_error("Not a directory");
            for (size_t j = i; j + 1 < size; ++j) {
                contents[j] = std::move(contents[j + 1]);
            }
            contents[size - 1].reset();
            --size;
            return;
        }
    }
    throw std::runtime_error("No such file or directory");
}

void Directory::removeFile(const std::string& child) {
    for (size_t i = 0; i < size; ++i) {
        if (!contents[i]) continue;
        if (contents[i]->getName() == child) {
            if (dynamic_cast<Directory*>(contents[i].get()))
                throw std::runtime_error("Is a directory");
            for (size_t j = i; j + 1 < size; ++j) {
                contents[j] = std::move(contents[j + 1]);
            }
            contents[size - 1].reset();
            --size;
            return;
        }
    }
    throw std::runtime_error("No such file or directory");
}


std::shared_ptr<FSObject>& Directory::get(const std::string& child) {
    for (size_t i = 0; i < size; i++) {
        if (contents[i]->getName() == child) return contents[i];
    }
    throw std::runtime_error("No such file or directory");
}
