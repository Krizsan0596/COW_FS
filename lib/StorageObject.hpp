#pragma once

#include <cstddef>
#include <string>

class StorageObject {
public:
    virtual ~StorageObject() = default;
    [[nodiscard]] virtual std::string read() const = 0;
    virtual void write(const std::string& data) = 0;
    virtual void clear() = 0;
};
