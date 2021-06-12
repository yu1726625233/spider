#include "bloomfilter.h"

BloomFilter::BloomFilter(int cap)
{
    m_bitmap=new char[cap];
    m_cap=cap*8;
    m_vecSeed={ 5, 7, 11, 13, 31, 37, 61 };
    m_hashnum=m_vecSeed.size();
    pthread_mutex_init(&m_lock,NULL);
}

BloomFilter::~BloomFilter()
{
    if(m_bitmap)
    {
        delete[] m_bitmap;
        m_bitmap=NULL;
    }
    pthread_mutex_destroy(&m_lock);
}

bool BloomFilter::NotInAndInsert(string str)
{
    vector<int> bitvec(m_hashnum);
    for(int i=0;i<m_hashnum;i++)
    {
        bitvec[i]=hashfunc(str,m_vecSeed[i]);
    }
    char c;
    int index;
    bool flag=true;
    pthread_mutex_lock(&m_lock);
    for(int i=0;i<m_hashnum && flag;i++)
    {
        c=m_bitmap[bitvec[i]/8];
        index=1<<bitvec[i]%8;
        flag =flag && (index & c);
    }
    bool res=true;
    if(!flag)
    {
        for(int i=0;i<m_hashnum;i++)
        {
            m_bitmap[bitvec[i]/8] = m_bitmap[bitvec[i]/8] | 1<<bitvec[i]%8;
        }
    }
    else
        res=false;
    pthread_mutex_unlock(&m_lock);
    return res;
}

int BloomFilter::hashfunc(string str,int seed)
{
    int res=0;
    for(auto c:str)
    {
        res=res*seed+c;
    }
    return (m_cap-1)&res /*res%m_cap*/;
}
