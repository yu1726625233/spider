#include "threadpool.h"

ThreadPool::ThreadPool()
{
    m_bQuitFlag=true;
    m_minThread=0;
    m_maxThread=0;
    m_busyThread=0;
    m_maxTask=0;
}

ThreadPool::~ThreadPool()
{
    m_bQuitFlag=false;
    //...

    pthread_mutex_destroy(&m_lock);
    pthread_cond_destroy(&m_notfull);
    pthread_cond_destroy(&m_notempty);
}

bool ThreadPool::Init(int min, int max, unsigned long maxtask)
{
    m_minThread=min;
    m_maxThread=max;
    m_busyThread=0;
    m_maxTask=maxtask;

    pthread_mutex_init(&m_lock,NULL);
    pthread_cond_init(&m_notfull,NULL);
    pthread_cond_init(&m_notempty,NULL);

    for(int i=0;i<m_minThread;i++)
    {
        pthread_t tid;
        if(0 != pthread_create(&tid,NULL,ThreadPool::customer,this))
        {
            cout<<"create customer failed"<<endl;
            return false;
        }
        m_vecTids.push_back(tid);
    }

    if(0 != pthread_create(&m_tidManager,NULL,ThreadPool::manager,this))
    {
        cout<<"create manager failed"<<endl;
        return false;
    }

    return true;
}

bool ThreadPool::AddTask(ITask *pTask)
{
    pthread_mutex_lock(&m_lock);
    if(m_bQuitFlag)
    {
        if(m_queTask.size() == m_maxTask)
        {
            pthread_cond_wait(&m_notfull,&m_lock);
            if(!m_bQuitFlag)
            {
                pthread_mutex_unlock(&m_lock);
                return false;
            }
        }
        m_queTask.push(pTask);
        pthread_cond_signal(&m_notempty);
    }
    pthread_mutex_unlock(&m_lock);
    return true;
}

void* ThreadPool::customer(void *pvoid)
{
    ThreadPool* pThis=(ThreadPool*)pvoid;
    while(pThis->m_bQuitFlag)
    {
        pthread_mutex_lock(&pThis->m_lock);
        if(!pThis->m_bQuitFlag)
        {
            pthread_mutex_unlock(&pThis->m_lock);
            return NULL;
        }
        while(pThis->m_queTask.empty())
        {

            pthread_cond_wait(&pThis->m_notempty,&pThis->m_lock);
            if(!pThis->m_bQuitFlag)
            {
                pthread_mutex_unlock(&pThis->m_lock);
                return NULL;
            }
        }
        ITask* pTask=pThis->m_queTask.front();
        pThis->m_queTask.pop();
        pThis->m_busyThread++;
        pthread_cond_signal(&pThis->m_notfull);
        pthread_mutex_unlock(&pThis->m_lock);

        pTask->workfunc();

        delete pTask;
        pTask=NULL;

        pthread_mutex_lock(&pThis->m_lock);
        pThis->m_busyThread--;
        pthread_mutex_unlock(&pThis->m_lock);
    }
    return NULL;
}

void *ThreadPool::manager(void *pvoid)
{
    ThreadPool* pThis=(ThreadPool*)pvoid;

    int busy;
    int curtask;
    int alive;
    while(pThis->m_bQuitFlag)
    {
        busy=pThis->m_busyThread;
        curtask=pThis->m_queTask.size();
        alive=pThis->m_vecTids.size();
        //cout<<"cur:"<<curtask<<" busy:"<<busy<<" alive:"<<alive<<endl;
        if(curtask > alive-busy && alive+pThis->m_minThread <= pThis->m_maxThread)
        {
            for(int i=0;i<pThis->m_minThread;i++)
            {
                pthread_t tid;
                if(0 != pthread_create(&tid,NULL,ThreadPool::customer,pThis))
                    continue;
                pThis->m_vecTids.push_back(tid);
            }
        }
        sleep(2);
    }
}
