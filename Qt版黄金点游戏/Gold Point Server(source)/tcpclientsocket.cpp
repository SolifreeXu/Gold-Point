#include "tcpclientsocket.h"

#include <QtextCodec>

TcpClientSocket::TcpClientSocket(QObject *parent, QTcpSocket *clientSocket)
    : QObject(parent), clientSocket(clientSocket)
{
    score=0;
    connect(clientSocket,SIGNAL(readyRead()),this,SLOT(dataReceived()));
    connect(clientSocket,SIGNAL(disconnected()),this,SLOT(disconnected()));
}

void TcpClientSocket::dataReceived()
{
    while(clientSocket->bytesAvailable()>0)
    {
        int length=clientSocket->bytesAvailable();
        char buf[1024];
        clientSocket->read(buf,length);
        qDebug()<<buf;
        QString msg=buf;
        QTextCodec *codec=QTextCodec::codecForName("UTF-8");
        emit updateServer(this,codec->fromUnicode(msg),length);
    }
}

void TcpClientSocket::disconnected()
{
    qDebug()<<clientSocket->objectName();
    emit disconnected(clientSocket->socketDescriptor());
}
