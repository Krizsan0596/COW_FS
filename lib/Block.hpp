#pragma once

#include <memory>
#include <string>

class Block {
private:
    std::string data;

public:
    std::string read() const;
    std::shared_ptr<Block> write(const std::string& data);
};
