//
// Created by ty on 2020/8/30.
//

#ifndef COMPARESERVER_WSSOCKET_NATIVE_H
#define COMPARESERVER_WSSOCKET_NATIVE_H
#include <atomic>
#include <string>
#include <map>
#include <queue>

#include "event2/event.h"
#include "mythread.h"
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
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
    struct sockaddr addr;
    struct event * event;
}st_FD_SOCKET;

class WSSocketServer
{
public:
    WSSocketServer(int port,bool use_head = true,int recv_buff_len = 1024*1024,int send_buff_len = 1024*1024);
    ~WSSocketServer();
    int Listen(int max_connect);
    int SendMsg(int fd,int type,char* message,int len);

    int  SetServerRecvCallBack(ServerRecvCallback call, void* arg){m_callback = call;m_callback_arg=arg; return 0;}
    int  SetServerCloseCallBack(ServerCloseCallback call, void* arg){m_close_callback = call;m_close_callback_arg=arg; return 0;}

    //   void SetFunRecv(FunRecv f){m_fun_recv = f;}
    //  void SetFunClose(FunClose f) {m_fun_close = f;}

protected:
    void Accept();
    void Recv(st_FD_SOCKET fd);
    static void OnActive(evutil_socket_t fd, short event, void *arg);
    static void WorkDispatch(void* arg);
    char* GetSendBuff();
    void FreeSendBuff(char* buff);

    char* GetRecvBuff();
    void FreeRecvBuff(char* buff);



private:
    int m_port;

    MyThread* m_work_thread = NULL;

    int m_listen_fd;

    struct event_base *m_base_ev = NULL;

    struct event * m_listen_event;

    std::map<evutil_socket_t ,st_FD_SOCKET> m_sockets_map;

    ServerRecvCallback m_callback = NULL;
    void* m_callback_arg;
    ServerCloseCallback m_close_callback = NULL;
    void* m_close_callback_arg;

    FunRecv m_fun_recv;
    FunClose  m_fun_close;

    int m_send_buff_len = 1024*1024;
    int m_recv_buff_len = 1024*1024;

    pthread_mutex_t  m_send_buff_lock;
    std::queue<char*> m_send_buffs;

    pthread_mutex_t  m_recv_buff_lock;
    std::queue<char*> m_recv_buffs;
    bool m_use_head;
    pthread_mutex_t  m_apr_send_lock;
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
    static void* Work( void* arg);
    static void* ReconnectThread(void* arg);
    static void RecvMsg(evutil_socket_t fd, short event, void *arg);
private:

    std::string m_server_ip;
    int m_server_port;

    int m_fd = 0;

    struct event_base *m_base_ev = NULL;
    struct event *m_ev = NULL;
    std::atomic<bool> m_connect;
    char* m_send_buff = NULL;
    char* m_recv_buff = NULL;
    int m_buf_len = 1024*1024;
    int m_head_len;
    RecvCallback m_callback;
    void* m_callback_arg;

    bool m_use_head;

};


#endif //COMPARESERVER_WSSOCKET_NATIVE_H
