#include "wslog.h"

#include "log4cpp/Category.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/DailyRollingFileAppender.hh"
#include "log4cpp/PatternLayout.hh"
#include "log4cpp/Priority.hh"
#include "log4cpp/LoggingEvent.hh"
#include <ctime>
#include <iostream>
#ifdef USE_APR
#include "apr-1/apr_file_io.h"
#include "apr-1/apr_thread_mutex.h"
#else
#include <pthread.h>
#endif
#include <unistd.h>

class myLayout:  public log4cpp::Layout{
public:
    myLayout(){
    }
    ~myLayout(){
    }
    std::string format(const log4cpp::LoggingEvent& event)
    {
        const std::string& priorityName = log4cpp::Priority::getPriorityName(event.priority);
        char formatted[100] = {0};
        std::time_t t = event.timeStamp.getSeconds();
        struct std::tm* currentTime = localtime(&t);
        std::strftime(formatted, sizeof(formatted), "%m-%d %H:%M:%S", currentTime);
        std::ostringstream message;
        message << formatted << " " << priorityName << " " << event.file_function_info << ":"
                << event.message << std::endl;
        return message.str();
    }
};

WSLog* WSLog::m_log = nullptr;

WSLog* WSLog::Instance()
{

    if(WSLog::m_log == nullptr)
    {

        pthread_mutexattr_t attr;
        pthread_mutex_t mutex;
        pthread_mutex_init(&mutex,&attr);
        pthread_mutex_lock(&mutex);

        if(WSLog::m_log  == nullptr)
        {
            WSLog::m_log  = new WSLog();
            pid_t pid = getpid();
            WSLog::m_log->log(LOG_DEBUG,"",1,"","###########################new log start:%d#########################",pid);
        }
        pthread_mutex_unlock(&mutex);
    }
    return WSLog::m_log ;
}

WSLog::WSLog()
{
    mkdir("logs", S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH);

    log4cpp::DailyRollingFileAppender* drfAppender = new log4cpp::DailyRollingFileAppender("log","logs/log.log",30);
    drfAppender->setLayout(new myLayout());
    log4cpp::OstreamAppender*osAppender = new log4cpp::OstreamAppender("log",&std::cout);
    osAppender->setLayout(new myLayout());
    log4cpp::Category& root =log4cpp::Category::getRoot();
    root.addAppender(osAppender);
    root.addAppender(drfAppender);
    root.setPriority(log4cpp::Priority::DEBUG);

}


void WSLog::log(int level,const char* file, int line, const char* function, const char* message, ...)
{

    log4cpp::Category& root =log4cpp::Category::getRoot();
    char buff[LOG_BUFF_SIZE] = {0};
    char info[256] = {0};
    sprintf(info,"%s:%s[%d]",file,function,line);
    va_list va;
    va_start(va,message);
    vsprintf(buff,message,va);
    va_end(va);
    switch(level)
    {
        case LOG_ERROR:
    {
        root.error_1(info,buff);
    }
        break;

        case LOG_WARN:
    {
        root.warn_1(info,buff);
    }

        break;
        case LOG_INFO:
    {
        root.info_1(info,buff);
    }
        break;
        case  LOG_DEBUG:
    {
        root.debug_1(info,buff);
    }
        break;
    }
}
WSLog::~WSLog()
{
    log4cpp::Category& root =log4cpp::Category::getRoot();
    root.shutdown();
    //dtor
}
