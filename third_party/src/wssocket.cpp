#include "apr-1/apr_arch_networkio.h"
#include "wssocket.h"

#include <stdio.h>

#include "wslog.h"
#include <algorithm>



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


    apr_initialize();
    apr_status_t ret = APR_SUCCESS;
    apr_pool_create(&local_pool,NULL);

    apr_thread_mutex_create(&m_send_buff_lock,0,local_pool);
    apr_thread_mutex_create(&m_recv_buff_lock,0,local_pool);
    apr_thread_mutex_create(&m_apr_send_lock,0,local_pool);

    ConstructTickData();
    m_base_ev = event_base_new();

    ret = apr_sockaddr_info_get(&sa, NULL, APR_INET, m_port, 0, local_pool);
    if (APR_SUCCESS != ret)
    {
        WS_ERROR("apr_sockaddr_info_get fail!,ret = %d",ret);
        return ;
    }
    ret = apr_socket_create(&listen_socket, sa->family, SOCK_STREAM, APR_PROTO_TCP, local_pool);
    if (APR_SUCCESS != ret)
    {
        WS_ERROR("reate socket fail!,ret =  %d",ret);
        return ;
    }
    apr_socket_opt_set(listen_socket, APR_SO_REUSEADDR, 1);
    ret = apr_socket_bind(listen_socket, sa);

}

WSSocketServer::~WSSocketServer()
{


    if(listen_socket != NULL) apr_socket_close(listen_socket);
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    event_base_loopexit(m_base_ev,&tv);
    while(m_send_buffs.empty())
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

    apr_thread_mutex_destroy(m_send_buff_lock);
    apr_thread_mutex_destroy(m_recv_buff_lock);
    apr_thread_mutex_destroy(m_apr_send_lock);

    apr_thread_exit(dispatch_thread,0);
    apr_pool_destroy(local_pool);
    apr_terminate();
}

int WSSocketServer::Listen(int max_connect)
{

    apr_threadattr_t *attr =NULL;


    apr_socket_opt_set(listen_socket, APR_SO_KEEPALIVE, 1);
    m_listen_event = event_new(m_base_ev, listen_socket->socketdes, EV_TIMEOUT|EV_READ|EV_CLOSED|EV_PERSIST, OnActive, this);

    st_FD_APR_SOCKET fd_socket;
    fd_socket.fd = listen_socket->socketdes;
    fd_socket.socket = listen_socket;
    fd_socket.event = m_listen_event;
    m_sockets_map.insert(std::make_pair(listen_socket->socketdes,fd_socket));

    apr_status_t ret = apr_socket_listen(listen_socket,max_connect);
    if(ret != APR_SUCCESS)
    {
        WS_ERROR("listen failed:%d",m_port);
        return -1;
    }
    WS_INFO("tcp listen %d success",m_port);
    event_add(m_listen_event,NULL);

    apr_socket_opt_set(listen_socket, APR_SO_NONBLOCK, 1);

    apr_threadattr_create(&attr,local_pool);
    if(APR_SUCCESS != apr_thread_create(&dispatch_thread,attr,WSSocketServer::WorkDispatch,this,local_pool))
    {
        WS_ERROR("create thread failed");
        return -1;
    }
    return 0;

}

void WSSocketServer::ConstructTickData()
{
    //构建心跳包数据
    unsigned int a[3];
    a[0] = 0x55aa55aa;
    a[1] = 16;
    a[2] = 1;
    unsigned long buf[4] = {0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa};
    for (int i = 0; i < 12; ++i)
    {
        cTickData[i] = ((char*)a)[i];
    }
    char *pdata = (char*)buf;
    for (int i = 0; i < 16; ++i)
    {
        cTickData[12+i] = pdata[i];
    }
}
static const char *
inet_ntop4_1(const unsigned char *src, char *dst, apr_size_t size)
{
    const apr_size_t MIN_SIZE = 16; /* space for 255.255.255.255\0 */
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
    st_FD_APR_SOCKET fd_socket;

    apr_status_t ret =  apr_socket_accept(&fd_socket.socket,listen_socket,local_pool);
    if(ret != APR_SUCCESS)
    {
        WS_ERROR("socket accept error");
        return;
    }

    char buf[100] = {0};
    int buflen =100;
    inet_ntop4_1((const unsigned char*)(fd_socket.socket->remote_addr->ipaddr_ptr), buf, buflen);
    WS_INFO("客户端[%s]连接",buf);



    apr_socket_opt_set(listen_socket, APR_SO_NONBLOCK, 1);
    fd_socket.fd = fd_socket.socket->socketdes;
    fd_socket.event = event_new(m_base_ev, fd_socket.socket->socketdes, EV_TIMEOUT|EV_READ|EV_CLOSED|EV_PERSIST, OnActive, this);
    event_add(fd_socket.event,NULL);
    m_sockets_map.insert(std::make_pair( fd_socket.socket->socketdes,fd_socket));
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

    apr_thread_mutex_lock(m_apr_send_lock);
    apr_size_t apr_len = len;
    apr_status_t ret = apr_socket_send(fd_socket->second.socket, (const char*)send_buff,&apr_len);
    apr_thread_mutex_unlock(m_apr_send_lock);

    FreeSendBuff(send_buff);
    if (APR_SUCCESS != ret)
    {
        WS_ERROR("apr_socket_send fail!,ret = %d",ret);
        return -1;
    }
    return len;
}

void WSSocketServer::Recv(st_FD_APR_SOCKET fd_socket)
{

    apr_size_t recvLen = m_recv_buff_len;
    char* recv_buff = GetRecvBuff();
    memset(recv_buff,0,m_recv_buff_len);
    apr_status_t  ret = apr_socket_recv(fd_socket.socket, recv_buff, &recvLen);
    if (APR_SUCCESS != ret)
    {
        WS_ERROR("apr_socket_recv fail!,ret = %d",ret);
        FreeRecvBuff(recv_buff);
        return;
    }
    if(m_use_head == true)
    {
        int* agreement =(int*)recv_buff;
        if(*agreement != 0x55aa55aa)
        {
            //WS_WARN("recv unknown message");
            FreeRecvBuff(recv_buff);
            return;
        }
        int* len  = (int*)&recv_buff[4];
        int* type = (int*)&recv_buff[8];

        if(*type == 1) //心跳包
        {
            //WS_DEBUG("recv tick data");
            unsigned long buf[4] = {0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa};
            SendMsg(fd_socket.fd,1,(char*)&buf,16);
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
        inet_ntop4_1((const unsigned char*)(fd_socket->second.socket->remote_addr->ipaddr_ptr), buf, buflen);
        WS_WARN("客户端[%s]断开连接",buf);
        if(pThis->m_close_callback != NULL)
        {
            pThis->m_close_callback(fd_socket->second.fd,pThis->m_close_callback_arg);
        }
        event_del(fd_socket->second.event);
        event_free(fd_socket->second.event);
        apr_socket_close(fd_socket->second.socket);
        pThis->m_sockets_map.erase(fd_socket);
        return;
    }
    if(fd_socket->second.socket == pThis->listen_socket) //新连接
    {
        pThis->Accept();
    }else   //客户端上传数据
    {
        pThis->Recv(fd_socket->second);
    }

}


void* WSSocketServer::WorkDispatch(apr_thread_t* thread, void* arg)
{

    WSSocketServer* pThis = (WSSocketServer*)arg;
    event_base_dispatch(pThis->m_base_ev);
    WS_DEBUG("%p",thread);
    return NULL;
}


char* WSSocketServer::GetSendBuff()
{
    apr_thread_mutex_lock(m_send_buff_lock);
    char* it = NULL;
    if(m_send_buffs.empty())
    {
        it = new char[m_send_buff_len];
    }else
    {
        it = m_send_buffs.front();
        m_send_buffs.pop();
    }

    apr_thread_mutex_unlock(m_send_buff_lock);
    return it;
}
void WSSocketServer::FreeSendBuff(char* buff)
{
    apr_thread_mutex_lock(m_send_buff_lock);
    m_send_buffs.push(buff);
    apr_thread_mutex_unlock(m_send_buff_lock);
}

char* WSSocketServer::GetRecvBuff()
{
    apr_thread_mutex_lock(m_recv_buff_lock);
    char* it = NULL;
    if(m_recv_buffs.empty())
    {
        it = new char[m_recv_buff_len];
    }else
    {
        it = m_recv_buffs.front();
        m_recv_buffs.pop();
    }

    apr_thread_mutex_unlock(m_recv_buff_lock);
    return it;
}
void WSSocketServer::FreeRecvBuff(char* buff)
{
    apr_thread_mutex_lock(m_recv_buff_lock);
    m_recv_buffs.push(buff);
    apr_thread_mutex_unlock(m_recv_buff_lock);
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

    apr_initialize();
    apr_status_t ret = APR_SUCCESS;


    apr_pool_create(&local_pool,NULL);
    ret = apr_sockaddr_info_get(&sa, server_ip, APR_INET, port, 0, local_pool);
    if (APR_SUCCESS != ret)
    {
        WS_ERROR("apr_sockaddr_info_get fail!,ret = %d",ret);
    }

    apr_threadattr_t *attr =NULL;
    ret = apr_threadattr_create(&attr,local_pool);
    if(APR_SUCCESS != apr_thread_create(&work_thread,attr,WSSocketClient::Work,this,local_pool))
    {
        WS_WARN("create work thread failed");
    }

    if(APR_SUCCESS != apr_thread_create(&reconnect_thread,attr,WSSocketClient::ReconnectThread,this,local_pool))
    {
        WS_WARN("create reconnect thread failed");
    }


}

WSSocketClient::~WSSocketClient()
{
    apr_thread_exit(work_thread,0);
    apr_thread_exit(reconnect_thread,0);
    if(new_socket != NULL) apr_socket_close(new_socket);
    delete[] m_send_buff;
    apr_pool_destroy(local_pool);
    apr_terminate();
}
int WSSocketClient::Connect()
{
    if(new_socket != NULL) apr_socket_close(new_socket);
    apr_status_t ret = apr_socket_create(&new_socket, sa->family, SOCK_STREAM, APR_PROTO_TCP, local_pool);
    if (APR_SUCCESS != ret)
    {
        WS_ERROR("create socket fail!,ret = %d",ret);
    }
    apr_socket_opt_set(new_socket, APR_SO_REUSEADDR, 1);
    apr_socket_opt_set(new_socket, APR_SO_KEEPALIVE, 1);
    m_ev = event_new(m_base_ev, new_socket->socketdes, EV_TIMEOUT|EV_READ|EV_CLOSED|EV_PERSIST, RecvMsg, this);

    ret = apr_socket_connect(new_socket, sa);
    if (APR_SUCCESS != ret)
    {
        //WS_ERROR("apr_socket_connect fail!,ret = %d",ret);
        return -1;
    }
    apr_socket_opt_set(new_socket, APR_SO_NONBLOCK, 1);
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

    apr_size_t apr_len = len;
    apr_status_t ret = apr_socket_send(new_socket, (const char*)m_send_buff,&apr_len);
    if (APR_SUCCESS != ret)
    {
        WS_ERROR("apr_socket_send fail!,ret = %d",ret);
        return -1;
    }
    return len;
}

void* WSSocketClient::ReconnectThread(apr_thread_t* thread, void* arg)
{
    WSSocketClient* pThis = (WSSocketClient*)arg;
    while (true)
    {
        if(pThis->m_connect.load() == false)
        {
            pThis->Connect();
        }
        apr_sleep(ONE_SECOND_TIME_MICRO);
    }
    WS_DEBUG("%p",thread);
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

    int d = pThis->new_socket->socketdes;
    if(fd != d)
    {
        WS_WARN("unknown socket:%d",fd);
        return;
    }
    apr_size_t recvLen = pThis->m_buf_len;
    memset(pThis->m_recv_buff,0,pThis->m_buf_len);
    apr_status_t  ret = apr_socket_recv(pThis->new_socket, pThis->m_recv_buff, &recvLen);
    if (APR_SUCCESS != ret)
    {
        WS_ERROR("apr_socket_recv fail!,ret = %d",ret);
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
void* WSSocketClient::Work(apr_thread_t* thread, void* arg)
{

    WSSocketClient* pThis = (WSSocketClient*)arg;
    while (true) {
        while (pThis->m_connect.load() == false) {
            apr_sleep(ONE_SECOND_TIME_MICRO);
        }

        event_base_dispatch(pThis->m_base_ev);

     //   WS_DEBUG("%d",__LINE__);
    }
    WS_DEBUG("end:%p",thread);
}

