#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QObject>
#include "tcpclientsocket.h"    //包含TCP套接字
#include <QQueue>

class Server : public QTcpServer
{
    Q_OBJECT    //添加宏（Q_OBJECT）是为了实现信号与槽的通信
public:
    Server(QObject *parent=0,int port=0);
    QList<TcpClientSocket*> tcpClientSocketList;
    QQueue<TcpClientSocket*> queue;
signals:
    void updateClients();
    void showCurrentConnection();
public slots:
    void updateServer(TcpClientSocket*,QString,int);
    void disconnected(int);
    void incomingConnection();
};

#endif // SERVER_H
