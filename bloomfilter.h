#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H
#include<vector>
#include<string>
#include<pthread.h>
using namespace std;
class BloomFilter
{
public:
    BloomFilter(int cap);
    ~BloomFilter();
public:
    bool NotInAndInsert(string str);
public:
    char* m_bitmap;
    int m_cap;
    int m_hashnum;
    pthread_mutex_t m_lock;
    vector<int> m_vecSeed;
    int hashfunc(string str,int seed);
};

#endif // BLOOMFILTER_H
