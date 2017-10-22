#ifndef TCPCLIENTSOCKET_H
#define TCPCLIENTSOCKET_H

#include <QTcpSocket>
#include <QObject>

class TcpClientSocket : public QObject
{
    Q_OBJECT    //添加（Q_OBJECT）是为了实现信号与槽的通信
public:
    TcpClientSocket(QObject *parent=0, QTcpSocket *clientSocket=0);
    QTcpSocket *clientSocket;
    double number;
    int score;
signals:
    void updateServer(TcpClientSocket*,QString,int);
    void disconnected(int);
protected slots:
    void dataReceived();
    void disconnected();
};

#endif // TCPCLIENTSOCKET_H
