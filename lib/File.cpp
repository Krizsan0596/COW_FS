#include "File.hpp"
#include "FSObject.hpp"
#include <memory>

File::File(const std::string& fileName) : File(fileName, "") {}

File::File(const std::string& fileName, const std::string& data) : FSObject(fileName), inode(std::make_shared<Inode>(data)) {}

File::File(const std::string& fileName, const File& other)
    : FSObject(fileName),
      std::enable_shared_from_this<File>(),
      inode(other.inode) {}

File::File(const File& other)
    : FSObject(other),
      std::enable_shared_from_this<File>(),
      inode(std::make_shared<Inode>(*other.inode)) {}

std::shared_ptr<FSObject> File::resolve(int) {
    return shared_from_this();
}

std::string File::read() const {
    return inode->read();
}

void File::write(const std::string& data) {
    inode->write(data);
}
