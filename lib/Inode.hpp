#pragma once

#include "StorageObject.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class Block;

class Inode : public StorageObject {
private:
    std::size_t size;
    std::vector<std::shared_ptr<Block>> blocks;

public:
    std::string read(std::size_t& dataSize) override;
    void write(const std::string& data) override;
    void clear() override;
};
