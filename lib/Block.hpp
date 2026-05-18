#pragma once

#include <memory>
#include <string>

#define BLOCK_SIZE 512

class Block : public std::enable_shared_from_this<Block>{
private:
    char data[BLOCK_SIZE];

public:
    Block() = delete;
    explicit Block(const std::string& Data);
    [[nodiscard]] std::string read() const;
    [[nodiscard]] std::shared_ptr<Block> write(const char newData[BLOCK_SIZE]);
};
