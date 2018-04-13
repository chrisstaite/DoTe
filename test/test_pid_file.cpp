
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
    pipe(toChild);
    pipe(toParent);
    if (fork() == 0)
    {
        // Child - lock pid file and wait for read
        close(toChild[1]);
        close(toParent[0]);
        PidFile file("test.pid");
        write(toParent[1], &c, 1);
        read(toChild[0], &c, 1);
        close(toParent[1]);
        close(toChild[0]);
        exit(0);
    }
    // Parent - wait for child and then try to lock again
    close(toChild[0]);
    close(toParent[1]);
    read(toParent[0], &c, 1);
    PidFile file2("test.pid");
    EXPECT_FALSE(file2.valid());
    write(toChild[1], &c, 1);
    close(toParent[0]);
    close(toChild[1]);
}

}  // namespace dote
