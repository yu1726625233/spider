#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<pthread.h>
#include<vector>
#include<queue>
#include<iostream>
#include<unistd.h>
using namespace std;

class ITask
{
public:
    virtual void workfunc() = 0 ;
};

class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool();
public:
    bool Init(int min,int max,unsigned long maxtask);
    bool AddTask(ITask* pTask);
    static void* customer(void* pvoid);
    static void* manager(void* pvoid);
public:
    bool m_bQuitFlag;
    int m_minThread;
    int m_maxThread;
    int m_busyThread;
    unsigned long m_maxTask;

    pthread_t m_tidManager;

    pthread_mutex_t m_lock;
    pthread_cond_t m_notempty;
    pthread_cond_t m_notfull;

    vector<pthread_t> m_vecTids;
    queue<ITask*> m_queTask;
};

#endif // THREADPOOL_H
