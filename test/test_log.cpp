
#include "log.h"

#include <syslog.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace dote {

class MockLogger : public ILogger
{
  public:
    ~MockLogger() noexcept
    { }

    MOCK_METHOD2(log, void(int, const std::string&));
};

class TestLog : public ::testing::Test
{
  public:
    TestLog() :
        m_logger(std::make_shared<MockLogger>())
    {
        Log::setLogger(m_logger);
    }

    ~TestLog()
    {
        Log::setLogger(nullptr);
    }

  protected:
    std::shared_ptr<MockLogger> m_logger;
};

TEST_F(TestLog, LogToNull)
{
    Log::setLogger(nullptr);
    Log::critical << "Test";
}

TEST_F(TestLog, DebugLevel)
{
    EXPECT_CALL(*m_logger, log(LOG_DEBUG, "Test"))
#ifdef NDEBUG
        .Times(0);
#else
        .Times(1);
#endif
    Log::debug << "Test";
}

TEST_F(TestLog, InfoLevel)
{
    EXPECT_CALL(*m_logger, log(LOG_INFO, "Test"))
        .Times(1);
    Log::info << "Test";
}

TEST_F(TestLog, InfoLevelStringInt)
{
    EXPECT_CALL(*m_logger, log(LOG_INFO, "Test10"))
        .Times(1);
    Log::info << "Test" << 10;
}

TEST_F(TestLog, NoticeLevel)
{
    EXPECT_CALL(*m_logger, log(LOG_NOTICE, "Test"))
        .Times(1);
    Log::notice << "Test";
}

TEST_F(TestLog, WarningLevel)
{
    EXPECT_CALL(*m_logger, log(LOG_WARNING, "Test"))
        .Times(1);
    Log::warn << "Test";
}

TEST_F(TestLog, ErrorLevel)
{
    EXPECT_CALL(*m_logger, log(LOG_ERR, "Test"))
        .Times(1);
    Log::err << "Test";
}

TEST_F(TestLog, CriticalLevel)
{
    EXPECT_CALL(*m_logger, log(LOG_CRIT, "Test"))
        .Times(1);
    Log::critical << "Test";
}

}  // namespace dote
