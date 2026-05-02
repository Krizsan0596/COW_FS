#include "SnapshotManager.hpp"
#include "Directory.hpp"
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>


void SnapshotManager::createSnapshot(const Dispatcher& globalDispatch) {
    if (count < SNAPSHOT_COUNT) {
        snapshots[count] = std::make_unique<Directory>(*globalDispatch.root);
        count++;
    } else {
        for (size_t i = 0; i < SNAPSHOT_COUNT - 1; i++) {
            snapshots[i] = std::move(snapshots[i + 1]);
        }
        snapshots[SNAPSHOT_COUNT - 1] = std::make_unique<Directory>(*globalDispatch.root);
    }
}

void SnapshotManager::restoreSnapshot(int index, Dispatcher& globalDispatch) const {
    if (index < 0 or index > 4) throw std::runtime_error("Snapshot index out of range");
    globalDispatch.root = std::make_unique<Directory>(*snapshots[index]);
}
