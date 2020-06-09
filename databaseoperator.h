#ifndef DATABASEOPERATOR_H
#define DATABASEOPERATOR_H

#include <QSqlDatabase>
#include <QString>
#include <QDebug>

class DatabaseOperator
{
public:
    explicit DatabaseOperator(const QSqlDatabase &_db);
    ~DatabaseOperator();

public:
    inline bool isOpen() const { return db.isOpen();}//数据库是否打开
    inline void open();     //打开数据库
    inline void close();    //关闭数据库

    bool signUp(QString ownName, QString ownPassword, qulonglong &returnId);//注册账号
    bool createGroup(qulonglong ownId, QString groupName, qulonglong &returnId);//创建群
    bool loginVerify(qulonglong ownId, QString ownPassword, QString ownIp,
                     QByteArray &information, QString &ownName);//登录验证
    void logout(qulonglong ownId);//退出登录
    bool modifyOwnPassword(qulonglong ownId, QString oldPassword, QString newPassword);//修改个人密码
    bool modifyOwnName(qulonglong ownId, QString newName);//修改个人昵称
    bool modifyOwnSign(qulonglong ownId, QString newSign);//修改个人签名
    bool modifyGroupName(qulonglong groupId, QString newName);//修改群昵称
    bool addFriend(qulonglong ownId, qulonglong friendId, QString &information);//加好友
    bool delFriend(qulonglong ownId, qulonglong friendId);//删除好友
    bool addGroup(qulonglong ownId, qulonglong groupId, QString &information);//加群
    bool delGroup(qulonglong ownId, qulonglong groupId);//删除群
    QSqlQuery *searchFriend(qulonglong friendId);//查找好友
    QSqlQuery *searchFriends(qulonglong ownId);//查找好友列表
    QSqlQuery *searchGroup(qulonglong groupId);//查找群
    QSqlQuery *searchGroups(qulonglong ownId);//查找群列表
    QSqlQuery *searchFriendsOfGroup(qulonglong groupId);//查找群内的好友列表

private:
    QSqlQuery *searchAccount(qulonglong ownId);//查找账号对应密码及登录状态
    void updateLoginStatus(qulonglong ownId, bool ownLoginStatus, const QString &ownIp);//修改登录状态
    bool updateOwnPassword(qulonglong ownId, const QString &newPassword);//修改个人密码
    bool updateOwnName(qulonglong ownId, const QString &newName);//修改个人昵称
    bool updateOwnSign(qulonglong ownId, const QString &newSign);//修改个人签名
    bool updateGroupName(qulonglong groupId, const QString &newName);//修改群昵称
    bool insertAccounts(const QString &ownId, const QString &ownPassword, const QString &ownName);//插入账号信息
    bool deleteAccounts(const QString &ownId);//删除账号
    bool insertFriends(const QString &ownId, const QString &friendId, QString &information);//插入好友关系
    bool queryFriends(const QString &ownId, const QString &friendId, QString &information);//查询好友关系
    bool deleteFriends(const QString &ownId, const QString &friendId);//删除好友关系
    bool insertGroups(const QString &groupId, const QString &groupName);//插入群信息
    bool deleteGroups(const QString &groupId);//删除群
    bool insertCircle(const QString &ownId, const QString &groupId, QString &information);//插入群关系
    bool queryCircle(const QString &ownId, const QString &groupId, QString &information);//查询群关系
    bool deleteCircle(const QString &ownId, const QString &groupId);//删除群关系
    qulonglong maxUserId();//已注册最大账号值
    qulonglong maxGroupId();//已注册最大群号值

private:
    QSqlDatabase db;//QSqlDatabase对象
};

#endif // DATABASEOPERATOR_H
