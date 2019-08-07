#include "LogWriter.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"

#ifdef WIN32
#include <direct.h>
#include <io.h>                      //C (Windows)    access
#define R_OK 0
#else
#include <unistd.h>                  //C (Linux)      access
#endif

#if defined(WIN32)
    #include <WinSock2.h>
    #include <Windows.h>
static DWORD WINAPI thread_Func(LPVOID pM)
#else
    #include <sys/time.h>
    #include <stdio.h>
    #include <time.h>
    #include <stdlib.h>
    #include <unistd.h>
static void *thread_Func(void *pM)
#endif
{
    LogWriter *pointer = (LogWriter*)pM;
    pointer->run();

    return 0;
}

#define TMPBUFFERLEN (1024 * 1024 * 3)

LogWriter::LogWriter()
{
    mCondition = new Cond;

    mTmpBuffer = new char[TMPBUFFERLEN];

#if defined(WIN32)
     HANDLE handle = CreateThread(NULL, 0, thread_Func, this, 0, NULL);
#else
    pthread_t thread1;
    pthread_create(&thread1,NULL,thread_Func,this);
#endif
}

LogWriter::~LogWriter()
{
    if (mTmpBuffer != NULL)
    {
        delete mTmpBuffer;
        mTmpBuffer = NULL;
    }

    if (mCondition != NULL)
    {
        delete mCondition;
        mCondition = NULL;
    }
}

void LogWriter::addLogNode(const LogInfoNode &node)
{
    mCondition->Lock();
    mLogNodeList.push_back(node);
    mCondition->Signal();
    mCondition->Unlock();
}

void LogWriter::writeLog(int cameraId, const std::string &str)
{

    LogInfoNode node;
    node.cameraId = cameraId;
    node.mCreateTime =  getTimeStamp_MilliSecond();

#if defined(WIN32)
    SYSTEMTIME sys;
    GetLocalTime( &sys );

    memset(mTmpBuffer, 0x0, TMPBUFFERLEN);

    sprintf(mTmpBuffer,"[%d-%02d-%02d %02d:%02d:%02d.%03d] %s\n",
            sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond, sys.wMilliseconds, str.c_str());

    node.logStr = mTmpBuffer;

#else
    struct timeval    tv;
    struct timezone tz;

    struct tm         *p;

    gettimeofday(&tv, &tz);
    p = localtime(&tv.tv_sec);


    memset(mTmpBuffer, 0x0, TMPBUFFERLEN);

//    memset(node.time,0x0,32);
//    sprintf(node.time,"%d-%02d-%02d %02d:%02d:%02d.%03d",1900+p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec);

    sprintf(mTmpBuffer,"[%d-%02d-%02d %02d:%02d:%02d.%03d] %s\n",
            1900+p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec, str.c_str());

    node.logStr = mTmpBuffer;

#endif

    addLogNode(node);

//    if (cameraId == WRITE_LOG_ID_MAIN)
//    {
//        fprintf(stderr, "******:");
//    }

//    if (cameraId == WRITE_LOG_ID_MAIN)
    {

#if defined(WIN32)
        fprintf(stderr,"[%d-%02d-%02d %02d:%02d:%02d.%03d] %s\n",
                sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond, sys.wMilliseconds, str.c_str());

#else
        fprintf(stderr,"[%d-%02d-%02d %02d:%02d:%02d.%03d] %s",
                1900+p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec, str.c_str());
#endif
    }

}

void LogWriter::run()
{

    while(1)
    {
        mCondition->Lock();

        if (mLogNodeList.empty())
        {
            mCondition->Wait();
        }

        bool isNeedWriteToFile = false;
        if (mLogNodeList.size() >= 10)//日志文件超过10条 则写入文件
        {
            isNeedWriteToFile = true;
        }
        else
        {
            uint64_t startTime = mLogNodeList.front().mCreateTime;
            uint64_t currentTime = getTimeStamp_MilliSecond();

            if ((currentTime - startTime) > (10000)) //日志数据最迟10秒写入文件
            {
                isNeedWriteToFile = true;
            }
        }

        if(isNeedWriteToFile)
        {
            std::list<LogInfoNode> LogNodeList = mLogNodeList;
            mLogNodeList.clear();

            mCondition->Unlock();

            while(!LogNodeList.empty())
            {
                LogInfoNode node = LogNodeList.front();
                LogNodeList.pop_front();

#ifdef WIN32
                char logDirName[20] = {0};
                sprintf(logDirName,"log\\%d",node.cameraId);
                ///如果log目录不存在 则创建
                if (access(logDirName, R_OK)!=0)
                {
//                        _mkdir(logDirName);
                    char cmd[32] = {0};
                    sprintf(cmd,"mkdir %s",logDirName);
                    system(cmd);
                }
#else
                char logDirName[20] = {0};
                sprintf(logDirName,"log/%d",node.cameraId);
                ///如果log目录不存在 则创建
                if (access(logDirName,R_OK)!=0)
                {
                    char cmd[32] = {0};
                    sprintf(cmd,"mkdir %s -p",logDirName);
                    system(cmd);
                }
#endif

                ///一个文件最多5M 超过5M则创建下一个文件
                int index = 0;
                char fileName[36];
                while(1)
                {
                    memset(fileName,0x0,36);
                    sprintf(fileName,"log/%d/logfile_%d",node.cameraId,index++);
                    if (access(fileName, R_OK)==0)
                    {
                        FILE * fp = fopen(fileName, "r");
                        fseek(fp, 0L, SEEK_END);
                        int size = ftell(fp);
                        fclose(fp);
                        if (size < 5*1024*1024) //小于5M则可以写
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                FILE * fp = fopen(fileName, "at+");
                if (fp == NULL)
                {
                    fprintf(stderr,"写日志失败，请确保你有足够的权限写!\n");
                }
                else
                {
                    fwrite(node.logStr.c_str(),1,node.logStr.size(),fp);
                    fclose(fp);
                }
            }
        }
        else
        {
            mCondition->Unlock();
            mSleep(5000);
            continue;
        }
    }
}
