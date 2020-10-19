#ifndef WSLOG_H
#define WSLOG_H
#include <string.h>

#define LOG_ERROR		0
#define LOG_WARN	    1
#define LOG_INFO		    2
#define LOG_DEBUG		3



#define filename(x) strrchr(x,'/')?strrchr(x,'/')+1:x
 #define __CODE_LOCATION__ filename(__FILE__), __LINE__,__FUNCTION__

#define LOG_BUFF_SIZE 1024


class WSLog
{
public:
    static WSLog* Instance();
     void log(int level,const char* file, int line, const char* function,const char* message, ...);
protected:
    WSLog();
    virtual ~WSLog();
private:
    static WSLog* m_log;
};

#define WS_LINE WSLog::Instance()->log( LOG_DEBUG,__CODE_LOCATION__,"")
#define WS_BEGIN WSLog::Instance()->log( LOG_DEBUG,__CODE_LOCATION__,"begin")
#define WS_END WSLog::Instance()->log( LOG_DEBUG,__CODE_LOCATION__,"end")

#define WS_ERROR(msg_fmt,...) WSLog::Instance()->log( LOG_ERROR,__CODE_LOCATION__,msg_fmt,##__VA_ARGS__)
#define WS_INFO(msg_fmt,...) WSLog::Instance()->log( LOG_INFO,__CODE_LOCATION__,msg_fmt,##__VA_ARGS__)
#define WS_WARN(msg_fmt,...) WSLog::Instance()->log( LOG_WARN,__CODE_LOCATION__,msg_fmt,##__VA_ARGS__)
#define WS_DEBUG(msg_fmt,...) WSLog::Instance()->log( LOG_DEBUG,__CODE_LOCATION__,msg_fmt,##__VA_ARGS__)

#define WS_DEBUG_E(is_out,msg_fmt,...) \
do{\
   if(is_out == true)\
    WSLog::Instance()->log( LOG_DEBUG,__CODE_LOCATION__,msg_fmt,##__VA_ARGS__);\
}while(0)

#endif // WSLOG_H
