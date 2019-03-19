#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <stdio.h>
#include <string>

#include "LogWriter/LogWriter.h"

#if defined(WIN32)

#define PRINT_INT64_FORMAT "%I64d"

#else
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

void Sleep(long mSeconds);

#define PRINT_INT64_FORMAT "%lld"

#endif

#include <QDebug>
#define OUTPUT qDebug

typedef unsigned char uchar;

class AppConfig
{
public:
    AppConfig();

    static int VERSION;
    static char VERSION_NAME[32];

    static LogWriter* gLogWriter;

    static void mkdir(char *dirName); //创建文件夹
    static void mkpath(char *path);   //创建多级文件夹

    static void removeDir(char *dirName);
    static void removeFile(const char *filePath);

    static void copyFile(const char *srcFile, const char *destFile);

    static void replaceChar(char *string, char oldChar, char newChar); //字符串替换字符
    static std::string removeFirstAndLastSpace(std::string &s); //移除开始和结束的空格

    static std::string getIpFromRtspUrl(std::string rtspUrl); //从rtsp地址中获取ip地址

    static void mSleep(int mSecond);

    static int64_t getTimeStamp_MilliSecond(); //获取时间戳（毫秒）

};

#endif // APPCONFIG_H
