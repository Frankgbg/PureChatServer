#include "databaseoperator.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

const QByteArray successful = "Login successful.";
const QByteArray failed = "Login failed!";
const QByteArray notExist = "Account/Group does not exists!";
const QByteArray logged = "Account already logged in!";
const QByteArray databaseFailed = "Database operation failed!";
const QByteArray friendExist = "Friend exists!";
const QByteArray groupExist = "Group exists!";

DatabaseOperator::DatabaseOperator(const QSqlDatabase &_db) :
    db(_db)
{
    open();
}

DatabaseOperator::~DatabaseOperator()
{
    close();
}

void DatabaseOperator::open() {
    if(isOpen()) return ;
    if(db.open()) qDebug()<<"Database opened successfully.";
    else qDebug()<<"Database opened failed!";
}

void DatabaseOperator::close()
{
    if(!isOpen()) return ;
    db.close();
    if(!isOpen()) qDebug()<<"Database closed successfully.";
    else qDebug()<<"Database closed failed!";
}

bool DatabaseOperator::signUp(QString ownName, QString ownPassword, qulonglong &returnId)
{
    returnId = 0;
    qulonglong id = maxUserId()+1;
    QString ownId = QString::number(id);
    bool isSuccessful;
    isSuccessful = insertAccounts(ownId,ownName,ownPassword);
    if(!isSuccessful) return false;
    QString information;
    isSuccessful = insertFriends(ownId,ownId,information);
    if(!isSuccessful){
        bool del = false;
        while(!del) del = deleteAccounts(ownId);
        return false;
    }
    returnId = id;
    return true;
}

bool DatabaseOperator::createGroup(qulonglong ownId, QString groupName, qulonglong &returnId)
{
    returnId = 0;
    qulonglong id = maxGroupId()+1;
    QString groupId = QString::number(id);
    bool isSuccessful;
    isSuccessful = insertGroups(groupId,groupName);
    if(!isSuccessful) return false;
    QString ownId2 = QString::number(ownId);
    QString information;
    isSuccessful = insertCircle(ownId2,groupId,information);
    if(!isSuccessful){
        bool del = false;
        while(!del) del = deleteGroups(groupId);
        return false;
    }
    returnId = id;
    return true;
}

bool DatabaseOperator::loginVerify(qulonglong ownId, QString ownPassword, QString ownIp,
                                   QByteArray &information, QString &returnName)
{
    QSqlQuery *query = searchAccount(ownId);
    if(query == nullptr) {
        information = databaseFailed;
        return false;
    }
    if(query->next()){
        if(query->value(0).toString() != ownPassword){
            information = failed;
        }
        else if(query->value(1).toBool()){
            information = logged;
        }
        else{
            information = successful;
            returnName = query->value(2).toString();
            updateLoginStatus(ownId,1,ownIp);
            delete query;
            return true;
        }
    }
    else{
        information = notExist;
    }
    delete query;
    return false;
}

void DatabaseOperator::logout(qulonglong ownId)
{
    updateLoginStatus(ownId,0,"");
}

bool DatabaseOperator::modifyOwnPassword(qulonglong ownId, QString oldPassword, QString newPassword)
{
    QSqlQuery *query = searchAccount(ownId);
    if(query->next()){
        qDebug()<<query->value(0).toString()<<query->value(1).toString();
        if(query->value(0).toString() == oldPassword){
            bool isSuccessful = updateOwnPassword(ownId,newPassword);
            if(!isSuccessful) return delete query,query = nullptr,false;
            return delete query,query = nullptr,true;
        }
    }
    return delete query,query = nullptr,false;
}

bool DatabaseOperator::modifyOwnName(qulonglong ownId, QString newName)
{
    bool isSuccessful = updateOwnName(ownId,newName);
    if(isSuccessful) return true;
    return false;
}

bool DatabaseOperator::modifyOwnSign(qulonglong ownId, QString newSign)
{
    bool isSuccessful = updateOwnSign(ownId,newSign);
    if(isSuccessful) return true;
    return false;
}

bool DatabaseOperator::modifyGroupName(qulonglong groupId, QString newName)
{
    bool isSuccessful = updateGroupName(groupId,newName);
    if(isSuccessful) return true;
    return false;
}

bool DatabaseOperator::addFriend(qulonglong ownId, qulonglong friendId, QString &information)
{
    QString ownId2 = QString::number(ownId);
    QString friendId2 = QString::number(friendId);
    bool isSuccessful;
    isSuccessful = insertFriends(ownId2,friendId2,information);
    if(!isSuccessful) return false;
    isSuccessful = insertFriends(friendId2,ownId2,information);
    if(!isSuccessful){
        bool del = false;
        while(!del) del = deleteFriends(ownId2,friendId2);
        return false;
    }
    return true;
}

bool DatabaseOperator::delFriend(qulonglong ownId, qulonglong friendId)
{
    QString ownId2 = QString::number(ownId);
    QString friendId2 = QString::number(friendId);
    bool isSuccessful;
    isSuccessful = deleteFriends(ownId2,friendId2);
    if(!isSuccessful) return false;
    isSuccessful = deleteFriends(friendId2,ownId2);
    if(!isSuccessful){
        bool del = false;
        QString information;
        while(!del) del = insertFriends(ownId2,friendId2,information);
        return false;
    }
    return true;
}

bool DatabaseOperator::addGroup(qulonglong ownId, qulonglong groupId, QString &information)
{
    QString ownId2 = QString::number(ownId);
    QString groupId2 = QString::number(groupId);
    bool isSuccessful = insertCircle(ownId2,groupId2,information);
    if(!isSuccessful) return false;
    return true;
}

bool DatabaseOperator::delGroup(qulonglong ownId, qulonglong groupId)
{
    QString ownId2 = QString::number(ownId);
    QString groupId2 = QString::number(groupId);
    bool isSuccessful = deleteCircle(ownId2,groupId2);
    if(!isSuccessful) return false;
    return true;
}

QSqlQuery *DatabaseOperator::searchFriend(qulonglong friendId)
{
    QString id = QString::number(friendId);
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    query->setForwardOnly(true);
    bool isSuccessful = query->exec(QString("SELECT userId,userName,userSign,userLoginStatus,userIp "
                                            "FROM Accounts "
                                            "WHERE userId='%1'").arg(id));
    if(!isSuccessful) delete query,query = nullptr;
    return query;
}

QSqlQuery *DatabaseOperator::searchFriends(qulonglong ownId)
{
    QString id = QString::number(ownId);
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    query->setForwardOnly(true);
    bool isSuccessful = query->exec(QString("SELECT friendId,userName,userSign "
                                            "FROM Friends,Accounts "
                                            "WHERE Friends.userId='%1' and "
                                            "Friends.friendId=Accounts.userId").arg(id));
    if(!isSuccessful) delete query,query = nullptr;
    return query;
}

QSqlQuery *DatabaseOperator::searchGroup(qulonglong groupId)
{
    QString id = QString::number(groupId);
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    query->setForwardOnly(true);
    bool isSuccessful = query->exec(QString("SELECT groupId,groupName "
                                            "FROM Groups "
                                            "WHERE groupId='%1'").arg(id));
    if(!isSuccessful) delete query,query = nullptr;
    return query;
}

QSqlQuery *DatabaseOperator::searchGroups(qulonglong ownId)
{
    QString id = QString::number(ownId);
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    query->setForwardOnly(true);
    bool isSuccessful = query->exec(QString("SELECT Groups.groupId,groupName "
                                            "FROM Circle_of_friends,Groups "
                                            "WHERE userId='%1' and "
                                            "Circle_of_friends.groupId=Groups.groupId").arg(id));
    if(!isSuccessful) delete query,query = nullptr;
    return query;
}

QSqlQuery *DatabaseOperator::searchFriendsOfGroup(qulonglong groupId)
{
    QString id = QString::number(groupId);
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    query->setForwardOnly(true);
    bool isSuccessful = query->exec(QString("SELECT Accounts.userId,userName,userLoginStatus,userIp "
                                            "FROM Circle_of_friends,Accounts "
                                            "WHERE groupId='%1' and "
                                            "Circle_of_friends.userId=Accounts.userId").arg(id));
    if(!isSuccessful) delete query,query = nullptr;
    return query;
}

QSqlQuery *DatabaseOperator::searchAccount(qulonglong ownId)
{
    QString id = QString::number(ownId);
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    query->setForwardOnly(true);
    bool isSuccessful = query->exec(QString("SELECT userPassword,userLoginStatus,userName "
                                            "FROM Accounts WHERE userId='%1'").arg(id));
    if(!isSuccessful) delete query,query = nullptr;
    return query;
}

void DatabaseOperator::updateLoginStatus(qulonglong ownId, bool ownLoginStatus, const QString &ownIp)
{
    QString id = QString::number(ownId);
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    query->setForwardOnly(true);
    query->exec(QString("UPDATE Accounts "
                        "SET userLoginStatus='%2',userIp='%3' "
                        "WHERE userId='%1'").arg(id).arg(ownLoginStatus).arg(ownIp));
}

bool DatabaseOperator::updateOwnPassword(qulonglong ownId, const QString &newPassword)
{
    QString id = QString::number(ownId);
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    query->setForwardOnly(true);
    bool isSuccessful = query->exec(QString("UPDATE Accounts "
                                            "SET userPassword='%2' "
                                            "WHERE userId='%1'").arg(id).arg(newPassword));
    if(!isSuccessful) return false;
    return true;
}

bool DatabaseOperator::updateOwnName(qulonglong ownId, const QString &newName)
{
    QString id = QString::number(ownId);
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    query->setForwardOnly(true);
    bool isSuccessful = query->exec(QString("UPDATE Accounts "
                                            "SET userName='%2' "
                                            "WHERE userId='%1'").arg(id).arg(newName));
    if(!isSuccessful) return false;
    return true;
}

bool DatabaseOperator::updateOwnSign(qulonglong ownId, const QString &newSign)
{
    QString id = QString::number(ownId);
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    query->setForwardOnly(true);
    bool isSuccessful = query->exec(QString("UPDATE Accounts "
                                            "SET userSign='%2' "
                                            "WHERE userId='%1'").arg(id).arg(newSign));
    delete query;
    if(!isSuccessful) return false;
    return true;
}

bool DatabaseOperator::updateGroupName(qulonglong groupId, const QString &newName)
{
    QString id = QString::number(groupId);
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    query->setForwardOnly(true);
    bool isSuccessful = query->exec(QString("UPDATE Groups "
                                            "SET groupName='%2' "
                                            "WHERE groupId='%1'").arg(id).arg(newName));
    delete query;
    if(!isSuccessful) return false;
    return true;
}

bool DatabaseOperator::insertAccounts(const QString &ownId, const QString &ownPassword, const QString &ownName)
{
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    bool isSuccessful = query->exec(QString("INSERT INTO Accounts (userId,userPassword,userName,userLoginStatus) "
                                            "VALUES ('%1','%2','%3',0)").arg(ownId).arg(ownPassword).arg(ownName));
    delete query;
    query = nullptr;
    if(!isSuccessful) return false;
    return true;
}

bool DatabaseOperator::deleteAccounts(const QString &ownId)
{
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    bool isSuccessful = query->exec(QString("DELETE FROM Accounts "
                                            "WHERE userId='%1'").arg(ownId));
    delete query;
    query = nullptr;
    if(!isSuccessful) return false;
    return true;
}

bool DatabaseOperator::insertFriends(const QString &ownId, const QString &friendId, QString &information)
{
    if(queryFriends(ownId,friendId,information)) return false;
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    bool isSuccessful = query->exec(QString("INSERT INTO Friends (userId,friendId) "
                                            "VALUES ('%1','%2')").arg(ownId).arg(friendId));
    delete query;
    query = nullptr;
    if(!isSuccessful) return false;
    return true;
}

bool DatabaseOperator::queryFriends(const QString &ownId, const QString &friendId, QString &information)
{
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    bool isSuccessful = query->exec(QString("SELECT friendId FROM Friends "
                                            "WHERE userId='%1' and friendId='%2'").arg(ownId).arg(friendId));
    if(!isSuccessful){
        information = databaseFailed;
        delete query;query = nullptr;
        return true;
    }
    if(query->record().isEmpty() || !query->next()) return delete query,query = nullptr,false;

    information = friendExist;
    return delete query,query = nullptr,true;
}

bool DatabaseOperator::deleteFriends(const QString &ownId, const QString &friendId)
{
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    bool isSuccessful = query->exec(QString("DELETE FROM Friends "
                                            "WHERE userId='%1' and friendId='%2'").arg(ownId).arg(friendId));
    delete query;
    query = nullptr;
    if(!isSuccessful) return false;
    return true;
}

bool DatabaseOperator::insertGroups(const QString &groupId, const QString &groupName)
{
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    bool isSuccessful = query->exec(QString("INSERT INTO Groups (groupId,groupName,membersNumber) "
                                            "VALUES ('%1','%2',1)").arg(groupId).arg(groupName));
    delete query;
    query = nullptr;
    if(!isSuccessful) return false;
    return true;
}

bool DatabaseOperator::deleteGroups(const QString &groupId)
{
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    bool isSuccessful = query->exec(QString("DELETE FROM Groups "
                                            "WHERE groupId='%1'").arg(groupId));
    delete query;
    query = nullptr;
    if(!isSuccessful) return false;
    return true;
}

bool DatabaseOperator::insertCircle(const QString &ownId, const QString &groupId, QString &information)
{
    if(queryCircle(ownId,groupId,information)) return false;
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    bool isSuccessful = query->exec(QString("INSERT INTO Circle_of_friends (userId,groupId) "
                                            "VALUES ('%1','%2')").arg(ownId).arg(groupId));
    delete query;
    query = nullptr;
    if(!isSuccessful) return false;
    return true;
}

bool DatabaseOperator::queryCircle(const QString &ownId, const QString &groupId, QString &information)
{  
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    bool isSuccessful = query->exec(QString("SELECT groupId FROM Circle_of_friends "
                                            "WHERE userId='%1' and groupId='%2'").arg(ownId).arg(groupId));
    if(!isSuccessful){
        information = databaseFailed;
        delete query;query = nullptr;
        return true;
    }
    if(query->record().isEmpty() || !query->next()) return delete query,query = nullptr,false;

    information = groupExist;
    return delete query,query = nullptr,true;
}

bool DatabaseOperator::deleteCircle(const QString &ownId, const QString &groupId)
{
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    bool isSuccessful = query->exec(QString("DELETE FROM Circle_of_friends "
                                            "WHERE userId='%1' and groupId='%2'").arg(ownId).arg(groupId));
    delete query;
    query = nullptr;
    if(!isSuccessful) return false;
    return true;
}

qulonglong DatabaseOperator::maxUserId()
{
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    bool isSuccessful = query->exec(QString("SELECT max(userId) "
                                            "FROM Accounts"));
    if(!isSuccessful) return delete query,query = nullptr,9999999999;
    qulonglong id = 99999;
    if(query->next()) id = query->value(0).toULongLong();
    if(id == 0) id = 99999;
    return delete query,query = nullptr,id;
}

qulonglong DatabaseOperator::maxGroupId()
{
    if(!isOpen()) open();
    QSqlQuery *query = new QSqlQuery(db);
    bool isSuccessful = query->exec(QString("SELECT max(groupId) "
                                            "FROM Groups"));
    if(!isSuccessful) return delete query,query = nullptr,999999999;
    qulonglong id = 999;
    if(query->next()) id = query->value(0).toULongLong();
    if(id == 0) id = 999;
    return delete query,query = nullptr,id;
}
