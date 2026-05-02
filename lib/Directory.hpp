#pragma once

#include "FSObject.hpp"
#include "File.hpp"

#include <memory>
#include <string>

class Directory : public FSObject, public std::enable_shared_from_this<Directory> {
private:
    size_t size;
    size_t capacity;
    std::shared_ptr<FSObject> *contents;
    std::shared_ptr<File> touch(const std::string& child);

public:
    Directory() = delete;
    Directory(const std::string& dirName);
    Directory(const Directory& other);
    Directory(Directory&& other) = delete;
    Directory& operator=(const Directory& other) = delete;
    Directory& operator=(Directory&& other) = delete;
    ~Directory();
    std::shared_ptr<FSObject> resolve(int depth) override;
    void list();
    std::shared_ptr<FSObject>& get(const std::string& child);
    void removeDir(const std::string& child);
    void removeFile(const std::string& child);
    void mkdir(const std::string& child);
};
