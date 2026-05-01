#pragma once

#include "StorageObject.hpp"

#include <cstddef>
#include <memory>
#include <string>

class Block;

class Inode : public StorageObject {
private:
    std::size_t size;
    std::shared_ptr<Block> *blocks;

public:
    Inode() = delete;
    Inode(const std::string& data);
    Inode(const Inode& other);
    ~Inode();
    std::string read() const override;
    void write(const std::string& data) override;
    void clear() override;
};
