//
// Created by ty on 2020/8/20.
//

#ifndef COMPARESERVER_DATAHANDLECORE_H
#define COMPARESERVER_DATAHANDLECORE_H
#include <list>
#include "wssocket.h"
#include "HttpServer.h"
#include "mythread.h"
#include "WsSem.h"
class DataHandleCore {
public:
    DataHandleCore();
    ~DataHandleCore();
    void start();
    void test();
    void test2();

protected:
    static void Work(void* arg);
    static void OnRecv(int fd,int type, char*buff, int len, void* param);
    static void OnRecv(ST_REQS *req, void *arg);
private:
    WSSocketServer* m_server_socket;
    HttpServer* m_http_server;
   std::list<ST_REQS*> m_reqs;
   MyThread* m_thread;
   WsSem* m_sem;
};


#endif //COMPARESERVER_DATAHANDLECORE_H
