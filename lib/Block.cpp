#include "Block.hpp"
#include <memory>

Block::Block(const std::string& Data) : data(Data) {}

std::string Block::read() const {
    return data;
}

std::shared_ptr<Block> Block::write(const std::string& data) {
    if (data == this->data) return shared_from_this();
    std::shared_ptr<Block> new_block = std::make_shared<Block>(data);
    return new_block;
}
