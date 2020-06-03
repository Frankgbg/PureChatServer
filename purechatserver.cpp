#include "purechatserver.h"
#include "ui_purechatserver.h"

#include <QDir>
#include <QGridLayout>
#include <QLabel>

PureChatServer::PureChatServer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PureChatServer)
{
    ui->setupUi(this);
    serverSocket = nullptr;
    QDir path;
    path.mkpath("./msg");
    setWindowTitle("PureChatServer");
}

PureChatServer::~PureChatServer()
{
    delete ui;
    if(serverSocket != nullptr) delete serverSocket;
}

void PureChatServer::on_pushButton_clicked()
{
    if(serverSocket != nullptr) return ;
    ui->textBrowser->clear();
    ui->textBrowser->append("服务器已打开<br><br>");
    port = 61314;//端口
    serverSocket = new ServerSocket(port);//服务器tcp
    /*服务器tcp与界面信号槽*/
    connect(serverSocket,SIGNAL(toUiUpdate(QString)),this,SLOT(serverUpdate(QString)));
    connect(serverSocket,SIGNAL(toUiNumber(uint)),this,SLOT(serverNumber(uint)));
}

void PureChatServer::on_pushButton_2_clicked()
{
    if(serverSocket == nullptr) return ;
    delete serverSocket;
    serverSocket = nullptr;
    ui->textBrowser->append("<br><br>服务器已关闭");
}

void PureChatServer::serverUpdate(QString msg)
{
    ui->textBrowser->append(msg);
}

void PureChatServer::serverNumber(uint num)
{
    ui->label_9->setText(QString("在线人数:")+QString::number(num));
}

