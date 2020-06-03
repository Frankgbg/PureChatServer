#ifndef PURECHATSERVER_H
#define PURECHATSERVER_H

#include "serversocket.h"

#include <QWidget>

namespace Ui {
class PureChatServer;
}

class PureChatServer : public QWidget
{
    Q_OBJECT
public:
    explicit PureChatServer(QWidget *parent = nullptr);
    ~PureChatServer();

public slots:
    void on_pushButton_clicked();//打开服务器槽
    void on_pushButton_2_clicked();//关闭服务器槽
    void serverUpdate(QString msg);//服务器tcp数据槽
    void serverNumber(uint num);//在线人数槽

private:
    Ui::PureChatServer *ui;
    quint16 port;//监听端口
    ServerSocket *serverSocket;//服务器tcp
};

#endif // PURECHATSERVER_H
