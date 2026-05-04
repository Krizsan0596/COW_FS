#pragma once

#include <stdexcept>
#include <string>

class FileSystemError : public std::runtime_error {
private:
    std::string m_path;

public:
    FileSystemError(const std::string& what, const std::string& path)
        : std::runtime_error(what), m_path(path) {}

    const std::string& path() const noexcept {
        return m_path;
    }
};
