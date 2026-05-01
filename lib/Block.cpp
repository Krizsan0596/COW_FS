#include "Block.hpp"
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <cstring>

Block::Block(const std::string& Data) : data{} {
    if (Data.size() > BLOCK_SIZE)
        throw std::invalid_argument("Block received >" + std::to_string(BLOCK_SIZE) + " bytes of data.");
    std::copy(Data.data(), Data.data() + Data.size(), data);
}

std::string Block::read() const {
    return std::string(data, BLOCK_SIZE);
}

std::shared_ptr<Block> Block::write(const char data[BLOCK_SIZE]) {
    if (std::memcmp(data, this->data, BLOCK_SIZE) == 0) return shared_from_this();
    std::shared_ptr<Block> new_block = std::make_shared<Block>(std::string(data, BLOCK_SIZE));
    return new_block;
}
