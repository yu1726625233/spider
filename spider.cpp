#include "spider.h"

Spider::Spider(int n):bloomfilter(BLOOMFILTER_BITMAP_SIZE)
{
    m_nTargetNum=n;
    m_nDoneNum=0;
    pool.Init(10,50,500);
    pthread_mutex_init(&m_lock,NULL);
    pthread_mutex_init(&m_fdlock,NULL);
    pthread_mutex_init(&m_maplock,NULL);
    fd=open("donwloadfile",O_RDWR|O_CREAT,0664);

    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_ssl_algorithms();
}

Spider::~Spider()
{
    pthread_mutex_destroy(&m_lock);
    pthread_mutex_destroy(&m_fdlock);
    pthread_mutex_destroy(&m_maplock);
    close(fd);
}

void Spider::spider_ctrl()
{
    urlnode node;
    if(!GetUrl(node))
        return ;

    AnalyUrl(node);

    if(Download(node))
    {
        AnalyHtml(node.file.c_str());
    }
}

bool Spider::GetUrl(urlnode &node)
{
    pthread_mutex_lock(&m_lock);
    if(m_queUrl.empty() || m_nDoneNum == m_nTargetNum)
    {
        pthread_mutex_unlock(&m_lock);
        return false;
    }
    node.url=m_queUrl.front();
    m_queUrl.pop();
    m_nDoneNum++;
    pthread_mutex_unlock(&m_lock);
    return true;;
}

bool Spider::AnalyUrl(urlnode &node)
{
    string str="http://";
    int domainbeginindex=0;
    if(node.url.compare(0,7,str) == 0)
    {
        node.httptype=0;
        node.port=80;
        domainbeginindex=7;
    }else
    {
        node.httptype=1;
        node.port=443;
        domainbeginindex=8;
    }

    int pathbeginindex=node.url.find('/',domainbeginindex);
    node.domain=node.url.substr(domainbeginindex,pathbeginindex-domainbeginindex);

    int filebeginindex=node.url.rfind('/',node.url.size()-1)+1;
    node.path=node.url.substr(pathbeginindex,filebeginindex-pathbeginindex);

    node.file="filedir/"+node.url.substr(filebeginindex,node.url.size()-filebeginindex);

    pthread_mutex_lock(&m_maplock);
    if(m_umDomainToIP.find(node.domain) == m_umDomainToIP.end())
    {
        hostent* ent = gethostbyname(node.domain.c_str());
        if(ent)
        {
            m_umDomainToIP[node.domain]=string(ent->h_addr_list[0]);
        }
    }
    node.ip=m_umDomainToIP[node.domain];
    pthread_mutex_unlock(&m_maplock);

//    hostent* ent;
//    ent = gethostbyname(node.domain.c_str());
//    if(ent)
//    {
//        node.ip=string(ent->h_addr_list[0]);
//    }

    return true;
}

bool Spider::Download(urlnode &node)
{
    int sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock < 0)
    {
        cout<<"sock failed"<<endl;
        return false;
    }
    sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_port=htons(node.port);
    addr.sin_addr.s_addr=*(unsigned int*)node.ip.c_str();
    //inet_pton(AF_INET,node.ip.c_str(),&addr.sin_addr.s_addr);
    if(-1 == connect(sock,(const sockaddr*)&addr,sizeof(addr)))
    {
        cout<<"connect failed"<<endl;
        close(sock);
        return false;
    }
    bool res=SendAndSave(node,sock);
    close(sock);
    return res;
}

bool Spider::SendAndSave(urlnode &node, int sock)
{
    char RqHead[4096];
    bzero(RqHead,sizeof(RqHead));
    sprintf(RqHead,"GET %s HTTP/1.1\r\n"\
                   "Accept:text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
                   "User-Agent:Mozilla/5.0 (x11, Linux x86_64)\r\n"\
                   "Host:%s\r\n"\
                   "Connection:Close\r\n\r\n",node.url.c_str(),node.domain.c_str());

    char recvBuf[4096];
    bzero(recvBuf,sizeof(recvBuf));
    char RsHead[4096];
    bzero(RsHead,sizeof(RsHead));
    int recvNum=0;
    int fd;
    if(node.httptype)
    {
        sslnode ssl(sock);
        if(SSL_write(ssl.sslsock,RqHead,strlen(RqHead)) < 0)
            return false;
        recvNum=SSL_read(ssl.sslsock,recvBuf,sizeof(recvBuf));
        if(recvNum <= 0)
            return false;
        char* pos=strstr(recvBuf,"\r\n\r\n");
        if(pos == NULL)
            return false;
        strncpy(RsHead,recvBuf,pos-recvBuf+4);
        int StatusCode=GetStatusCode(RsHead);
        if(StatusCode != 200)
            return false;
        fd=open(node.file.c_str(),O_RDWR|O_CREAT,0666);
        write(fd,pos+4,recvNum-(pos-recvBuf+4));
        while((recvNum=SSL_read(ssl.sslsock,recvBuf,sizeof(recvBuf))) > 0)
            write(fd,recvBuf,recvNum);
    }
    else
    {
        if(send(sock,RqHead,strlen(RqHead),0) < 0)
            return false;
        recvNum=recv(sock,recvBuf,sizeof(recvBuf),0);
        if(recvNum <= 0)
            return false;
        char* pos=strstr(recvBuf,"\r\n\r\n");
        if(pos == NULL)
            return false;
        strncpy(RsHead,recvBuf,pos-recvBuf+4);
        int StatusCode=GetStatusCode(RsHead);
        if(StatusCode != 200)
            return false;
        fd=open(node.file.c_str(),O_RDWR|O_CREAT,0666);
        write(fd,pos+4,recvNum-(pos-recvBuf+4));
        while((recvNum=recv(sock,recvBuf,sizeof(recvBuf),0)) > 0)
            write(fd,recvBuf,recvNum);
    }

    close(fd);
    return true;
}

int Spider::GetStatusCode(char *RsHead)
{
    char* pos=strstr(RsHead," ");
    if(pos == NULL) return -1;
    pos++;
    int res=0;
    for(int i=0;i<3;i++)
    {
        res=res*10+*pos++-'0';
    }
    return res;
}

class Task:public ITask
{
public:
    Task(Spider* p)
    {
        pSpider=p;
    }
    void workfunc()
    {
        pSpider->spider_ctrl();
    }
public:
    Spider* pSpider;
};

void Spider::AnalyHtml(const char *file)
{
    int fd=open(file,O_RDWR);
    int filesize=lseek(fd,0,SEEK_END);
    char* pstr=(char*)mmap(NULL,filesize,PROT_WRITE|PROT_READ,MAP_PRIVATE,fd,0);
//    printf("fd=%d,file=%s,p=%p\n",fd,file,pstr);
    char* fstr=pstr;
    close(fd);

    string title,desc;
    regex_t hreg,dreg,areg;
    regcomp(&hreg,"<h1 >\\([^<]\\+\\?\\)</h1>",0);
    regcomp(&dreg,"<meta name=\"description\" content=\"\\([^\"]\\+\\?\\)\">",0);
    regcomp(&areg,"<a[^>]\\+\\?href=\"\\(/item/[^\"]\\+\\?\\)\"[^>]\\+\\?>[^<]\\+\\?</a>",0);
    regmatch_t hmatch[2];
    regmatch_t dmatch[2];
    regmatch_t amatch[2];
    if(regexec(&hreg,pstr,2,hmatch,0) == 0)
        title.append(pstr+hmatch[1].rm_so,hmatch[1].rm_eo-hmatch[1].rm_so);
    if(regexec(&dreg,pstr,2,dmatch,0) == 0)
        desc.append(pstr+dmatch[1].rm_so,dmatch[1].rm_eo-dmatch[1].rm_so);
    while(regexec(&areg,pstr,2,amatch,0) == 0)
    {
        if(m_queUrl.size()>=150) break;
        string link="https://baike.baidu.com";
        link.append(pstr+amatch[1].rm_so,amatch[1].rm_eo-amatch[1].rm_so);
        pstr+=amatch[0].rm_eo;
        if(IsInAndPush(link))
        {
            ITask* ptask=new Task(this);
            pool.AddTask(ptask);
        }

    }

    WriteContent(title,desc);

    regfree(&hreg);
    regfree(&dreg);
    regfree(&areg);
    munmap(fstr,filesize);
    unlink(file);
}

bool Spider::IsInAndPush(string &link)
{
    unsigned char temp[17];
    MD5((const unsigned char*)link.c_str(),link.size(),temp);
    string md5(temp,temp+16);

    bool flag=bloomfilter.NotInAndInsert(md5);
    if(flag)
    {
        pthread_mutex_lock(&m_lock);
        m_queUrl.push(link);
        pthread_mutex_unlock(&m_lock);
        return true;
    }
    return false;
//    pthread_mutex_lock(&m_lock);
//    if(m_usUrlMD5.find(md5)!=m_usUrlMD5.end())
//    {
//        pthread_mutex_unlock(&m_lock);
//        return false;
//    }
//    m_queUrl.push(link);
//    m_usUrlMD5.insert(md5);
//    pthread_mutex_unlock(&m_lock);
//    return true;
}

void Spider::WriteContent(string &title, string &desc)
{
    pthread_mutex_lock(&m_fdlock);
    write(fd,title.c_str(),title.size());
    write(fd,"\n",1);
    write(fd,desc.c_str(),desc.size());
    write(fd,"\n\n",2);
    pthread_mutex_unlock(&m_fdlock);
}

sslnode::sslnode(int sock)
{
    sslctx=SSL_CTX_new(SSLv23_method());
    sslsock=SSL_new(sslctx);
    SSL_set_fd(sslsock,sock);
    SSL_connect(sslsock);
}

sslnode::~sslnode()
{
    SSL_CTX_free(sslctx);
    SSL_free(sslsock);
}
