#include "File.hpp"
#include "Inode.hpp"
#include "Directory.hpp"
#include "Symlink.hpp"
#include "Dispatcher.hpp"
#include "SnapshotManager.hpp"
#include "Util.hpp"
#ifdef CPORTA
#include "gtest_lite.h"
#endif
#include <iostream>
#include <string>
#include <sstream>

int main() {
#ifdef CPORTA
    // 1. Inode Tests
    TEST(InodeTest, BasicReadWrite) {
        Inode inode("Hello, world!");
        EXPECT_EQ("Hello, world!", inode.read());
        
        inode.write("New data");
        EXPECT_EQ("New data", inode.read());
        
        std::string largeData(1000, 'A');
        inode.write(largeData);
        EXPECT_EQ(largeData, inode.read());
    } END

    // 2. File Tests
    TEST(FileTest, BasicOperations) {
        File file("test.txt", "Initial content");
        EXPECT_EQ("test.txt", file.getName());
        EXPECT_EQ("Initial content", file.read());
        
        file.write("Updated content");
        EXPECT_EQ("Updated content", file.read());
    } END

    TEST(FileTest, CopyBehavior) {
        File original("original.txt", "Shared content");
        File copy = original;
        
        EXPECT_EQ(original.read(), copy.read());
        
        copy.write("Modified content");
        
        EXPECT_EQ("Shared content", original.read());
        EXPECT_EQ("Modified content", copy.read());
    } END

    // 3. Directory Tests
    TEST(DirectoryTest, BasicOperations) {
        Directory dir("root");
        dir.mkdir("subdir");
        
        std::shared_ptr<FSObject> obj = dir.get("subdir");
        EXPECT_EQ("subdir", obj->getName());
        EXPECT_NE(nullptr, dynamic_cast<Directory*>(obj.get()));
        
        EXPECT_THROW(dir.get("nonexistent"), std::runtime_error&);
    } END

    TEST(DirectoryTest, RecursiveCopy) {
        Directory root("root");
        root.mkdir("subdir");
        std::shared_ptr<File> file = root.touch("file.txt");
        root.ln("link", file);
        
        Directory copy = root;
        EXPECT_EQ("subdir", copy.get("subdir")->getName());
        EXPECT_EQ("link", copy.get("link")->getName());
        
        // Check if symlink in copy resolves to the file in the copy (deep copy/re-linking)
        auto link = dynamic_cast<Symlink*>(copy.get("link").get());
        EXPECT_NE(nullptr, link);
        EXPECT_EQ(copy.get("file.txt"), link->resolve());
        
        copy.mkdir("newdir");
        EXPECT_THROW(root.get("newdir"), std::runtime_error&);
        EXPECT_NE(nullptr, copy.get("newdir"));
    } END

    // 4. Dispatcher & Snapshot Tests
    TEST(DispatcherTest, PathResolutionAndBasicOps) {
        Dispatcher disp;
        disp.mkdir("/dir1");
        disp.write("/dir1/f1", "v1");
        
        disp.createSnapshot();
        
        disp.write("/dir1/f1", "v2");
        disp.mkdir("/dir2");
        
        disp.createSnapshot();
        
        disp.rmdir("/dir1");
        
        disp.restoreSnapshot(0);
        
        EXPECT_NO_THROW(disp.ls("/dir1"));
        EXPECT_THROW(disp.ls("/dir2"), FileSystemError&);
    } END

    TEST(DispatcherTest, CommandRouting) {
        Dispatcher disp;
        
        std::streambuf* orig_cin = std::cin.rdbuf();
        std::stringstream input;
        input << "mkdir /routed_dir\n";
        input << "write /routed_dir/note This is a test\n";
        input << "exit\n";
        std::cin.rdbuf(input.rdbuf());
        
        disp.route();
        
        std::cin.rdbuf(orig_cin);
        
        EXPECT_NO_THROW(disp.read("/routed_dir/note"));
    } END

    return gtest_lite::test.fail() ? 1 : 0;
#else
    Dispatcher dispatcher;
    dispatcher.route();
    return 0;
#endif
}
