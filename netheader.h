#ifndef NETHEADER_H
#define NETHEADER_H
#include <QMetaType>
#include <QMutex>
#include <QQueue>
#include <QImage>
#include <QWaitCondition>

//网络包模块
#define QUEUE_MAXSIZE 1500
#ifndef MB
#define MB 1024*1024
#endif

#ifndef KB
#define KB 1024
#endif

#ifndef WAITSECONDS
#define WAITSECONDS 2
#endif

#ifndef OPENVIDEO
#define OPENVIDEO "打开视频"
#endif

#ifndef CLOSEVIDEO
#define CLOSEVIDEO "关闭视频"
#endif

#ifndef OPENAUDIO
#define OPENAUDIO "打开音频"
#endif

#ifndef CLOSEAUDIO
#define CLOSEAUDIO "关闭音频"
#endif


#ifndef MSG_HEADER
#define MSG_HEADER 11
#endif

enum MSG_TYPE
{
    IMG_SEND = 0,  /* 图片发送 */
    IMG_RECV,       /* 图片接收 */
    AUDIO_SEND,     /* 音频发送 */
    AUDIO_RECV,     /* 音频接收 */
    TEXT_SEND,      /* 文本发送 */
    TEXT_RECV,      /* 文本接收 */
    CREATE_MEETING, /* 创建会议 */
    EXIT_MEETING,  /* 退出会议 */
    JOIN_MEETING,  /* 加入会议 */
    CLOSE_CAMERA,   /* 关闭摄像头 */

    CREATE_MEETING_RESPONSE = 20, //创建会议响应
    PARTNER_EXIT = 21,   //会议伙伴退出
    PARTNER_JOIN = 22,   //会议伙伴加入
    JOIN_MEETING_RESPONSE = 23,  //加入会议响应
    PARTNER_JOIN2 = 24,  //合作伙伴加入2
    RemoteHostClosedError = 40,  //远程主机失败
    OtherNetError = 41   //其他网络问题
};
Q_DECLARE_METATYPE(MSG_TYPE);

struct MESG //消息结构体
{
    MSG_TYPE msg_type; //信息类别
    uchar* data;  //传递的数据指针
    long len; //
    quint32 ip; //发送方ip
};
Q_DECLARE_METATYPE(MESG *);

//-------------------------------

template<class T>
struct QUEUE_DATA //消息队列
{
private:
    QMutex send_queueLock; //消息队列锁
    QWaitCondition send_queueCond; //线程间的等待和唤醒机制
    QQueue<T*> send_queue; //于存储消息对象
public:
    void push_msg(T* msg) //加入消息队列
    {
        send_queueLock.lock();
        while(send_queue.size() > QUEUE_MAXSIZE)
        {
            send_queueCond.wait(&send_queueLock); //超出线程队列,在队列外等待,等待其他线程wakeone
        }
        send_queue.push_back(msg);  //压入队列
        send_queueLock.unlock();
        send_queueCond.wakeOne();//唤醒等待的线程（如果有的话
    }

    T* pop_msg() //从队列中获取一个消息对象
    {
        send_queueLock.lock();
        while(send_queue.size() == 0)
        {
            bool f = send_queueCond.wait(&send_queueLock, WAITSECONDS * 1000);
            if(f == false)
            {
                send_queueLock.unlock();
                return NULL;
            }
        }
        T* send = send_queue.front();
        send_queue.pop_front();
        send_queueLock.unlock();
        send_queueCond.wakeOne();
        return send;
    }

    void clear()
    {
        send_queueLock.lock();
        send_queue.clear();
        send_queueLock.unlock();
    }
};

struct Log
{
    char *ptr;
    uint len;
};



void log_print(const char *, const char *, int, const char *, ...);
#define WRITE_LOG(LOGTEXT, ...) do{ \
    log_print(__FILE__, __FUNCTION__, __LINE__, LOGTEXT, ##__VA_ARGS__);\
}while(0);



#endif // NETHEADER_H
