#pragma once

#include "FSObject.hpp"

#include <memory>
#include <string>
#include <vector>

class Directory : public FSObject {
private:
    std::vector<std::shared_ptr<FSObject>> contents;
    void touch(const std::string& child);

public:
    std::shared_ptr<FSObject> resolve(int depth) override;
    void list();
    std::shared_ptr<FSObject> get(const std::string& child);
    void removeDir();
    void removeFile();
    void mkdir(const std::string& child);
};
