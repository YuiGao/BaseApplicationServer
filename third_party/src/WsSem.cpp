//
// Created by ty on 2020/8/21.
//

#include "WsSem.h"
WsSem::WsSem(int number)
{
    sem_init(&m_signal, 0, number);//总数初始化为 number
}
WsSem::~WsSem()
{
    sem_destroy(&m_signal);
}
void WsSem::wait()
{
    sem_wait(&m_signal);
}
void WsSem::post()
{
    sem_post(&m_signal);
}