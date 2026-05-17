#include "Directory.hpp"
#include "FSObject.hpp"
#include "File.hpp"
#include "Symlink.hpp"
#include "Util.hpp"
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
    auto dirsToClone = makeRemapArray<directory_remap>();
    auto symlink_map = makeRemapArray<symlink_remap>();
    auto hardlink_map = makeRemapArray<hardlink_remap>();

    dirsToClone.data[dirsToClone.count++] = { &other, this };

    auto cloneContents = [&](const Directory& src, Directory& dst) -> void {
        for (size_t i = 0; i < src.size; i++) {
            if (auto dir = dynamic_cast<Directory*>(src.contents[i].get())) {
                if (dirsToClone.capacity == dirsToClone.count) resizeRemapArray(dirsToClone);
                dirsToClone.data[dirsToClone.count++] = {
                    dir,
                    static_cast<Directory*>(dst.get(dir->getName()).get())
                };

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
                size_t j = 0;
                for (; j < hardlink_map.count; j++) {
                    if (file->inode == hardlink_map.data[j].oldInode) break;
                }
                bool isHardlink = j < hardlink_map.count;
                std::shared_ptr<File> newFile;
                if (!isHardlink) {
                    newFile = dst.touch(file->getName());
                    newFile->write(file->read());
                }
                else {
                    newFile = dst.touch(file->getName(), *hardlink_map.data[j].newFile);
                }

                if (symlink_map.count == symlink_map.capacity) resizeRemapArray(symlink_map);
                symlink_map.data[symlink_map.count++] = { src.contents[i], newFile };
                
                if (!isHardlink) {
                    if (hardlink_map.count == hardlink_map.capacity) resizeRemapArray(hardlink_map);
                    hardlink_map.data[hardlink_map.count++] = { file->inode, newFile };
                }
                continue;
            }
        }
    };

    while (dirsToClone.count > 0) {
        const auto current = dirsToClone.data[--dirsToClone.count];
        cloneContents(*current.oldDir, *current.newDir);
    }

    auto pendingDirs = makeRemapArray<Directory*>();
    pendingDirs.data[pendingDirs.count++] = this;

    auto fixLinks = [&symlink_map, &pendingDirs](Directory& dir) -> void {
        for (size_t i = 0; i < dir.size; i++) {
            if (auto subdir = dynamic_cast<Directory*>(dir.contents[i].get())) {
                if (pendingDirs.count == pendingDirs.capacity) resizeRemapArray(pendingDirs);
                pendingDirs.data[pendingDirs.count++] = subdir;
            }
            if (auto symlink = dynamic_cast<Symlink*>(dir.contents[i].get())) {
                for (size_t j = 0; j < symlink_map.count; j++) {
                    auto current = symlink_map.data[j];
                    if (symlink->target.lock() == current.oldTarget) symlink->target = current.newTarget;
                }
            }
        }
    };

    while (pendingDirs.count > 0) {
        fixLinks(*pendingDirs.data[--pendingDirs.count]);
    }
}

Directory::~Directory() {
    auto pendingDirs = makeRemapArray<std::shared_ptr<Directory>>();

    auto ownDirs = [&pendingDirs] (Directory& dir) -> void {
        for (size_t i = 0; i < dir.size; i++) {
            if (dir.contents[i]) {
                if (dynamic_cast<Directory*>(dir.contents[i].get())) {
                    if (pendingDirs.count == pendingDirs.capacity) resizeRemapArray(pendingDirs);
                    pendingDirs.data[pendingDirs.count++] = std::dynamic_pointer_cast<Directory>(std::move(dir.contents[i]));
                }
            }
        }
    };

    ownDirs(*this);
    while (pendingDirs.count > 0) {
        auto dir = std::move(pendingDirs.data[--pendingDirs.count]);
        if (dir) {
            ownDirs(*dir);
        }
    }
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
