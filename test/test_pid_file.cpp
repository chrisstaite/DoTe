
#include "pid_file.h"

#include <gtest/gtest.h>

#include <unistd.h>

namespace dote {

TEST(TestPidFile, NoFile)
{
    PidFile file("");
    EXPECT_TRUE(file.valid());
}

TEST(TestPidFile, CreateFile)
{
    PidFile file("test.pid");
    EXPECT_TRUE(file.valid());
    EXPECT_EQ(0, access("test.pid", F_OK));
}

TEST(TestPidFile, FileDeleted)
{
    {
        PidFile file("test.pid");
        EXPECT_TRUE(file.valid());
    }
    EXPECT_NE(0, access("test.pid", F_OK));
}

TEST(TestPidFile, DoubleLockFail)
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
        PidFile file("test.pid");
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
    PidFile file2("test.pid");
    EXPECT_FALSE(file2.valid());
    EXPECT_EQ(1, write(toChild[1], &c, 1));
    (void) close(toParent[0]);
    (void) close(toChild[1]);
}

}  // namespace dote
