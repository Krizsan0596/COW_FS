#include "File.hpp"
#include "FSObject.hpp"
#include <memory>

File::File(const std::string& fileName) : File(fileName, "") {}

File::File(const std::string& fileName, const std::string& data) : inode(std::make_shared<Inode>(data)), FSObject(fileName) {}

File::File(const std::string& fileName, const File& other)
    : FSObject(fileName),
      std::enable_shared_from_this<File>(),
      inode(other.inode) {}

std::shared_ptr<FSObject> File::resolve(int) {
    return shared_from_this();
}

std::string File::read() const {
    return inode->read();
}

void File::write(const std::string& data) {
    inode->write(data);
}
