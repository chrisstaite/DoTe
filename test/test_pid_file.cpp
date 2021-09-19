
#include "pid_file.h"

#include <gtest/gtest.h>

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace dote {

class TestPidFile : public ::testing::Test
{
  public:
    TestPidFile() :
        m_pidFile(::testing::internal::TempDir() + "pidfile-XXXXXX")
    { }
    
    void SetUp()
    {
        int handle = mkstemp(&m_pidFile[0]);
        ASSERT_NE(-1, handle) << "Error creating temp file " << strerror(errno);
        (void) close(handle);
    }

    ~TestPidFile()
    {
        unlink(m_pidFile.c_str());
    }
    
  protected:
    std::string m_pidFile;
};

TEST_F(TestPidFile, NoFile)
{
    PidFile file("");
    EXPECT_TRUE(file.valid());
}

TEST_F(TestPidFile, CreateFile)
{
    PidFile file(m_pidFile);
    EXPECT_TRUE(file.valid());
    EXPECT_EQ(0, access(m_pidFile.c_str(), F_OK));
}

TEST_F(TestPidFile, FileDeleted)
{
    {
        PidFile file(m_pidFile);
        EXPECT_TRUE(file.valid());
    }
    EXPECT_NE(0, access(m_pidFile.c_str(), F_OK));
}

TEST_F(TestPidFile, DoubleLockFail)
{
    char c = 'a';
    int toChild[2];
    int toParent[2];
    ASSERT_EQ(0, pipe(toChild));
    ASSERT_EQ(0, pipe(toParent));
    if (fork() == 0)
    {
        // Child - lock pid file and wait for read
        (void) close(toChild[1]);
        (void) close(toParent[0]);
        PidFile file(m_pidFile);
        ASSERT_TRUE(file.valid());
        ASSERT_EQ(1, write(toParent[1], &c, 1));
        ASSERT_EQ(1, read(toChild[0], &c, 1));
        (void) close(toParent[1]);
        (void) close(toChild[0]);
        exit(0);
    }
    // Parent - wait for child and then try to lock again
    (void) close(toChild[0]);
    (void) close(toParent[1]);
    ASSERT_EQ(1, read(toParent[0], &c, 1));
    PidFile file2(m_pidFile);
    EXPECT_FALSE(file2.valid());
    EXPECT_EQ(1, write(toChild[1], &c, 1));
    (void) close(toParent[0]);
    (void) close(toChild[1]);
}

}  // namespace dote
