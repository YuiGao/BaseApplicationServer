//
// Created by ty on 2020/8/21.
//

#ifndef COMPARESERVER_WSSEM_H
#define COMPARESERVER_WSSEM_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <semaphore.h>


class WsSem {
public:
    WsSem(int number);
    ~WsSem();
    void wait();
    void post();

private:
    sem_t m_signal;
};


#endif //COMPARESERVER_WSSEM_H
