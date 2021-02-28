#pragma execution_character_set("utf-8")
#include "tcpserver.h"

TcpServer::TcpServer(QWidget *parent,Qt::WindowFlags f)
    : QDialog(parent,f)
{
    setWindowTitle(tr("黄金点游戏服务器"));
    currentLabel = new QLabel(tr("当前在线人数："));
    currentNumberLabel = new QLabel;
    maxNumberLabel = new QLabel(tr("游戏人数上限："));
    maxNumberLineEdit = new QLineEdit;
    settingBtn = new QPushButton(tr("设置游戏人数"));
    startGameBtn = new QPushButton(tr("强制开始游戏"));
    portLabel = new QLabel(tr("端口："));
    portLineEdit = new QLineEdit;
    createBtn = new QPushButton(tr("创建游戏房间"));
    mainLayout = new QGridLayout(this);
    mainLayout->addWidget(currentLabel,0,0);
    mainLayout->addWidget(currentNumberLabel,0,1);
    mainLayout->addWidget(maxNumberLabel,1,0);
    mainLayout->addWidget(maxNumberLineEdit,1,1);
    mainLayout->addWidget(settingBtn,2,0,1,2);
    mainLayout->addWidget(startGameBtn,3,0,1,2);
    mainLayout->addWidget(portLabel,4,0);
    mainLayout->addWidget(portLineEdit,4,1);
    mainLayout->addWidget(createBtn,5,0,1,2);
    currentNumberLabel->setText(QString::number(0));
    startGameBtn->setEnabled(false);
    isRunning=false;
    maxNumber=5;
    maxNumberLineEdit->setText(QString::number(maxNumber));
    port=9090;
    portLineEdit->setText(QString::number(port));
    connect(createBtn,SIGNAL(clicked(bool)),this,SLOT(createServer()));
    connect(settingBtn,SIGNAL(clicked(bool)),this,SLOT(setMaxNumber()));
    connect(startGameBtn,SIGNAL(clicked(bool)),this,SLOT(startGame()));
}

TcpServer::~TcpServer()
{
    delete currentLabel;
    delete currentNumberLabel;
    delete maxNumberLabel;
    delete maxNumberLineEdit;
    delete settingBtn;
    delete startGameBtn;
    delete portLabel;
    delete portLineEdit;
    delete createBtn;
    delete mainLayout;
    if(server)
        delete server;
}

void TcpServer::createServer()
{
    if(!isRunning)
    {
        server=new Server(this,port);   //创建一个Server对象
        qDebug()<<server->serverAddress().toString();
        qDebug()<<server->serverPort();
        startGameBtn->setEnabled(true);
        connect(server,SIGNAL(updateClients()),this,SLOT(updateClients()));
        connect(server,SIGNAL(showCurrentConnection()),this,SLOT(showCurrentConnection()));
        createBtn->setText(tr("结束游戏"));
        isRunning=true;
    }
    else
    {
        disconnect(server,SIGNAL(updateClients()),this,SLOT(updateClients()));
        disconnect(server,SIGNAL(showCurrentConnection()),this,SLOT(showCurrentConnection()));
        delete server;
        startGameBtn->setEnabled(false);
        currentNumberLabel->setText(QString::number(0));
        createBtn->setText(tr("创建游戏房间"));
        isRunning=false;
    }
}

void TcpServer::updateClients()
{
    qDebug()<<"许聪";
    if(server->queue.size()>=maxNumber)
        startGame();
}

void TcpServer::setMaxNumber()
{
    int temp=maxNumberLineEdit->text().toInt();
    if(temp>0)
        maxNumber=temp;
}

void TcpServer::showCurrentConnection()
{
    currentNumberLabel->setText(QString::number(server->tcpClientSocketList.size()));
}

void TcpServer::startGame()
{
    if(server->queue.size()<2)
        return;

    int number=server->queue.size()<maxNumber?server->queue.size():maxNumber;

    int goldPoint = 0;
    for(int i=0;i<number;++i)
        goldPoint+=server->queue.at(i)->number;
    goldPoint*=(sqrt((double)5) - 1) / 2;
    goldPoint/=number;

    double min = abs(server->queue.at(0)->number);
    for(int i=0;i<number;++i)
        if(abs(server->queue.at(i)->number)-abs(min)>0)
            min=server->queue.at(i)->number;

    double *gap = new double[number];
    for(int i=0;i<number;++i)
    {
        gap[i]=abs(goldPoint-server->queue.at(i)->number);
        if(gap[i]-min<0)
            min=gap[i];
    }
    for(int i=0;i<number;++i)
    {
        if(gap[i]-min==0)
            ++server->queue.at(i)->score;
    }
    delete gap;
    for(int i=0;i<number;++i)
    {
        QString msg=QString::number(server->queue.first()->score);
        server->queue.first()->clientSocket->write(msg.toLatin1(),msg.length());
        qDebug()<<msg;
        server->queue.pop_front();
    }
}
