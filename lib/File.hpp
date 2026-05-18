#pragma once

#include "FSObject.hpp"
#include "Inode.hpp"
#include "Directory.hpp"

#include <memory>

class Inode;

class File : public FSObject, public std::enable_shared_from_this<File> {
private:
    std::shared_ptr<Inode> inode;

public:
    File() = delete;
    explicit File(const std::string& fileName);
    File(const std::string& fileName, const std::string& data);
    explicit File(const File& other);
    File(const std::string& fileName, const File& other);
    [[nodiscard]] std::shared_ptr<FSObject> resolve(int depth = 0) noexcept override;
    [[nodiscard]] std::string read() const;
    void write(const std::string& data);
    friend Directory::Directory(const Directory& other);
};
