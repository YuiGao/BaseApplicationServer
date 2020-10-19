//
// Created by ty on 2020/8/20.
//

#include "DataHandleCore.h"

#include <unistd.h>
//#include "face_tool.h"
#include "wslog.h"

//#include "GetFeature.h"

#include "cdzs_facereco.h"

#define MAT_HEAD(mat) img.data, img.channels(), img.cols, img.rows, img.step[0]

DataHandleCore::DataHandleCore()
{
    m_sem = new WsSem(0);
    m_thread = new MyThread(DataHandleCore::Work, this);

    m_http_server = new HttpServer(8087);
    m_http_server->SetCallback(DataHandleCore::OnRecv, this);

    m_server_socket = new WSSocketServer(8089);
    m_server_socket->SetServerRecvCallBack(DataHandleCore::OnRecv, this);
}
DataHandleCore::~DataHandleCore()
{
    delete m_sem;
    delete m_thread;
    delete m_http_server;
}
void DataHandleCore::start()
{
    m_server_socket->Listen(4000);
    m_http_server->Init();
}
void DataHandleCore::Work(void *arg)
{

    DataHandleCore *pThis = (DataHandleCore *)arg;
    pThis->m_sem->wait();
    if (pThis->m_reqs.size() == 0)
    {
        sleep(1);
        return;
    }
    auto it = pThis->m_reqs.begin();
    struct evkeyvalq *headers = evhttp_request_get_input_headers((*it)->req); //返回输入标头
    const char *key = evhttp_find_header(headers, "Content-Length");          //查找属于标题的值，实体长度

    struct evbuffer *buf = evhttp_request_get_input_buffer((*it)->req); //返回输出缓冲区

    int len = atoi(key);

    char data[256] = {0};
    evbuffer_copyout(buf, data, len); //复制数据

    WS_DEBUG("recv:%s", data);
    std::string message = "{\"复制数据\":\"2\"}";
    HttpServer::send_success(*it, message);
    pThis->m_reqs.erase(it);
}
void DataHandleCore::OnRecv(ST_REQS *req, void *arg)
{
    DataHandleCore *pThis = (DataHandleCore *)arg;
    pThis->m_reqs.push_back(req);
    pThis->m_sem->post();
}
void DataHandleCore::OnRecv(int fd, int type, char *buff, int len, void *param)
{
    DataHandleCore *pThis = (DataHandleCore *)param;
    WS_DEBUG("recv:%s", buff);
    pThis->m_server_socket->SendMsg(fd, type, (char*)"recv", 4);
}

#include "FL_Det_V1.h"

