#pragma once

#include <array>
#include <memory>

class Directory;

class SnapshotManager {
private:
    std::array<std::unique_ptr<Directory>, 5> snapshots;

public:
    void createSnapshot(std::shared_ptr<Directory> root);
    void restoreSnapshot(int index);
};
