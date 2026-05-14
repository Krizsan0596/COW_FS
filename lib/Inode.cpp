#include "Inode.hpp"
#include "Block.hpp"
#include "Util.hpp"
#include <cstddef>
#include <memory>
#include <algorithm>

Inode::Inode(const std::string& data)
    : size(data.size()), blocks(std::make_unique<std::shared_ptr<Block>[]>((size + BLOCK_SIZE - 1) / BLOCK_SIZE)) {
    for (size_t i = 0; i < size; i += BLOCK_SIZE) {
        std::string chunk = data.substr(i, BLOCK_SIZE);
        blocks[i/BLOCK_SIZE] = std::make_shared<Block>(chunk);
    }
}

Inode::Inode(const Inode& other)
    : size(other.size), blocks(std::make_unique<std::shared_ptr<Block>[]>((size + BLOCK_SIZE - 1) / BLOCK_SIZE)) {
    for (std::size_t i = 0; i < (size + BLOCK_SIZE - 1) / BLOCK_SIZE; i++) {
        blocks[i] = other.blocks[i];
    }
}

std::string Inode::read() const {
    std::string output;
    for (size_t i = 0; i < (size + BLOCK_SIZE - 1) / BLOCK_SIZE; i++) {
        output += blocks[i]->read();
    }
    output.resize(size);
    return output;
}

void Inode::write(const std::string& data) {
    size_t current_block_count = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    size_t new_block_count = (data.size() + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if (new_block_count != current_block_count) {
        blocks = resizeArray(std::move(blocks), std::min(current_block_count, new_block_count), new_block_count);
    }
    size = data.size();

    for (size_t i = 0; i < size; i += BLOCK_SIZE) {
        std::string chunk = data.substr(i, BLOCK_SIZE);
        size_t index = i / BLOCK_SIZE;
        if (!blocks[index]) blocks[index] = std::make_shared<Block>(chunk);
        else {
            if (chunk.size() < BLOCK_SIZE) chunk.resize(BLOCK_SIZE, '\0');
            blocks[index] = blocks[index]->write(chunk.data());
        }
    }
}

void Inode::clear() {
    blocks = nullptr;
    size = 0;
}
