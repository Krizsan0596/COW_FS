#pragma once
#include "Directory.hpp"
#include <cstddef>
#include <memory>

class Dispatcher;

#define SNAPSHOT_COUNT 5

class SnapshotManager {
private:
    std::unique_ptr<Directory> snapshots[SNAPSHOT_COUNT];
    size_t count = 0;
public:
    void createSnapshot(const Dispatcher& globalDispatch);
    void restoreSnapshot(size_t index, Dispatcher& globalDispatch) const;
};
