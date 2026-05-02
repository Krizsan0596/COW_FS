#pragma once

#include <memory>
#include <string>

class SnapshotManager;

class Directory;

class Dispatcher {
private:
    std::unique_ptr<Directory> root;
public:
    void route();

    void read(const std::string& path);
    void write(const std::string& path, const std::string& data);
    void rm(const std::string& path);
    void slink(const std::string& dstPath, const std::string& srcPath);
    void hlink(const std::string& dstPath, const std::string& srcPath);
    void mkdir(const std::string& path);
    void rmdir(const std::string& path);
    void ls(const std::string& path);
    void createSnapshot();
    void restoreSnapshot(int index);

    friend class SnapshotManager;
};
