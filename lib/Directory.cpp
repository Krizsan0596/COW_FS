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
      size(0),
      capacity(8),
      contents(std::make_unique<std::shared_ptr<FSObject>[]>(capacity)) {
    for (size_t i = 0; i < other.size; i++) {
        if (!other.contents[i]) continue;
        if (dynamic_cast<Directory*>(other.contents[i].get())) {
            if (size == capacity) {
                resizeContents();
            }
            contents[size++] = std::make_shared<Directory>(other.contents[i]->getName());
        }
    }
    auto pendingSrc = makeRemapArray<const Directory*>();
    auto pendingDst = makeRemapArray<Directory*>();
    auto symlink_map = makeRemapArray<symlink_remap>();
    auto hardlink_map = makeRemapArray<hardlink_remap>();

    pendingDst.data[pendingDst.count++] = this;
    pendingSrc.data[pendingSrc.count++] = &other;

    auto cloneContents = [&](const Directory& src, Directory& dst) -> void {
        for (size_t i = 0; i < src.size; i++) {
            if (auto dir = dynamic_cast<Directory*>(src.contents[i].get())) {
                if (pendingSrc.capacity == pendingSrc.count) resizeRemapArray(pendingSrc);
                if (pendingDst.capacity == pendingDst.count) resizeRemapArray(pendingDst);
                pendingSrc.data[pendingSrc.count++] = dir;
                pendingDst.data[pendingDst.count++] = static_cast<Directory*>(dst.get(dir->getName()).get());

                if (symlink_map.count == symlink_map.capacity) resizeRemapArray(symlink_map);
                symlink_map.data[symlink_map.count++] = { src.contents[i], dst.get(src.contents[i]->getName()) };
                continue;
            }
            if (auto symlink = dynamic_cast<Symlink*>(src.contents[i].get())) {
                dst.ln(symlink->getName(), symlink->target.lock());

                if (symlink_map.count == symlink_map.capacity) resizeRemapArray(symlink_map);
                symlink_map.data[symlink_map.count++] = { src.contents[i], dst.get(src.contents[i]->getName()) };
                continue;
            }
            if (auto file = dynamic_cast<File*>(src.contents[i].get())) {
                auto newFile = dst.touch(file->getName());
                newFile->write(file->read());

                if (symlink_map.count == symlink_map.capacity) resizeRemapArray(symlink_map);
                symlink_map.data[symlink_map.count++] = { src.contents[i], dst.get(src.contents[i]->getName()) };

                if (hardlink_map.count == hardlink_map.capacity) resizeRemapArray(hardlink_map);
                hardlink_map.data[hardlink_map.count++] = { file->inode, newFile };
                continue;
            }
        }
    };
}

void Directory::resizeContents() {
    size_t newCapacity = growByHalf(capacity);
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
