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

std::shared_ptr<Block> Block::write(const char newData[BLOCK_SIZE]) {
    char paddedData[BLOCK_SIZE]{};
    size_t inputLength = 0;
    while (inputLength < BLOCK_SIZE && newData[inputLength] != '\0') {
        inputLength++;
    }
    std::memcpy(paddedData, newData, inputLength);

    if (std::memcmp(paddedData, this->data, BLOCK_SIZE) == 0) return shared_from_this();
    return std::make_shared<Block>(std::string(paddedData, BLOCK_SIZE));
}
