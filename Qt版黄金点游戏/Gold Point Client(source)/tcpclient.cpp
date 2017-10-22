#pragma execution_character_set("utf-8")

#include "tcpclient.h"

#include <QMessageBox>
#include <QHostInfo>
#include <QTextCodec>

TcpClient::TcpClient(QWidget *parent,Qt::WindowFlags f)
    : QDialog(parent,f)
{
    setWindowTitle(tr("黄金点游戏客户端"));
    scorePromptLabel=new QLabel(tr("当前得分："));
    scoreLabel=new QLabel;
    sendLineEdit=new QLineEdit;
    sendBtn=new QPushButton(tr("发送"));
    userNameLabel=new QLabel(tr("姓名："));
    userNameLineEdit=new QLineEdit;
    serverIPLabel=new QLabel(tr("服务器地址："));
    serverIPLineEdit=new QLineEdit;
    portLabel=new QLabel(tr("端口："));
    portLineEdit=new QLineEdit;
    enterBtn=new QPushButton(tr("进入游戏"));
    mainLayout=new QGridLayout(this);
    mainLayout->addWidget(scorePromptLabel,0,0);
    mainLayout->addWidget(scoreLabel,0,1);
    mainLayout->addWidget(sendLineEdit,1,0);
    mainLayout->addWidget(sendBtn,1,1);
    mainLayout->addWidget(userNameLabel,2,0);
    mainLayout->addWidget(userNameLineEdit,2,1);
    mainLayout->addWidget(serverIPLabel,3,0);
    mainLayout->addWidget(serverIPLineEdit,3,1);
    mainLayout->addWidget(portLabel,4,0);
    mainLayout->addWidget(portLineEdit,4,1);
    mainLayout->addWidget(enterBtn,5,0,1,2);
    scoreLabel->setText(QString::number(0));
    status=false;
    port=9090;
    portLineEdit->setText(QString::number(port));
    serverIP=new QHostAddress();
    qDebug()<<serverIP->toString();
    connect(enterBtn,SIGNAL(clicked(bool)),this,SLOT(enter()));
    connect(sendBtn,SIGNAL(clicked(bool)),this,SLOT(send()));
    sendBtn->setEnabled(false);
}

TcpClient::~TcpClient()
{
    delete scorePromptLabel;
    delete scoreLabel;
    delete sendLineEdit;
    delete sendBtn;
    delete userNameLabel;
    delete userNameLineEdit;
    delete serverIPLabel;
    delete serverIPLineEdit;
    delete portLabel;
    delete portLineEdit;
    delete enterBtn;
    delete mainLayout;
    delete serverIP;
    if(tcpSocket)
        delete tcpSocket;
}

void TcpClient::enter()
{
    if(!status)
    {
        /* 完成输入合法性检验 */
        QString ip=serverIPLineEdit->text();
        if(!serverIP->setAddress(ip))
        {
            QMessageBox::information(this,tr("error"),tr("server ip address error!"));
            return;
        }
        if(userNameLineEdit->text()=="")
        {
            QMessageBox::information(this,tr("error"),tr("User name error!"));
            return;
        }
        userName=userNameLineEdit->text();
        /* 创建了一个QTcpSocket类对象，并将信号/槽连接起来 */
        tcpSocket=new QTcpSocket(this);
        connect(tcpSocket,SIGNAL(connected()),this,SLOT(connected()));
        connect(tcpSocket,SIGNAL(disconnected()),this,SLOT(disconnected()));
        connect(tcpSocket,SIGNAL(readyRead()),this,SLOT(dataReceived()));
        tcpSocket->connectToHost(*serverIP,port);
        if(tcpSocket->waitForConnected())
            status=true;
    }
    else
    {
        tcpSocket->disconnectFromHost();
        status=false;
    }
}

void TcpClient::connected()
{
    sendBtn->setEnabled(true);
    enterBtn->setText(tr("离开游戏"));
}

void TcpClient::send()
{
    if(sendLineEdit->text()=="")
        return;
    QTextCodec *codec=QTextCodec::codecForName("UTF-8");
    tcpSocket->write(codec->fromUnicode(sendLineEdit->text()),sendLineEdit->text().length());
    sendLineEdit->clear();
}

void TcpClient::disconnected()
{
    scoreLabel->setText(QString::number(0));
    status=false;
    sendBtn->setEnabled(false);
    enterBtn->setText(tr("进入游戏"));
}

void TcpClient::dataReceived()
{
    while(tcpSocket->bytesAvailable()>0)
    {
        QByteArray datagram;
        datagram.resize(tcpSocket->bytesAvailable());
        tcpSocket->read(datagram.data(),datagram.size());
        QString msg=datagram.data();
        if(datagram.size()>0)
            scoreLabel->setText(msg.left(datagram.size()));
    }
}
