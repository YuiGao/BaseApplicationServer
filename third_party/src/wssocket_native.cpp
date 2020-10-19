//
// Created by ty on 2020/8/30.
//

#include "wssocket_native.h"
#include <stdio.h>

#include "wslog.h"
#include <algorithm>
#include<unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>


namespace ws_error {
    const char* Err(int code)
    {
        switch (code) {
            case SOCKET_SUCCESS:
                return "成功";
            case SOCKET_DISCONNECT:
                return "未连接";
            case SOCKET_SEND_MESSAGE_LARGE:
                return "发送数据过大";
        }
        return "";
    }
}


WSSocketServer::WSSocketServer(int port,bool use_head,int recv_buff_len, int send_buff_len):
        m_port(port),
        m_use_head(use_head),
        m_recv_buff_len(recv_buff_len),
        m_send_buff_len(send_buff_len)
{
    m_recv_buffs = std::queue<char*>();
    /*
    for(int i=0;i<10;i++)
    {
        char* it  = new char[m_recv_buff_len];
        m_recv_buffs.push(it);
    }
     */
    WS_DEBUG("recv buffs size:%d",m_recv_buffs.size());
    pthread_mutexattr_t attr;
    pthread_mutex_init(&m_send_buff_lock,&attr);
    pthread_mutex_init(&m_recv_buff_lock,&attr);
    pthread_mutex_init(&m_apr_send_lock,&attr);
    m_base_ev = event_base_new();

    struct sockaddr_in t_sockaddr;
    memset(&t_sockaddr, 0, sizeof(t_sockaddr));
    t_sockaddr.sin_family = AF_INET;
    t_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    t_sockaddr.sin_port = htons(port);

    m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listen_fd < 0) {
        WS_ERROR(" create socket error %s errno: %d",strerror(errno), errno);
    }

    int ret = bind(m_listen_fd,(struct sockaddr *) &t_sockaddr,sizeof(t_sockaddr));
    if (ret < 0) {
        WS_ERROR("bind socket error %s errno: %d", strerror(errno), errno);
    }
    int one = 1;
    setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
}

WSSocketServer::~WSSocketServer()
{
    if(m_listen_fd > 0 ) close(m_listen_fd);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    event_base_loopexit(m_base_ev,&tv);

    while(!m_send_buffs.empty())
    {
        char* it =m_send_buffs.front();
        delete[] it;
        m_send_buffs.pop();
    }
    while(!m_recv_buffs.empty())
    {
        char* it =m_recv_buffs.front();
        delete[] it;
        m_recv_buffs.pop();
    }
    pthread_mutex_destroy(&m_send_buff_lock);
    pthread_mutex_destroy(&m_recv_buff_lock);
    pthread_mutex_destroy(&m_apr_send_lock);
}

int WSSocketServer::Listen(int max_connect)
{
    int one = 1;
    setsockopt(m_listen_fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
    m_listen_event = event_new(m_base_ev, m_listen_fd, EV_TIMEOUT|EV_READ|EV_CLOSED|EV_PERSIST, OnActive, this);

    st_FD_SOCKET fd_socket;
    fd_socket.fd = m_listen_fd;
    fd_socket.event = m_listen_event;
    m_sockets_map.insert(std::make_pair(m_listen_fd,fd_socket));

    int ret = listen(m_listen_fd, max_connect);
    if (ret < 0) {
        WS_ERROR("listen error %s errno: %d\n", strerror(errno), errno);
    }

    WS_INFO("tcp listen %d success",m_port);
    event_add(m_listen_event,NULL);

    int flags = fcntl(m_listen_fd, F_GETFL, 0);                       //获取文件的flags值。
    fcntl(m_listen_fd, F_SETFL, flags | O_NONBLOCK);   //设置成非阻塞模式；

    m_work_thread = new MyThread(WSSocketServer::WorkDispatch,this);

    return 0;

}
static const char *
inet_ntop4_1(const unsigned char *src, char *dst, int size)
{
    const int MIN_SIZE = 16; /* space for 255.255.255.255\0 */
    int n = 0;
    char *next = dst;

    if (size < MIN_SIZE) {
        errno = ENOSPC;
        return NULL;
    }
    do {
        unsigned char u = *src++;
        if (u > 99) {
            *next++ = '0' + u/100;
            u %= 100;
            *next++ = '0' + u/10;
            u %= 10;
        }
        else if (u > 9) {
            *next++ = '0' + u/10;
            u %= 10;
        }
        *next++ = '0' + u;
        *next++ = '.';
        n++;
    } while (n < 4);
    *--next = 0;
    return dst;
}

void WSSocketServer::Accept()
{
    st_FD_SOCKET fd_socket;
    socklen_t socket_len = sizeof(struct sockaddr);
    int conn_fd = accept(m_listen_fd, &fd_socket.addr, &socket_len);
    if(conn_fd < 0) {
        WS_ERROR("socket accept error,%s errno :%d",strerror(errno), errno);
        return;
    }
    char buf[100] = {0};
    int buflen =100;
    inet_ntop4_1((const unsigned char*)(fd_socket.addr.sa_data), buf, buflen);
    WS_INFO("客户端[%s]连接",buf);

    int flags = fcntl(m_listen_fd, F_GETFL, 0);                       //获取文件的flags值。
    fcntl(m_listen_fd, F_SETFL, flags | O_NONBLOCK);   //设置成非阻塞模式；
    fd_socket.fd = conn_fd;
    fd_socket.event = event_new(m_base_ev, conn_fd, EV_TIMEOUT|EV_READ|EV_CLOSED|EV_PERSIST, OnActive, this);
    event_add(fd_socket.event,NULL);
    m_sockets_map.insert(std::make_pair( conn_fd,fd_socket));
}

int WSSocketServer::SendMsg(int fd,int type,char* message,int len)
{
    int head_len=12;
    auto fd_socket = m_sockets_map.find(fd);
    if(fd_socket == m_sockets_map.end()) return SOCKET_DISCONNECT;
    if(len > m_send_buff_len - head_len) return SOCKET_SEND_MESSAGE_LARGE;

    char* send_buff = GetSendBuff();
    memset(send_buff,0,m_send_buff_len);

    if(m_use_head == true)
    {
        unsigned int* a = (unsigned int *)send_buff;
        a[0] = 0x55aa55aa;
        a[1] = len;
        a[2] = type;
        memcpy(send_buff+head_len,message,len);
        len += head_len;
    } else{
        memcpy(send_buff,message,len);
    }

    int send_len = send(fd,send_buff,len,0);
    FreeSendBuff(send_buff);
    if(send_len != len)
    {
        WS_ERROR("apr_socket_send fail!,%s errno :%d",strerror(errno), errno);
        return -1;
    }
    return len;
}

void WSSocketServer::Recv(st_FD_SOCKET fd_socket)
{

    size_t recvLen = m_recv_buff_len;
    char* recv_buff = GetRecvBuff();
    if(recv_buff == NULL)
    {
        WS_ERROR("recv buff is null");
    }
    memset(recv_buff,0,m_recv_buff_len);
    ssize_t recv_len = recv(fd_socket.fd,(void*)recv_buff,recvLen,0);
    if(recv_len < 0)
    {
        WS_ERROR("recv %d error, %s errno :%d",strerror(errno), errno);
        FreeRecvBuff(recv_buff);
        return;
    }

    if(m_use_head == true)
    {
        int* agreement =(int*)recv_buff;
        if(*agreement != 0x55aa55aa)
        {
            WS_WARN("recv unknown message");
            FreeRecvBuff(recv_buff);
            return;
        }
        int* type  = (int*)&recv_buff[4];
        int* len = (int*)&recv_buff[8];

        if(*type == 16 && *len == 1) //心跳包
        {
            FreeRecvBuff(recv_buff);
            return;
        }
        if(m_callback)
            m_callback(fd_socket.fd,*type,&recv_buff[12],*len,m_callback_arg);
    } else
    {
        if(m_callback)
            m_callback(fd_socket.fd,0,recv_buff,recvLen,m_callback_arg);
    }
    FreeRecvBuff(recv_buff);
}


void WSSocketServer::OnActive(evutil_socket_t fd, short event, void *arg)
{

    WSSocketServer* pThis = (WSSocketServer*)arg;
    auto fd_socket = pThis->m_sockets_map.find(fd);
    if(fd_socket == pThis->m_sockets_map.end())
    {
        WS_DEBUG("unknown fd:%d",fd);
        return;
    }
    if(event&EV_TIMEOUT)//超时
    {
        WS_WARN("timeout");
        return;
    };
    if(event&EV_CLOSED)//断开连接
    {
        char buf[100] = {0};
        int buflen =100;
        inet_ntop4_1((const unsigned char*)(fd_socket->second.addr.sa_data), buf, buflen);
        WS_WARN("客户端[%s]断开连接",buf);
        if(pThis->m_close_callback != NULL)
        {
            pThis->m_close_callback(fd_socket->second.fd,pThis->m_close_callback_arg);
        }
        event_del(fd_socket->second.event);
        event_free(fd_socket->second.event);
        close(fd_socket->second.fd);
        pThis->m_sockets_map.erase(fd_socket);
        return;
    }
    if(fd_socket->second.fd == pThis->m_listen_fd) //新连接
    {
        pThis->Accept();
    }else   //客户端上传数据
    {
        pThis->Recv(fd_socket->second);
    }

}


void WSSocketServer::WorkDispatch(void* arg)
{

    WSSocketServer* pThis = (WSSocketServer*)arg;
    event_base_dispatch(pThis->m_base_ev);
}


char* WSSocketServer::GetSendBuff()
{

    pthread_mutex_lock(&m_send_buff_lock);
    char* it = NULL;
    WS_DEBUG("send buff size:%d",m_recv_buffs.size());
    if(m_send_buffs.empty())
    {
        it = new char[m_send_buff_len];
    }else
    {
        it = m_send_buffs.front();
        m_send_buffs.pop();
    }

    pthread_mutex_unlock(&m_send_buff_lock);
    return it;
}
void WSSocketServer::FreeSendBuff(char* buff)
{
    pthread_mutex_lock(&m_send_buff_lock);
    m_send_buffs.push(buff);
    pthread_mutex_unlock(&m_send_buff_lock);
}

char* WSSocketServer::GetRecvBuff()
{
    char* it = NULL;
    pthread_mutex_lock(&m_recv_buff_lock);

    WS_DEBUG("recv buff size:%d",m_recv_buffs.size());
    if(m_recv_buffs.empty())
    {
        it = new char[m_recv_buff_len];
    }else
    {
        it = m_recv_buffs.front();
        m_recv_buffs.pop();
    }
    pthread_mutex_unlock(&m_recv_buff_lock);
    return it;
}
void WSSocketServer::FreeRecvBuff(char* buff)
{
    pthread_mutex_lock(&m_recv_buff_lock);
    WS_DEBUG(" free recv buff size:%d",m_recv_buffs.size());
    m_recv_buffs.push(buff);
    WS_DEBUG(" free recv buff size:%d",m_recv_buffs.size());
    pthread_mutex_unlock(&m_recv_buff_lock);
}





/*----------------------------------------------------------------------------*/

WSSocketClient::WSSocketClient(char* server_ip,int port,bool use_head,int buf_len)
        :m_server_ip(server_ip),
         m_server_port(port),
         m_use_head(use_head),
         m_buf_len(buf_len)

{

    m_send_buff = new char[m_buf_len];
    memset(m_send_buff,0,m_buf_len);

    m_recv_buff = new char[m_buf_len];
    memset(m_recv_buff,0,m_buf_len);


    m_head_len = sizeof(unsigned int)*3;
    m_connect.store(false);

    m_base_ev = event_base_new();

    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd < 0) {
        WS_ERROR(" create socket error %s errno: %d",strerror(errno), errno);
    }


}

WSSocketClient::~WSSocketClient()
{
    if(m_fd > 0) close(m_fd);
    delete[] m_send_buff;

}
int WSSocketClient::Connect()
{

    struct sockaddr_in t_sockaddr;
    memset(&t_sockaddr, 0, sizeof(struct sockaddr_in));
    t_sockaddr.sin_family = AF_INET;
    t_sockaddr.sin_port = htons(m_server_port);
    inet_pton(AF_INET, m_server_ip.c_str(), &t_sockaddr.sin_addr);


    int flags = fcntl(m_fd, F_GETFL, 0);                       //获取文件的flags值。
    fcntl(m_fd, F_SETFL, flags | O_NONBLOCK);   //设置成非阻塞模式；




    m_ev = event_new(m_base_ev, m_fd, EV_TIMEOUT|EV_READ|EV_CLOSED|EV_PERSIST, RecvMsg, this);

    if((connect(m_fd, (struct sockaddr*)&t_sockaddr, sizeof(struct sockaddr))) < 0 ) {
        WS_ERROR("connect [%s:%d ] error %s errno: %d", m_server_ip.c_str(),m_server_port,strerror(errno), errno);
        return 0;
    }

    int reuse = 1;
    setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    WS_INFO("connnect server:%s:%d success",m_server_ip.c_str(),m_server_port);
    event_add(m_ev, NULL);
    m_connect.store(true);
    return 0;
}
int WSSocketClient::SendMsg(int type,char* message,int len)
{
    if(len > m_buf_len - m_head_len) return SOCKET_SEND_MESSAGE_LARGE;
    if(m_connect.load() == false) return SOCKET_DISCONNECT;
    memset(m_send_buff,0,m_buf_len);
    unsigned int* a = (unsigned int *)m_send_buff;
    a[0] = 0x55aa55aa;
    a[1] = len;
    a[2] = type;
    memcpy(m_send_buff+m_head_len,message,len);
    len += m_head_len;

    int send_len = send(m_fd,m_send_buff,len,0);
    if(send_len != len)
    {
        WS_ERROR("send fail!,%s errno :%d",strerror(errno), errno);
        return -1;
    }
    return len;
}

void* WSSocketClient::ReconnectThread(void* arg)
{
    WSSocketClient* pThis = (WSSocketClient*)arg;
    while (true)
    {
        if(pThis->m_connect.load() == false)
        {
            pThis->Connect();
        }
        usleep(ONE_SECOND_TIME_MICRO);
    }
}

void WSSocketClient::RecvMsg(evutil_socket_t fd, short event, void *arg)
{

    WSSocketClient* pThis = (WSSocketClient*)arg;
    if(event&EV_TIMEOUT)//超时
    {
        WS_WARN("timeout");
        return;
    };
    if(event&EV_CLOSED)//断开连接
    {
        WS_WARN("服务端断开连接");
        event_del(pThis->m_ev);
        event_free(pThis->m_ev);
        pThis->m_connect.store(false);
        return;
    }
    if(fd != pThis->m_fd)
    {
        WS_WARN("unknown socket:%d",fd);
        return;
    }


    int recvLen = pThis->m_buf_len;
    memset(pThis->m_recv_buff,0,pThis->m_buf_len);
    ssize_t recv_len = recv((int)fd,pThis->m_recv_buff,recvLen,0);
    if(recv_len < 0)
    {
        WS_ERROR("recv %d error, %s errno :%d",strerror(errno), errno);
        return;
    }
    int* agreement =(int*)pThis->m_recv_buff;
    if(*agreement != 0x55aa55aa)
    {
        WS_WARN("recv unknown message");
        return;
    }
    int* type  = (int*)&pThis->m_recv_buff[4];
    int* len = (int*)&pThis->m_recv_buff[8];

    if(*type == 16 && *len == 1) //心跳包
    {
        return;
    }
    if(pThis->m_callback)
        pThis->m_callback(*type,&pThis->m_recv_buff[12],*len,pThis->m_callback_arg);
    // WS_INFO("recv:type:%d,len:%d,%s",*type,*len,recvBuf +pThis->m_head_len);
}
void* WSSocketClient::Work(void* arg)
{

    WSSocketClient* pThis = (WSSocketClient*)arg;
    while (true) {
        while (pThis->m_connect.load() == false) {
            usleep(ONE_SECOND_TIME_MICRO);
        }

        event_base_dispatch(pThis->m_base_ev);

        //   WS_DEBUG("%d",__LINE__);
    }
}