#pragma once

#include <cstddef>
#include <string>

class StorageObject {
public:
    virtual ~StorageObject() = default;
    virtual std::string read(std::size_t& dataSize) = 0;
    virtual void write(const std::string& data) = 0;
    virtual void clear() = 0;
};
