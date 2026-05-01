#pragma once

#include <memory>
#include <string>

class Block : public std::enable_shared_from_this<Block>{
private:
    const std::string data;

public:
    Block() = delete;
    Block(const std::string& Data);
    std::string read() const;
    std::shared_ptr<Block> write(const std::string& data);
};
