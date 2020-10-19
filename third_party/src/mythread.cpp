#include "mythread.h"
#include "wslog.h"
#include <unistd.h>
#include <signal.h>
#ifdef USE_APR_THREAD
#include "apr_time.h"
#endif
MyThread::MyThread(callFunction call,void* arg,int sleepTimeMicro,int Timeout)
    :m_fun(call),
    m_arg(arg),
    m_sleepTime(sleepTimeMicro),
    m_timeout(Timeout)
{
    m_bThreadExit.store(false);
    m_bThreadExitState.store(false);
#ifdef USE_APR_THREAD
    apr_initialize();
    apr_pool_create(&local_pool,NULL);
    apr_threadattr_create(&attr,local_pool);
    if(APR_SUCCESS != apr_thread_create(&work_thread,attr,MyThread::WorkThread,this,local_pool))
    {
        WS_ERROR("create thread failed");
    }
#else
    int ret=pthread_create(&m_thread_id,NULL,MyThread::WorkThread,this);
    if(ret!=0){
        WS_ERROR ("Create pthread error!");
    }

#endif
}

#if 0
MyThread::MyThread(FunCall call,int sleepTimeMicro,int Timeout)
    : m_fun(NULL),
      m_arg(NULL),
      m_call(call),
    m_sleepTime(sleepTimeMicro),
    m_timeout(Timeout)
{

    m_bThreadExit.store(false);
    m_bThreadExitState.store(false);
#ifdef USE_APR_THREAD
    apr_initialize();
    apr_pool_create(&local_pool,NULL);
    apr_threadattr_create(&attr,local_pool);
    if(APR_SUCCESS != apr_thread_create(&work_thread,attr,MyThread::WorkThread,this,local_pool))
    {
        WS_ERROR("create thread failed");
    }
#else
    int ret=pthread_create(&m_thread_id,NULL,MyThread::WorkThread,this);
    if(ret!=0){
        WS_ERROR ("Create pthread error!");
    }
#endif

}
#endif
MyThread::~MyThread(void)
{
    m_bThreadExit.store(true);
    int loop_time = 10;
    while (loop_time-- > 0) {
        if(m_bThreadExitState.load() == false)
        {
#ifdef USE_APR_THREAD
            apr_sleep(100*ONE_MILLI_SECOND_TIME);
#else
            usleep(100*ONE_MILLI_SECOND_TIME);
#endif
            continue;
        }
    }
#ifdef USE_APR_THREAD
    apr_thread_exit(work_thread,0);
    apr_pool_destroy(local_pool);
    apr_terminate();
#else
    pthread_kill(m_thread_id,0);
#endif
}
#ifdef USE_APR_THREAD
void*  MyThread::WorkThread(apr_thread_t* /*thread*/, void* arg )
#else
void*  MyThread::WorkThread(void* arg )
#endif
{
    MyThread* pThis = (MyThread*)arg;
    while (!pThis->m_bThreadExit.load())
    {
        if(pThis->m_fun != NULL)
        {
            pThis->m_fun(pThis->m_arg);
        }
        if(pThis->m_call.operator bool() == true)
        {
            pThis->m_call(NULL);
        }
#ifdef USE_APR_THREAD
        apr_sleep(pThis->m_sleepTime);
#else
        usleep(pThis->m_sleepTime);
#endif
    }
    pThis->m_bThreadExitState.store(true);
    return NULL;
}
