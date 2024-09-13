#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QWidget>
#include <QLabel>
//聊天信息
class ChatMessage:public QWidget
{
    Q_OBJECT
public:
    //explicit是一个关键字，用于控制隐式类型转换
    explicit ChatMessage(QWidget *parent = nullptr);

    enum User_Type{
        User_System,//系统
        User_Me,    //自己
        User_She,   //用户
        User_Time,  //时间
    };
    void setTextSuccess();
    void setText(QString text, QString time, QSize allSize, QString ip = "",  User_Type userType = User_Time);
    void getm_ipLeftRect(QPixmap pp);
    void getm_ipRightRect(QPixmap pp);


    QSize getRealString(QString src);
    QSize fontRect(QString str);
    QPixmap m_leftPixmap;
    QPixmap m_rightPixmap;
    inline QString text() {return m_msg;}
    inline QString time() {return m_time;}
    inline User_Type userType() {return m_userType;}
protected:
    void paintEvent(QPaintEvent *event);
private:
    QString m_msg;
    QString m_time;
    QString m_curTime;
    QString m_ip;

    QSize m_allSize;
    User_Type m_userType = User_System;

    int m_kuangWidth;
    int m_textWidth;
    int m_spaceWid;
    int m_lineHeight;

    QRect m_ipLeftRect;
    QRect m_ipRightRect;
    QRect m_iconLeftRect;
    QRect m_iconRightRect;
    QRect m_sanjiaoLeftRect;
    QRect m_sanjiaoRightRect;
    QRect m_kuangLeftRect;
    QRect m_kuangRightRect;
    QRect m_textLeftRect;
    QRect m_textRightRect;

    QLabel* m_loading = Q_NULLPTR;
    QMovie* m_loadingMovie = Q_NULLPTR;
    bool m_isSending = false;
};

#endif // CHATMESSAGE_H
