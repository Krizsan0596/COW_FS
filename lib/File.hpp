#pragma once

#include "FSObject.hpp"
#include "Inode.hpp"

#include <memory>

class Inode;

class File : public FSObject, public std::enable_shared_from_this<File> {
private:
    std::shared_ptr<Inode> inode;

public:
    File() = delete;
    File(const std::string& fileName);
    File(const std::string& fileName, const std::string& data);
    File(const std::string& fileName, const File& other);
    std::shared_ptr<FSObject> resolve(int depth = 0) override;
    std::string read() const;
    void write(const std::string& data);
};
