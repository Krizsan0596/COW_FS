#include "Directory.hpp"
#include "FSObject.hpp"
#include "File.hpp"
#include "Symlink.hpp"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>

Directory::Directory(const std::string& dirName) : FSObject(dirName), size(0), capacity(8), contents(new std::shared_ptr<FSObject>[capacity]) {}

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

void Directory::resizeContents() {
    std::shared_ptr<FSObject> *new_contents = new std::shared_ptr<FSObject>[capacity * 2];
    std::move(contents, contents + size, new_contents);
    delete[] contents;
    contents = new_contents;
    capacity *= 2;
}

std::shared_ptr<File> Directory::touch(const std::string& child) {
    if (size == capacity) {
        resizeContents();
    }
    std::shared_ptr<File> new_file = std::make_shared<File>(child);
    contents[size++] = new_file;
    return new_file;
}

std::shared_ptr<File> Directory::touch(const std::string& child, const File& source) {
    if (size == capacity) {
        resizeContents();
    }
    std::shared_ptr<File> new_file = std::make_shared<File>(child, source);
    contents[size++] = new_file;
    return new_file;
}

void Directory::mkdir(const std::string& child) {
    if (size == capacity) {
        resizeContents();
    }
    contents[size++] = std::make_shared<Directory>(child);
}

void Directory::ln(const std::string& child, const std::shared_ptr<FSObject>& target) {
    if (size == capacity) {
        resizeContents();
    }
    contents[size++] = std::make_shared<Symlink>(child, target);
}

std::shared_ptr<FSObject> Directory::resolve(int) {
    return shared_from_this();
}

void Directory::list() {
    for (size_t i = 0; i < size; i++) {
        if (!contents[i]) continue;
        if (auto dir = dynamic_cast<Directory*>(contents[i].get())) {
            std::cout << dir->getName() << "/\n";
        }
        else if (auto link = dynamic_cast<Symlink*>(contents[i].get())) {
            if (auto dir = dynamic_cast<Directory*>(link->resolve().get())) {
                std::cout << link->getName() << "/\n";
            }
            else std::cout << contents[i]->getName() << '\n';
        }
        else std::cout << contents[i]->getName() << '\n';
    }
}

void Directory::removeDir(const std::string& child) {
    for (size_t i = 0; i < size; ++i) {
        if (!contents[i]) continue;
        if (contents[i]->getName() == child) {
            if (!dynamic_cast<Directory*>(contents[i].get()))
                throw std::runtime_error("Not a directory");
            for (size_t j = i; j + 1 < size; ++j) {
                contents[j] = std::move(contents[j + 1]);
            }
            contents[size - 1].reset();
            --size;
            return;
        }
    }
    throw std::runtime_error("No such file or directory");
}

void Directory::removeFile(const std::string& child) {
    for (size_t i = 0; i < size; ++i) {
        if (!contents[i]) continue;
        if (contents[i]->getName() == child) {
            if (dynamic_cast<Directory*>(contents[i].get()))
                throw std::runtime_error("Is a directory");
            for (size_t j = i; j + 1 < size; ++j) {
                contents[j] = std::move(contents[j + 1]);
            }
            contents[size - 1].reset();
            --size;
            return;
        }
    }
    throw std::runtime_error("No such file or directory");
}


std::shared_ptr<FSObject>& Directory::get(const std::string& child) {
    for (size_t i = 0; i < size; i++) {
        if (contents[i]->getName() == child) return contents[i];
    }
    throw std::runtime_error("No such file or directory");
}
