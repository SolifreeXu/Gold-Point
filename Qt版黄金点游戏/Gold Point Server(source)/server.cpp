#include "server.h"

Server::Server(QObject *parent,int port)
    :QTcpServer(parent)
{
    listen(QHostAddress::Any,port);
    connect(this,SIGNAL(newConnection()),this,SLOT(incomingConnection()));
}

void Server::incomingConnection()
{
    QTcpSocket *clientSocket=this->nextPendingConnection();
    TcpClientSocket *tcpClientSocket=new TcpClientSocket(this,clientSocket);
    connect(tcpClientSocket,SIGNAL(updateServer(TcpClientSocket*,QString,int)),this,SLOT(updateServer(TcpClientSocket*,QString,int)));
    connect(tcpClientSocket,SIGNAL(disconnected(int)),this,SLOT(disconnected(int)));
    qDebug()<<tcpClientSocket->clientSocket->socketDescriptor();
    tcpClientSocketList.append(tcpClientSocket);
    qDebug()<<tcpClientSocketList.count();
    emit showCurrentConnection();
}

void Server::updateServer(TcpClientSocket *clientSocjet, QString msg, int length)
{
    for(int i=0;i<queue.size();++i)
        if(queue.at(i)==clientSocjet)
            return;
    clientSocjet->number=msg.left(length).toDouble();
    queue.push_back(clientSocjet);
    emit updateClients();
}

void Server::disconnected(int descriptor)
{
    for(int i=0;i<tcpClientSocketList.count();++i)
    {
        TcpClientSocket *item=tcpClientSocketList.at(i);
        if(item->clientSocket->socketDescriptor()==descriptor)
        {
            tcpClientSocketList.removeAt(i);
            emit showCurrentConnection();
            return;
        }
    }
}
