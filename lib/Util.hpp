#pragma once

#include "FSObject.hpp"
#include "Inode.hpp"
#include "File.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>

class FileSystemError : public std::runtime_error {
private:
    std::string m_path;

public:
    FileSystemError(const std::string& what, const std::string& path)
        : std::runtime_error(what), m_path(path) {}

    const std::string& path() const noexcept {
        return m_path;
    }
};

struct symlink_remap {
    std::shared_ptr<FSObject> oldTarget;
    std::shared_ptr<FSObject> newTarget;
};

struct hardlink_remap {
    std::shared_ptr<Inode> oldInode;
    std::shared_ptr<File> newFile;
};

template <typename T>
struct RemapArray {
    std::unique_ptr<T[]> data;
    std::size_t count;
    std::size_t capacity;
};

inline std::size_t growByHalf(std::size_t capacity) {
    if (capacity == 0) {
        return 8;
    }

    std::size_t newCapacity = (capacity * 3) / 2;
    return newCapacity;
}

template <typename T>
RemapArray<T> makeRemapArray(std::size_t capacity = 8) {
    return {
        capacity == 0 ? nullptr : std::make_unique<T[]>(capacity),
        0,
        capacity
    };
}

template <typename T>
std::unique_ptr<T[]> resizeArray(std::unique_ptr<T[]> oldArray, std::size_t copyCount, std::size_t newCount) {
    if (newCount == 0) {
        return nullptr;
    }

    std::unique_ptr<T[]> newArray = std::make_unique<T[]>(newCount);
    if (oldArray && copyCount > 0) {
        std::move(oldArray.get(), oldArray.get() + copyCount, newArray.get());
    }
    return newArray;
}

template <typename T>
void resizeRemapArray(RemapArray<T>& remaps) {
    const std::size_t newCapacity = growByHalf(remaps.capacity);
    remaps.data = resizeArray(std::move(remaps.data), remaps.count, newCapacity);
    remaps.capacity = newCapacity;
}
