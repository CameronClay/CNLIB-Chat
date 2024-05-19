#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <iostream>
#include "CNLIB/Includes.h"
#include "CNLIB/File.h"

using namespace testing;

static const LIB_TCHAR* TESTS_DIR = _T("tests_dir");

static const LIB_TCHAR* TEST_FOLDER = _T("test_folder");
static const LIB_TCHAR* TEST_FNAME = _T("test_file");
static const LIB_TCHAR* TEST_FNAME2 = _T("test_file2");

class FileTest : public testing::Test {
protected:
    FileTest() :
        testing::Test()
    {}

    static void RemoveFiles() {
        if(FileMisc::Exists(TEST_FNAME)) {
            FileMisc::Remove(TEST_FNAME);
        }
        if(FileMisc::Exists(TEST_FNAME2)) {
            FileMisc::Remove(TEST_FNAME2);
        }
    }

    static void SetUpTestSuite() {
        if(FileMisc::Exists(TESTS_DIR)) {
            FileMisc::SetCurDirectory(TESTS_DIR);
            RemoveFiles();
        }
        else {
            FileMisc::CreateFolder(TESTS_DIR);
            FileMisc::SetCurDirectory(TESTS_DIR);
        }
    }

    static void TearDownTestSuite() {
        FileMisc::SetCurDirectory(_T(".."));
        if(FileMisc::Exists(TESTS_DIR)) {
            FileMisc::RemoveFolder(TESTS_DIR);
        }
    }

    virtual void SetUp() override {

    }

    virtual void TearDown() override {
        RemoveFiles();
    }
};

TEST_F(FileTest, FileCreateRemove_Success)
{
    File file(TEST_FNAME, FILE_READ_ACCESS);
    file.Close();
    ASSERT_TRUE(FileMisc::Exists(TEST_FNAME));

    FileMisc::Remove(TEST_FNAME);
    ASSERT_FALSE(FileMisc::Exists(TEST_FNAME));
}

TEST_F(FileTest, FileIsOpen_Success)
{
    File file(TEST_FNAME, FILE_READ_ACCESS);
    ASSERT_TRUE(file.IsOpen());
    file.Close();
    ASSERT_FALSE(file.IsOpen());

    FileMisc::Remove(TEST_FNAME);
}

TEST_F(FileTest, FileReadWrite_Success)
{
    {
        File file(TEST_FNAME, FILE_READ_ACCESS | FILE_WRITE_ACCESS);
        file.WriteString(_T("Test string"));
        file.Close();
    }

    {
        std::tstring str;
        File file(TEST_FNAME, FILE_READ_ACCESS);
        file.ReadString(str);
        ASSERT_EQ(str, std::tstring(_T("Test string")));
        file.Close();
    }
}

TEST_F(FileTest, MoveOrRename_Success)
{
    File file(TEST_FNAME, FILE_WRITE_ACCESS);
    file.Close();
    ASSERT_TRUE(FileMisc::Exists(TEST_FNAME));

    FileMisc::MoveOrRename(TEST_FNAME, TEST_FNAME2);
    // std::cout << "Error: " << GetLastError() << "\n";

    ASSERT_FALSE(FileMisc::Exists(TEST_FNAME));
    ASSERT_TRUE(FileMisc::Exists(TEST_FNAME2));
    // FileMisc::Remove(TEST_FNAME2);
}

TEST_F(FileTest, CreateRemoveDirectory_Success)
{
    FileMisc::CreateFolder(TEST_FOLDER);
    ASSERT_TRUE(FileMisc::Exists(TEST_FOLDER));

    FileMisc::RemoveFolder(TEST_FOLDER);
    ASSERT_FALSE(FileMisc::Exists(TEST_FOLDER));
}
