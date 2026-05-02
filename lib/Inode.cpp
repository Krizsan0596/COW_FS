#include "Inode.hpp"
#include "Block.hpp"
#include <cstddef>
#include <memory>
#include <algorithm>

Inode::Inode(const std::string& data) : size(data.size()), blocks(new std::shared_ptr<Block>[(size + BLOCK_SIZE - 1) / BLOCK_SIZE]) {
    for (size_t i = 0; i < size; i += BLOCK_SIZE) {
        std::string chunk = data.substr(i, BLOCK_SIZE);
        blocks[i/BLOCK_SIZE] = std::make_shared<Block>(chunk);
    }
}

Inode::Inode(const Inode& other) : size(other.size), blocks(new std::shared_ptr<Block>[(size + BLOCK_SIZE - 1) / BLOCK_SIZE]) {
    for (std::size_t i = 0; i < (size + BLOCK_SIZE - 1) / BLOCK_SIZE; i++) {
        blocks[i] = other.blocks[i];
    }
}

Inode::~Inode() {
    delete[] blocks;
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
        std::shared_ptr<Block> *new_blocks = new std::shared_ptr<Block>[new_block_count];
        std::move(blocks, blocks + std::min(current_block_count, new_block_count), new_blocks);
        delete[] blocks;
        blocks = new_blocks;
    }
    size = data.size();

    for (size_t i = 0; i < size; i += BLOCK_SIZE) {
        std::string chunk = data.substr(i, BLOCK_SIZE);
        size_t index = i / BLOCK_SIZE;
        if (!blocks[index]) blocks[index] = std::make_shared<Block>(chunk);
        else blocks[index] = blocks[index]->write(chunk.data());
    }
}

void Inode::clear() {
    delete[] blocks;
    blocks = nullptr;
    size = 0;
}
