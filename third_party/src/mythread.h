#ifndef MYTHREAD_H
#define MYTHREAD_H
//#define USE_APR_THREAD

#ifdef USE_APR_THREAD
#include "apr_thread_proc.h"
#include "apr_network_io.h"
#include "apr_thread_mutex.h"
#else
#include <pthread.h>
#endif
#include <atomic>
#include <functional>

#define WORK_TIMEOUT 5*60*1000
#define ONE_SECOND_TIME_MICRO 1000000
#define ONE_MILLI_SECOND_TIME 1000

#define UNUSED(x) (void)(x)


typedef void (*callFunction)(void* arg);   //传入静态函数
typedef std::function<int(void* arg)> FunCall;   //传入成员函数
class MyThread
{
public:

    MyThread(callFunction call,void* arg,int sleepTimeMicro = 0,int Timeout = WORK_TIMEOUT);
  //  MyThread(FunCall call,int sleepTimeMicro = 0,int Timeout = WORK_TIMEOUT);
    ~MyThread(void);
#ifdef USE_APR_THREAD
    static void*  WorkThread(apr_thread_t* /*thread*/, void* arg );
#else
    static void*  WorkThread(void* arg );
#endif
private:
    std::atomic_bool m_bThreadExit;
    std::atomic_bool m_bThreadExitState;
    callFunction m_fun;
    void* m_arg;

    FunCall m_call;


    int m_sleepTime;
    int m_timeout;

#ifdef USE_APR_THREAD
    apr_pool_t *local_pool = NULL;
    apr_thread_t *work_thread =NULL;
    apr_threadattr_t *attr =NULL;
#else
    pthread_t m_thread_id;

#endif


};

#endif // MYTHREAD_H
