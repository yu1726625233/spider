#ifndef SPIDER_H
#define SPIDER_H

#include<iostream>
#include<stdlib.h>
#include<unistd.h>
#include<strings.h>
#include<string>
#include<unordered_set>
#include<unordered_map>
#include<queue>
#include<netdb.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<pthread.h>
#include<openssl/md5.h>
#include<openssl/ssl.h>
#include<openssl/err.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<regex.h>
#include"threadpool.h"
#include"bloomfilter.h"
using namespace std;

#define BLOOMFILTER_BITMAP_SIZE 1024*1024

struct urlnode
{
    string url;
    string domain;
    string  path;
    string file;
    int port;
    string ip;
    bool httptype;
};

class sslnode
{
public:
    sslnode(int sock);
    ~sslnode();
public:
    SSL* sslsock;
    SSL_CTX* sslctx;
};

class Spider
{
public:
    Spider(int n);
    ~Spider();
public:
    void spider_ctrl();
    bool GetUrl(urlnode& node);
    bool AnalyUrl(urlnode& node);
    bool Download(urlnode& node);
    bool SendAndSave(urlnode& node,int sock);
    int GetStatusCode(char* RsHead);
    void AnalyHtml(const char* file);
    bool IsInAndPush(string& link);
    void WriteContent(string& title,string& desc);
public:
    int m_nTargetNum;
    int m_nDoneNum;
    int fd;
    pthread_mutex_t m_lock;
    pthread_mutex_t m_fdlock;
    pthread_mutex_t m_maplock;
    unordered_set<string> m_usUrlMD5;
    BloomFilter bloomfilter;
    queue<string> m_queUrl;
    unordered_map<string,string> m_umDomainToIP;
    ThreadPool pool;
};


#endif // SPIDER_H
