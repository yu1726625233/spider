#include"spider.h"

int main()
{
    Spider spider(1000);
    string str="https://baike.baidu.com/item/%E8%8B%B1%E9%9B%84%E8%81%94%E7%9B%9F/4615671?fromtitle=LOL&fromid=6978147";
    spider.IsInAndPush(str);
    spider.spider_ctrl();
    while(1)
    {
        sleep(2);
        cout<<"done "<<spider.m_nDoneNum<<endl;
    }
    return 0;
}
