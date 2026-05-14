#include "Directory.hpp"
#include "FSObject.hpp"
#include "File.hpp"
#include "Symlink.hpp"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

namespace {
template <typename T>
void growArray(std::unique_ptr<T[]>& array, std::size_t oldCapacity) {
    std::unique_ptr<T[]> larger = std::make_unique<T[]>(oldCapacity * 2);
    for (std::size_t i = 0; i < oldCapacity; ++i) {
        larger[i] = std::move(array[i]);
    }
    array = std::move(larger);
}
}

Directory::Directory(const std::string& dirName) : FSObject(dirName), size(0), capacity(8), contents(new std::shared_ptr<FSObject>[capacity]) {}

Directory::Directory(const Directory& other)
    : FSObject(other),
      std::enable_shared_from_this<Directory>(),
      size(other.size),
      capacity(other.capacity),
      contents(new std::shared_ptr<FSObject>[capacity]) {
    struct PendingDirectoryCopy {
        const Directory* source;
        Directory* clone;
    };
    struct PendingSymlinkCopy {
        const Symlink* source;
        std::shared_ptr<Symlink> clone;
    };
    struct CloneRecord {
        const FSObject* source;
        std::shared_ptr<FSObject> clone;
    };

    std::unique_ptr<std::shared_ptr<FSObject>[]> root_contents(contents);
    std::size_t pendingDirectoryCapacity = 8;
    std::size_t pendingDirectoryCount = 1;
    std::unique_ptr<PendingDirectoryCopy[]> pendingDirectories = std::make_unique<PendingDirectoryCopy[]>(pendingDirectoryCapacity);
    pendingDirectories[0] = {&other, this};

    std::size_t pendingSymlinkCapacity = 8;
    std::size_t pendingSymlinkCount = 0;
    std::unique_ptr<PendingSymlinkCopy[]> pendingSymlinks = std::make_unique<PendingSymlinkCopy[]>(pendingSymlinkCapacity);

    std::size_t cloneCapacity = 8;
    std::size_t cloneCount = 0;
    std::unique_ptr<CloneRecord[]> clones = std::make_unique<CloneRecord[]>(cloneCapacity);

    auto createDirectoryClone = [](const Directory& source) {
        std::shared_ptr<Directory> clone = std::make_shared<Directory>(source.getName());
        clone->size = source.size;
        if (clone->capacity != source.capacity) {
            delete[] clone->contents;
            clone->capacity = source.capacity;
            clone->contents = new std::shared_ptr<FSObject>[clone->capacity];
        }
        return clone;
    };
    auto appendPendingDirectory = [&](const Directory* source, Directory* clone) {
        if (pendingDirectoryCount == pendingDirectoryCapacity) {
            growArray(pendingDirectories, pendingDirectoryCapacity);
            pendingDirectoryCapacity *= 2;
        }
        pendingDirectories[pendingDirectoryCount++] = {source, clone};
    };
    auto appendPendingSymlink = [&](const Symlink* source, const std::shared_ptr<Symlink>& clone) {
        if (pendingSymlinkCount == pendingSymlinkCapacity) {
            growArray(pendingSymlinks, pendingSymlinkCapacity);
            pendingSymlinkCapacity *= 2;
        }
        pendingSymlinks[pendingSymlinkCount++] = {source, clone};
    };
    auto appendClone = [&](const FSObject* source, const std::shared_ptr<FSObject>& clone) {
        if (cloneCount == cloneCapacity) {
            growArray(clones, cloneCapacity);
            cloneCapacity *= 2;
        }
        clones[cloneCount++] = {source, clone};
    };
    auto findClone = [&](const FSObject* source) -> std::shared_ptr<FSObject> {
        for (std::size_t i = 0; i < cloneCount; ++i) {
            if (clones[i].source == source) return clones[i].clone;
        }
        return std::shared_ptr<FSObject>();
    };

    while (pendingDirectoryCount > 0) {
        PendingDirectoryCopy pending = pendingDirectories[--pendingDirectoryCount];

        for (std::size_t i = 0; i < pending.source->size; ++i) {
            const std::shared_ptr<FSObject>& sourceEntry = pending.source->contents[i];
            if (!sourceEntry) {
                pending.clone->contents[i].reset();
                continue;
            }

            if (Directory* sourceDirectory = dynamic_cast<Directory*>(sourceEntry.get())) {
                std::shared_ptr<Directory> clonedDirectory = createDirectoryClone(*sourceDirectory);
                pending.clone->contents[i] = clonedDirectory;
                appendClone(sourceDirectory, clonedDirectory);
                appendPendingDirectory(sourceDirectory, clonedDirectory.get());
            } else if (File* sourceFile = dynamic_cast<File*>(sourceEntry.get())) {
                std::shared_ptr<File> clonedFile = std::make_shared<File>(*sourceFile);
                pending.clone->contents[i] = clonedFile;
                appendClone(sourceFile, clonedFile);
            } else if (Symlink* sourceSymlink = dynamic_cast<Symlink*>(sourceEntry.get())) {
                std::shared_ptr<Symlink> clonedSymlink =
                    std::make_shared<Symlink>(sourceSymlink->getName(), std::shared_ptr<FSObject>());
                pending.clone->contents[i] = clonedSymlink;
                appendClone(sourceSymlink, clonedSymlink);
                appendPendingSymlink(sourceSymlink, clonedSymlink);
            } else {
                throw std::logic_error("Unknown FSObject type during Directory copy");
            }
        }
    }

    for (std::size_t i = 0; i < pendingSymlinkCount; ++i) {
        const PendingSymlinkCopy& pending = pendingSymlinks[i];
        std::shared_ptr<FSObject> target = pending.source->getTarget().lock();
        if (!target) continue;

        std::shared_ptr<FSObject> clonedTarget = findClone(target.get());
        if (clonedTarget) {
            pending.clone->setTarget(clonedTarget);
            continue;
        }

        pending.clone->setTarget(target);
    }

    root_contents.release();
}

Directory::~Directory() {
    if (!contents) return;

    std::size_t pendingDirectoryCapacity = 8;
    std::size_t pendingDirectoryCount = 0;
    std::unique_ptr<std::shared_ptr<Directory>[]> pendingDirectories =
        std::make_unique<std::shared_ptr<Directory>[]>(pendingDirectoryCapacity);

    std::size_t traversalStackCapacity = 8;
    std::size_t traversalStackCount = 1;
    std::unique_ptr<Directory*[]> traversalStack = std::make_unique<Directory*[]>(traversalStackCapacity);
    traversalStack[0] = this;

    while (traversalStackCount > 0) {
        Directory* current = traversalStack[--traversalStackCount];

        std::shared_ptr<FSObject>* currentContents = current->contents;
        const std::size_t currentSize = current->size;
        current->contents = nullptr;
        current->size = 0;
        current->capacity = 0;

        for (std::size_t i = 0; currentContents && i < currentSize; ++i) {
            if (Directory* childDirectory = dynamic_cast<Directory*>(currentContents[i].get());
                childDirectory != nullptr && currentContents[i].use_count() == 1) {
                if (pendingDirectoryCount == pendingDirectoryCapacity) {
                    growArray(pendingDirectories, pendingDirectoryCapacity);
                    pendingDirectoryCapacity *= 2;
                }
                if (traversalStackCount == traversalStackCapacity) {
                    growArray(traversalStack, traversalStackCapacity);
                    traversalStackCapacity *= 2;
                }
                pendingDirectories[pendingDirectoryCount] =
                    std::static_pointer_cast<Directory>(currentContents[i]);
                traversalStack[traversalStackCount++] = pendingDirectories[pendingDirectoryCount].get();
                ++pendingDirectoryCount;
                currentContents[i].reset();
            } else {
                currentContents[i].reset();
            }
        }

        delete[] currentContents;
    }

    delete[] contents;
}

void Directory::resizeContents() {
    std::shared_ptr<FSObject> *new_contents = new std::shared_ptr<FSObject>[capacity * 2];
    std::move(contents, contents + size, new_contents);
    delete[] contents;
    contents = new_contents;
    capacity *= 2;
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
