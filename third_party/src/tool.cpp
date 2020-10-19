//
// Created by ty on 2020/8/23.
//

#include "tool.h"
#include <string>
#include <time.h>
#include  <sys/time.h>
void GetNowTime(char* buff, int len)
{
    timeval tv;
    gettimeofday(&tv,0);
    tm *pTime = localtime(&tv.tv_sec);
    snprintf(buff, len, "%04d%02d%02d%02d%02d%02d%03d", pTime->tm_year+1900, \
            pTime->tm_mon+1, pTime->tm_mday, pTime->tm_hour, pTime->tm_min, pTime->tm_sec, \
            (int)tv.tv_usec/1000);
}