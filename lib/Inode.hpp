#pragma once

#include "StorageObject.hpp"

#include <cstddef>
#include <memory>
#include <string>

class Block;

class Inode : public StorageObject {
private:
    std::size_t size;
    std::unique_ptr<std::shared_ptr<Block>[]> blocks;

public:
    Inode() = delete;
    explicit Inode(const std::string& data);
    Inode(const Inode& other);
    Inode& operator=(const Inode& other) = delete;
    Inode(Inode&& other) = delete;
    Inode& operator=(Inode&& other) = delete;
    ~Inode() = default;
    [[nodiscard]] std::string read() const override;
    void write(const std::string& data) override;
    void clear() noexcept override;
};
