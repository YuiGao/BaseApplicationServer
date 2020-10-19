//
// Created by ty on 2020/8/21.
//

#ifndef COMPARESERVER_HTTPSERVER_H
#define COMPARESERVER_HTTPSERVER_H

#include <event2/bufferevent.h>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <event2/http_struct.h>
#include <event2/thread.h>
#include <string>
#include "base.h"
//请求结构体处理
typedef struct reqs{
    clock_t start;			//开始时间
    char serial[20];
    int index;				//序列号 1--1000循环
    std::string url;
    struct evhttp_request * req;
    reqs(){
        index = 0;
        req = NULL;
    }

}ST_REQS;

typedef void (*HttpRecvCallback)(ST_REQS *req, void *arg);

class HttpServer {
public:

    HttpServer(int listen_port);
    ~HttpServer();
    int Init();
    void SetCallback(HttpRecvCallback call,void* arg){m_call = call;m_arg = arg;}
    static void send_document_cb(struct evhttp_request *req, void *arg);
    static int send_error(ST_REQS* req, en_RESPONSE_CODE code, const char* extra = NULL);
    static int send_success(ST_REQS* req, std::string& reponse);
private:

    int m_listen_port;
    int m_recv_index;
    HttpRecvCallback m_call;
    void* m_arg;
};


#endif //COMPARESERVER_HTTPSERVER_H
