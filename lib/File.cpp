#include "File.hpp"
#include <memory>

File::File(const std::string& data) : inode(std::make_shared<Inode>(data)) {}

std::shared_ptr<FSObject> File::resolve(int depth) {
    return shared_from_this();
}

std::string File::read() const {
    return inode->read();
}

void File::write(const std::string& data) {
    inode->write(data);
}
