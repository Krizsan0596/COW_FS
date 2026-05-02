#pragma once
#include "Dispatcher.hpp"
#include "Directory.hpp"
#include <cstddef>
#include <memory>

#define SNAPSHOT_COUNT 5

class SnapshotManager {
private:
    std::unique_ptr<Directory> snapshots[SNAPSHOT_COUNT];
    size_t count = 0;
public:
    void createSnapshot(const Dispatcher& globalDispatch);
    void restoreSnapshot(int index, Dispatcher& globalDispatch) const;
};
