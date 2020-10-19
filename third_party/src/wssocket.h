#ifndef WSSOCKETCLIENT_H
#define WSSOCKETCLIENT_H
#include <atomic>
#include <string>
#include <map>
#include <queue>
#include "apr-1/apr_thread_proc.h"
#include "apr-1/apr_network_io.h"
#include "apr-1/apr_thread_mutex.h"
#include "event2/event.h"
#include "mythread.h"
#include <functional>

#define SOCKET_SUCCESS 0
#define SOCKET_ERROR_BASE -1000
#define SOCKET_DISCONNECT (SOCKET_ERROR_BASE - 1)
#define SOCKET_SEND_MESSAGE_LARGE (SOCKET_ERROR_BASE - 2)


namespace ws_error {
const char* Err(int code);
}

/*
 * 2种方式回调
 */
typedef void (*ServerRecvCallback)(int fd,int type, char*buff, int len, void* param);
typedef void (*ServerCloseCallback)(int fd,void* param);

typedef std::function<void(int fd, int errorcode)> FunClose;
typedef std::function<void(int fd,int type, char*buff, int len)> FunRecv;

typedef struct fd_apr_event{
    evutil_socket_t fd;
    apr_socket_t* socket;
    struct event * event;
}st_FD_APR_SOCKET;

class WSSocketServer
{
public:
    WSSocketServer(int port,bool use_head = true,int recv_buff_len = 1024*1024,int send_buff_len = 1024*1024);
    ~WSSocketServer();
    int Listen(int max_connect);
    int SendMsg(int fd,int type,char* message,int len);

    int  SetServerRecvCallBack(ServerRecvCallback call, void* arg){
        m_callback = call;
        m_callback_arg=arg;
        printf("set call back \n");
        return 0;
    }
    int  SetServerCloseCallBack(ServerCloseCallback call, void* arg){m_close_callback = call;m_close_callback_arg=arg; return 0;}

 //   void SetFunRecv(FunRecv f){m_fun_recv = f;}
  //  void SetFunClose(FunClose f) {m_fun_close = f;}

protected:
    void Accept();
    void Recv(st_FD_APR_SOCKET fd);
    static void OnActive(evutil_socket_t fd, short event, void *arg);
    static void* WorkDispatch(apr_thread_t* thread, void* arg);

    char* GetSendBuff();
    void FreeSendBuff(char* buff);

    char* GetRecvBuff();
    void FreeRecvBuff(char* buff);


    void ConstructTickData();
private:
    int m_port;
    apr_pool_t *local_pool = NULL;
    apr_thread_t *dispatch_thread =NULL;


    apr_sockaddr_t *sa = NULL;
    apr_socket_t *listen_socket = NULL;
    struct event_base *m_base_ev = NULL;

    struct event * m_listen_event;

    std::map<evutil_socket_t ,st_FD_APR_SOCKET> m_sockets_map;

    ServerRecvCallback m_callback = NULL;
    void* m_callback_arg;
    ServerCloseCallback m_close_callback = NULL;
    void* m_close_callback_arg;

    FunRecv m_fun_recv;
    FunClose  m_fun_close;

    int m_send_buff_len = 1024*1024;
    int m_recv_buff_len = 1024*1024;

    apr_thread_mutex_t* m_send_buff_lock;
    std::queue<char*> m_send_buffs;

    apr_thread_mutex_t* m_recv_buff_lock;
    std::queue<char*> m_recv_buffs;
    bool m_use_head;
    apr_thread_mutex_t* m_apr_send_lock;
    char  cTickData[28];
};


typedef void (*RecvCallback)(int type, char*buff, int len, void* para);

class WSSocketClient
{
public:
    WSSocketClient(char* server_ip,int port,bool use_head = true,int buf_len = 1024*1024);
    ~WSSocketClient();
    int Connect();
    int  SetRecvCallBack(RecvCallback call, void* arg){m_callback = call;m_callback_arg=arg; return 0;}
    bool ConnectState(){return m_connect.load();}
    int SendMsg(int type,char* message,int len);
protected:
    static void* Work(apr_thread_t* thread, void* arg);
    static void* ReconnectThread(apr_thread_t* thread, void* arg);
    static void RecvMsg(evutil_socket_t fd, short event, void *arg);
private:

    std::string m_server_ip;
    int m_server_port;

    apr_pool_t *local_pool = NULL;
    apr_sockaddr_t *sa = NULL;
    apr_socket_t *new_socket = NULL;

    apr_thread_t *work_thread =NULL;
    apr_thread_t *reconnect_thread =NULL;
    struct event_base *m_base_ev = NULL;
    struct event *m_ev = NULL;
    std::atomic<bool> m_connect;
    char* m_send_buff = NULL;
    char* m_recv_buff = NULL;
    int m_buf_len = 1024*1024;
    int m_head_len;
    RecvCallback m_callback = NULL;
    void* m_callback_arg;

    bool m_use_head;

};

#endif // WSSOCKETCLIENT_H
