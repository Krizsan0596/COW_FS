#pragma once

#include <memory>
#include <string>

#include "FSObject.hpp"
#include "Directory.hpp"
#include "SnapshotManager.hpp"

#define MAX_PATH_DEPTH 100

class Dispatcher {
private:
    std::shared_ptr<Directory> root;
    std::unique_ptr<SnapshotManager> snapshotManager;

    std::shared_ptr<FSObject> resolvePath(const std::string& path) const;
    std::string stripTrailingSlashes(std::string path) const;
public:
    Dispatcher();
    void route();

    void read(const std::string& path) const;
    void write(const std::string& path, const std::string& data);
    void rm(const std::string& path);
    void slink(const std::string& dstPath, const std::string& srcPath);
    void hlink(const std::string& dstPath, const std::string& srcPath);
    void mkdir(const std::string& path);
    void rmdir(const std::string& path);
    void ls(const std::string& path) const;
    void createSnapshot() const;
    void restoreSnapshot(int index);
};
