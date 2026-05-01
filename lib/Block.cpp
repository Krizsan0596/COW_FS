#include "Block.hpp"
#include <algorithm>
#include <memory>
#include <stdexcept>

Block::Block(const std::string& Data) : data{} {
    if (Data.size() > 512) throw std::invalid_argument("Block received >512 bytes of data.");
    std::copy(Data.data(), Data.data() + Data.size(), data);
}

std::string Block::read() const {
    return std::string(data, BLOCK_SIZE);
}

std::shared_ptr<Block> Block::write(const char data[BLOCK_SIZE]) {
    if (data == this->data) return shared_from_this();
    std::shared_ptr<Block> new_block = std::make_shared<Block>(std::string(data, BLOCK_SIZE));
    return new_block;
}
