#pragma once

#include "FSObject.hpp"

#include <memory>
#include <string>

class File;

class Directory : public FSObject, public std::enable_shared_from_this<Directory> {
private:
    size_t size;
    size_t capacity;
    std::unique_ptr<std::shared_ptr<FSObject>[]> contents;
    void resizeContents();

public:
    Directory() = delete;
    explicit Directory(const std::string& dirName);
    Directory(const Directory& other);
    Directory(Directory&& other) = delete;
    Directory& operator=(const Directory& other) = delete;
    Directory& operator=(Directory&& other) = delete;
    ~Directory();
    [[nodiscard]] std::shared_ptr<FSObject> resolve(int depth = 0) noexcept override;
    void list();
    [[nodiscard]] std::shared_ptr<FSObject>& get(const std::string& child);
    void removeDir(const std::string& child);
    void removeFile(const std::string& child);
    void mkdir(const std::string& child);
    void ln(const std::string& child, const std::shared_ptr<FSObject>& target);
    [[nodiscard]] std::shared_ptr<File> touch(const std::string& child);
    [[nodiscard]] std::shared_ptr<File> touch(const std::string& child, const File& source);
};
