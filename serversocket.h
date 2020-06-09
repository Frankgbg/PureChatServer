#ifndef SERVER_H
#define SERVER_H

#include "clientsocket.h"

#include <QSqlDatabase>
#include <QTcpServer>
#include <QList>

class ServerSocket : public QTcpServer
{
    Q_OBJECT
public:
    ServerSocket(quint16 port, QTcpServer *parent = nullptr);
    ~ServerSocket();

    void incomingConnection(qintptr handle) override;//重载,当有新连接时触发

public slots:
    void clientData(qulonglong friendId, const QByteArray &data);//客户传输给好友信息槽
    void clientUpdate(QString msg);//客户tcp数据槽,用来显示在Ui界面
    void clientDisconnection(qintptr descriptor, qulonglong id);//客户tcp断开槽
    void signUpSuccessful(qulonglong id, qintptr handle);//登录成功

private:
    uint numberOfPeople;
    QHash<qintptr, ClientSocket*> *clientTemporary;//临时连接
    QHash<qulonglong, ClientSocket*> *clientSocket;//常连接
    QSqlDatabase *db;

signals:
    void toUiUpdate(QString msg);//传给Ui界面服务数据
    void toUiNumber(uint num);//传给Ui界面在线人数
};

#endif // SERVER_H
