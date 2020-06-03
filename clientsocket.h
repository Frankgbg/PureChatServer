#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include "databaseoperator.h"

#include <QTcpSocket>
#include <QHostAddress>

class ClientSocket : public QTcpSocket
{
    Q_OBJECT
public:
    ClientSocket(const QSqlDatabase &db, QTcpSocket *parent = nullptr);
    ~ClientSocket();

public slots:
    void receiveData();//接收数据槽
    void disconnection();//断开连接槽

public:
    qulonglong getId();
    void send(const QByteArray &type, const QByteArray &data);//发送"包"

private:
    QByteArray readDataLength(uint dataLength, QByteArray &data);//读dataLength字节数据
    void sendType(const QByteArray &type);//发送类型
    void sendData(const QByteArray &data);//发送数据

    void sendData(const QByteArray &type, const QJsonObject &json);//发送数据"包"
    void sendTips(const QByteArray &type, const QByteArray &data);//发送提示信息"包"

    void signUp(const QJsonObject &json);//注册账号
    void loginVerify(const QJsonObject &json);//登录验证

    void modifyOwnPassword(const QJsonObject &json);//修改个人密码
    void modifyOwnName(const QJsonObject &json);//修改个人昵称
    void modifyOwnSign(const QJsonObject &json);//修改个人签名
    void modifyGroupName(const QJsonObject &json);//修改群昵称

    void searchFriend(const QJsonObject &json);//查找好友
    void addFriend(const QJsonObject &json);//添加好友
    void delFriend(const QJsonObject &json);//删除好友

    void createGroup(const QJsonObject &json);//创建群
    void searchGroup(const QJsonObject &json);//查找群
    void addGroup(const QJsonObject &json);//添加群
    void delGroup(const QJsonObject &json);//删除群

    void searchFriends(qulonglong ownId);//查找好友列表
    void searchGroups(qulonglong ownId);//查找群列表
    void searchFriendsOfGroup(const QJsonObject &json);//查找群内好友列表
    void searchFriendsOfGroup(qulonglong groupId);//查找群内好友列表

    void chatFriend(const QJsonObject &json);//好友聊天
    void chatGroup(const QJsonObject &json);//群内聊天

    void sendFriend(const QByteArray &type, const qulonglong &friendId,
                    const QString &friendName, const QString &friendSign);//发送好友信息
    void sendGroup(const QByteArray &type, const qulonglong &groupId,
                   const QString &groupName);//发送群信息

    void sendOfflineMsg(qulonglong ownId); //发送离线消息

    uint byteArrayToUint(const QByteArray &byteArray);//QByteArray 转 uint,用于类型和长度判断
    qulonglong stringToQulonglong(const QString &string);//QString 转 qulonglong,用于Id判断
    QString byteArrayToUnicode(const QByteArray &array);

private:
    DatabaseOperator *dbo;//数据库操作对象
    qulonglong ownId;//用户账号
    QString ownName;//用户昵称

signals:
    void toServerData(qulonglong friendId,const QByteArray &type, const QByteArray &data);//发送给好友数据
    void toServerUpdata(QString msg);//传给服务器数据信号
    void toServerDisconnection(qintptr descriptor, qulonglong id);//传给服务器断开连接信号
    void toServerSuccessful(qulonglong id, qintptr handle);//登录成功信号
};

#endif // CLIENTSOCKET_H
