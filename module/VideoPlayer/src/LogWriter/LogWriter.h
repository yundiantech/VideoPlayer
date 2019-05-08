#ifndef LOGWRITER_H
#define LOGWRITER_H

#include <time.h>
#include <string.h>
#include <list>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string>

#include "Mutex/Cond.h"

#define LOGSTR_MAX_LENGTH 512

/**
 * @brief The LogWriter class
 * 写日志类 负责定时将日志信息写入文件  并管理日志文件
 */

class LogWriter
{
public:
    struct LogInfoNode
    {
        int cameraId;
        uint64_t mCreateTime; //创建的时间(用来判断过了多久)
        std::string logStr;

        LogInfoNode()
        {
            cameraId = 0;
    //        memset(time, 0x0, 32);
    //        memset(logStr, 0x0, LOGSTR_MAX_LENGTH);
        }

    };

    LogWriter();
    ~LogWriter();

    void writeLog(int cameraId, const std::string &str);

    void run();

private:
    char fileName[20];

    char *mTmpBuffer;

    void addLogNode(const LogInfoNode &node);

    std::list<LogInfoNode> mLogNodeList; //数据队列
    Cond *mCondition;

};

#endif // LOGWRITER_H
