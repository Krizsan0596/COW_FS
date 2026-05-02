#include "Directory.hpp"
#include "FSObject.hpp"
#include "File.hpp"
#include "Symlink.hpp"
#include <cstddef>
#include <memory>
#include <stdexcept>

Directory::Directory(const std::string& dirName) : FSObject(dirName), capacity(8), size(0), contents(new std::shared_ptr<FSObject>[capacity]) {}

Directory::Directory(const Directory& other)
    : FSObject(other),
      std::enable_shared_from_this<Directory>(),
      size(other.size),
      capacity(other.capacity),
      contents(new std::shared_ptr<FSObject>[capacity]) {
    for (std::size_t i = 0; i < size; i++) {
        if (!other.contents[i]) {
            contents[i].reset();
            continue;
        }
        if (Directory* dir = dynamic_cast<Directory*>(other.contents[i].get())) {
            contents[i] = std::make_shared<Directory>(*dir);
        } else if (File* file = dynamic_cast<File*>(other.contents[i].get())) {
            contents[i] = std::make_shared<File>(*file);
        } else if (Symlink* symlink = dynamic_cast<Symlink*>(other.contents[i].get())) {
            contents[i] = std::make_shared<Symlink>(*symlink);
        } else {
            throw std::logic_error("Unknown FSObject type during Directory copy");
        }
    }
}

Directory::~Directory() {
    delete[] contents;
}

std::shared_ptr<File> Directory::touch(const std::string& child) {
    if (size == capacity) {
        std::shared_ptr<FSObject> *new_contents = new std::shared_ptr<FSObject>[capacity * 2];
        std::move(contents, contents + size, new_contents);
        delete[] contents;
        contents = new_contents;
        capacity *= 2;
    }
    std::shared_ptr<File> new_file = std::make_shared<File>(child);
    contents[size++] = new_file;
    return new_file;
}

void Directory::mkdir(const std::string& child) {
    if (size == capacity) {
        std::shared_ptr<FSObject> *new_contents = new std::shared_ptr<FSObject>[capacity * 2];
        std::move(contents, contents + size, new_contents);
        delete[] contents;
        contents = new_contents;
        capacity *= 2;
    }
    contents[size++] = std::make_shared<Directory>(child);
}

std::shared_ptr<FSObject> Directory::resolve(int) {
    return shared_from_this();
}

void Directory::removeDir(const std::string& child) {
        std::shared_ptr<FSObject>& dir = get(child);
        if (dynamic_cast<Directory*>(dir.get())) dir.reset();
        else throw std::runtime_error("Not a directory");
}

void Directory::removeFile(const std::string& child) {
    std::shared_ptr<FSObject>& file = get(child);
    if (dynamic_cast<File*>(file.get()) or dynamic_cast<Symlink*>(file.get())) file.reset();
    else throw std::runtime_error("Is a directory");
}


std::shared_ptr<FSObject>& Directory::get(const std::string& child) {
    for (size_t i = 0; i < size; i++) {
        if (contents[i]->getName() == child) return contents[i];
    }
    throw std::runtime_error("No such file or directory");
}
