#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>
#include "server.h"

class TcpServer : public QDialog
{
    Q_OBJECT
public:
    TcpServer(QWidget *parent = 0, Qt::WindowFlags f=0);
    ~TcpServer();
private:
    QLabel *currentLabel;
    QLabel *currentNumberLabel;
    QLabel *maxNumberLabel;
    QLineEdit *maxNumberLineEdit;
    QPushButton *settingBtn;
    QPushButton *startGameBtn;
    QLabel *portLabel;
    QLineEdit *portLineEdit;
    QPushButton *createBtn;
    QGridLayout *mainLayout;
    int maxNumber;
    bool isRunning;
    int port;
    Server *server;
public slots:
    void setMaxNumber();
    void showCurrentConnection();
    void startGame();
    void createServer();
    void updateClients();
};

#endif // TCPSERVER_H
