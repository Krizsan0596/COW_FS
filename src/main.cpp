#include "../lib/Dispatcher.hpp"
#ifdef CPORTA
#include <sstream>
#include "../lib/File.hpp"
#include "../lib/Inode.hpp"
#include "../lib/Directory.hpp"
#include "../lib/Symlink.hpp"
#include "../lib/Util.hpp"
#include "../lib/SnapshotManager.hpp"
#include "gtest_lite.h"
#endif


int main() {
#ifdef CPORTA
    // 0. Util Tests
    TEST(UtilTest, GrowByHalf) {
        EXPECT_EQ(8U, growByHalf(0));
        EXPECT_EQ(12U, growByHalf(8));
        EXPECT_EQ(2U, growByHalf(1));
        EXPECT_EQ(3U, growByHalf(2));
    } END

    TEST(UtilTest, RemapArrayBasic) {
        RemapArray<int> arr = makeRemapArray<int>(5);
        EXPECT_EQ(0U, arr.count);
        EXPECT_EQ(5U, arr.capacity);
        EXPECT_NE(nullptr, arr.data.get());

        RemapArray<int> emptyArr = makeRemapArray<int>(0);
        EXPECT_EQ(0U, emptyArr.count);
        EXPECT_EQ(0U, emptyArr.capacity);
        EXPECT_EQ(nullptr, emptyArr.data.get());
    } END

    TEST(UtilTest, ResizeArray) {
        auto arr = std::make_unique<int[]>(2);
        arr[0] = 10;
        arr[1] = 20;
        
        auto arr2 = resizeArray(std::move(arr), 2, 5);
        EXPECT_EQ(10, arr2[0]);
        EXPECT_EQ(20, arr2[1]);
        EXPECT_EQ(nullptr, arr.get());

        auto arr3 = resizeArray(std::move(arr2), 2, 0);
        EXPECT_EQ(nullptr, arr3.get());
    } END

    TEST(UtilTest, ResizeRemapArray) {
        RemapArray<int> arr = makeRemapArray<int>(2);
        arr.data[0] = 1;
        arr.data[1] = 2;
        arr.count = 2;

        resizeRemapArray(arr);
        EXPECT_EQ(3U, arr.capacity);
        EXPECT_EQ(2U, arr.count);
        EXPECT_EQ(1, arr.data[0]);
        EXPECT_EQ(2, arr.data[1]);

        resizeRemapArray(arr);
        EXPECT_EQ(4U, arr.capacity);
        EXPECT_EQ(1, arr.data[0]);
        EXPECT_EQ(2, arr.data[1]);
    } END

    TEST(UtilTest, FileSystemError) {
        FileSystemError err("Something went wrong", "/path/to/file");
        EXPECT_STREQ("Something went wrong", err.what());
        EXPECT_EQ(std::string("/path/to/file"), err.path());
    } END

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

    TEST(DispatcherTest, PathNormalizationAndNavigation) {
        Dispatcher disp;
        disp.mkdir("/dir1");
        disp.write("/dir1/file1", "content");
        
        EXPECT_NO_THROW(disp.ls("/dir1/"));
        EXPECT_NO_THROW(disp.read("/dir1/file1///"));
        
        EXPECT_NO_THROW(disp.ls("/dir1/."));
        EXPECT_NO_THROW(disp.ls("/dir1/.."));
        EXPECT_NO_THROW(disp.read("/dir1/../dir1/file1"));
        
        EXPECT_NO_THROW(disp.ls("///dir1//"));
    } END

    TEST(DispatcherTest, ErrorHandling) {
        Dispatcher disp;
        
        EXPECT_THROW(disp.mkdir("relative"), FileSystemError&);
        
        EXPECT_THROW(disp.mkdir("/a/b"), FileSystemError&);
        EXPECT_THROW(disp.write("/a/b", "data"), FileSystemError&);
        
        disp.mkdir("/dir");
        EXPECT_THROW(disp.mkdir("/dir"), FileSystemError&);
        
        disp.write("/dir/file", "data");
        EXPECT_THROW(disp.read("/dir"), FileSystemError&);
        EXPECT_THROW(disp.write("/dir", "data"), FileSystemError&);
        EXPECT_THROW(disp.ls("/dir/file"), FileSystemError&);
        
        EXPECT_THROW(disp.read("/nonexistent"), FileSystemError&);
        EXPECT_THROW(disp.rm("/nonexistent"), FileSystemError&);
        EXPECT_THROW(disp.rmdir("/nonexistent"), FileSystemError&);
    } END

    TEST(DispatcherTest, LinkOperations) {
        Dispatcher disp;
        disp.mkdir("/dir");
        disp.write("/dir/file", "data");
        
        disp.slink("/link", "/dir/file");
        EXPECT_NO_THROW(disp.read("/link"));
        
        EXPECT_THROW(disp.slink("/badlink", "/nonexistent"), FileSystemError&);
        
        disp.hlink("/hlink", "/dir/file");
        EXPECT_NO_THROW(disp.read("/hlink"));
        
        EXPECT_THROW(disp.hlink("/hdir", "/dir"), FileSystemError&);
        
        disp.write("/hlink", "new data");
        EXPECT_NO_THROW(disp.read("/dir/file"));
        EXPECT_NO_THROW(disp.read("/hlink"));
    } END

    TEST(DispatcherTest, SnapshotFIFO) {
        Dispatcher disp;
        for (int i = 0; i < 5; ++i) {
            std::string name = "/dir" + std::to_string(i);
            disp.mkdir(name);
            disp.createSnapshot();
            disp.rmdir(name);
        }
        
        disp.mkdir("/dir5");
        disp.createSnapshot();
        
        disp.restoreSnapshot(0);
        EXPECT_NO_THROW(disp.ls("/dir1"));
        EXPECT_THROW(disp.ls("/dir0"), FileSystemError&);
        
        EXPECT_THROW(disp.restoreSnapshot(-1), std::runtime_error&);
        EXPECT_THROW(disp.restoreSnapshot(5), std::runtime_error&);
    } END

    TEST(DispatcherTest, SnapshotDataIntegrity) {
        Dispatcher disp;
        disp.mkdir("/dir");
        disp.write("/dir/file.txt", "Original Data");
        
        disp.createSnapshot(); // Snapshot 0: "Original Data"
        
        disp.write("/dir/file.txt", "Modified Data");
        
        {
            std::stringstream ss;
            std::streambuf* old_cout = std::cout.rdbuf(ss.rdbuf());
            disp.read("/dir/file.txt");
            std::cout.rdbuf(old_cout);
            EXPECT_EQ("Modified Data\n", ss.str());
        }
        
        disp.restoreSnapshot(0);
        
        {
            std::stringstream ss;
            std::streambuf* old_cout = std::cout.rdbuf(ss.rdbuf());
            disp.read("/dir/file.txt");
            std::cout.rdbuf(old_cout);
            EXPECT_EQ("Original Data\n", ss.str());
        }
    } END

    TEST(DispatcherTest, RouteReadWrite) {
        Dispatcher disp;
        std::streambuf* orig_cin = std::cin.rdbuf();
        std::stringstream input;
        input << "write /f1 \"data\"\n";
        input << "read /f1\n";
        input << "exit\n";
        std::cin.rdbuf(input.rdbuf());
        
        std::stringstream out_ss;
        std::streambuf* orig_cout = std::cout.rdbuf(out_ss.rdbuf());
        
        disp.route();
        
        std::cin.rdbuf(orig_cin);
        std::cout.rdbuf(orig_cout);
        
        EXPECT_TRUE(out_ss.str().find("data") != std::string::npos);
    } END

    TEST(DispatcherTest, RouteLinks) {
        Dispatcher disp;
        std::streambuf* orig_cin = std::cin.rdbuf();
        std::stringstream input;
        input << "write /f1 \"content\"\n";
        input << "slink /slink /f1\n";
        input << "hlink /hlink /f1\n";
        input << "exit\n";
        std::cin.rdbuf(input.rdbuf());
        
        disp.route();
        std::cin.rdbuf(orig_cin);
        
        EXPECT_NO_THROW(disp.read("/slink"));
        EXPECT_NO_THROW(disp.read("/hlink"));
    } END

    TEST(DispatcherTest, RouteRmRmdir) {
        Dispatcher disp;
        std::streambuf* orig_cin = std::cin.rdbuf();
        std::stringstream input;
        input << "mkdir /dir\n";
        input << "write /dir/f1 \"data\"\n";
        input << "rm /dir/f1\n";
        input << "rmdir /dir\n";
        input << "exit\n";
        std::cin.rdbuf(input.rdbuf());
        
        disp.route();
        std::cin.rdbuf(orig_cin);
        
        EXPECT_THROW(disp.ls("/dir"), FileSystemError&);
    } END

    TEST(DispatcherTest, RouteSnapshots) {
        Dispatcher disp;
        std::streambuf* orig_cin = std::cin.rdbuf();
        std::stringstream input;
        input << "mkdir /dir\n";
        input << "createSnapshot\n";
        input << "rmdir /dir\n";
        input << "restoreSnapshot 0\n";
        input << "exit\n";
        std::cin.rdbuf(input.rdbuf());
        
        disp.route();
        std::cin.rdbuf(orig_cin);
        
        EXPECT_NO_THROW(disp.ls("/dir"));
    } END

    TEST(DispatcherTest, RouteErrors) {
        Dispatcher disp;
        std::streambuf* orig_cin = std::cin.rdbuf();
        std::stringstream input;
        input << "unknown_command\n";
        input << "mkdir relative\n";
        input << "mkdir /too /many /args\n";
        input << "exit\n";
        std::cin.rdbuf(input.rdbuf());
        
        std::stringstream err_ss;
        std::streambuf* orig_cerr = std::cerr.rdbuf(err_ss.rdbuf());
        
        disp.route();
        
        std::cin.rdbuf(orig_cin);
        std::cerr.rdbuf(orig_cerr);
        
        std::string errors = err_ss.str();
        EXPECT_TRUE(errors.find("Unknown command") != std::string::npos);
        EXPECT_TRUE(errors.find("Only absolute paths allowed") != std::string::npos);
        EXPECT_TRUE(errors.find("Too many arguments") != std::string::npos);
    } END

    TEST(DispatcherTest, PathResolutionEdgeCases) {
        Dispatcher disp;
        disp.mkdir("/dir");
        disp.write("/dir/file", "data");
        
        EXPECT_THROW(disp.ls("/dir/file/something"), FileSystemError&);
        
        disp.mkdir("/target");
        disp.write("/target/f", "content");
        disp.slink("/link_to_dir", "/target");
        
        std::stringstream ss;
        std::streambuf* old_cout = std::cout.rdbuf(ss.rdbuf());
        disp.read("/link_to_dir/f");
        std::cout.rdbuf(old_cout);
        EXPECT_EQ("content\n", ss.str());
    } END

    TEST(DispatcherTest, ErrorConditions) {
        Dispatcher disp;
        disp.mkdir("/dir");
        disp.write("/dir/file", "data");
        
        EXPECT_THROW(disp.write("/dir/file/f2", "data"), FileSystemError&);
        
        EXPECT_THROW(disp.mkdir("/dir/file/newdir"), FileSystemError&);
        
        EXPECT_THROW(disp.rmdir("/dir/file/somedir"), FileSystemError&);
        
        disp.write("/f1", "d1");
        disp.write("/f2", "d2");
        EXPECT_THROW(disp.slink("/f2", "/f1"), FileSystemError&);
        EXPECT_THROW(disp.hlink("/f2", "/f1"), FileSystemError&);
        
        EXPECT_THROW(disp.slink("/dir/file/link", "/f1"), FileSystemError&);
    } END

    TEST(DispatcherTest, QuotedTokens) {
        Dispatcher disp;
        std::streambuf* orig_cin = std::cin.rdbuf();
        std::stringstream input;
        input << "mkdir \"/dir with spaces\"\n";
        input << "write \"/dir with spaces/file\" \"content with spaces\"\n";
        input << "exit\n";
        std::cin.rdbuf(input.rdbuf());
        
        disp.route();
        std::cin.rdbuf(orig_cin);
        
        EXPECT_NO_THROW(disp.ls("/dir with spaces"));
    } END

    return gtest_lite::test.fail() ? 1 : 0;
#else
    Dispatcher dispatcher;
    dispatcher.route();
    return 0;
#endif
}
