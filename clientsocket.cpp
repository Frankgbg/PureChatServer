#include "clientsocket.h"
//#define Q_QDOC
#include <QJsonObject>
#include <QJsonParseError>
#include <QHostAddress>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QFile>
#include <QTextCodec>

const QByteArray successful = "Login successful.";
const QByteArray failed = "Login failed!";
const QByteArray notExist = "Account/Group does not exists!";
const QByteArray logged = "Account already logged in!";
const QByteArray databaseFailed = "Database operation failed!";
const QByteArray friendExist = "Friend exists!";
const QByteArray groupExist = "Group exists!";
const QByteArray exist = "Account/Group exists.";

const QByteArray unknownError = "Unknown error!";
const QByteArray numberError = "Account/Group number error!";
const QByteArray requestSuccessful = "Request successful!";
const QByteArray requestFailed = "Request failed!";
const QByteArray signUpFailed = "Sign up failed!";
const QByteArray createFailed = "Create group failed!";
const QByteArray complete = "Data transmission completed";

const QByteArray signUpType = "90000";//注册
const QByteArray loginType = "90001";//登录
const QByteArray modifyOwnPasswordType = "90002";//修改个人密码
const QByteArray modifyOwnNameType = "90003";//修改个人昵称
const QByteArray modifyOwnSignType = "90004";//修改个人签名
const QByteArray modifyGroupNameType = "90005";//修改群昵称
const QByteArray searchFriendType = "90006";//查找好友
const QByteArray addFriendType = "90007";//添加好友
const QByteArray delFriendType = "90008";//删除好友
const QByteArray createGroupType = "90009";//创建群聊
const QByteArray searchGroupType = "90010";//查找群
const QByteArray addGroupType = "90011";//添加群
const QByteArray delGroupType = "90012";//删除群
const QByteArray searchFriendsType = "90013";//查找好友列表
const QByteArray searchGroupsType = "90014";//查找群列表
const QByteArray searchFriendsOfGroupType = "90015";//查找群内好友列表
const QByteArray chatFriendType = "90016";//好友聊天
const QByteArray chatGroupType = "90017";//群内聊天

const QByteArray unknownType = "99998";//未知类型,数据错误
const QByteArray endType = "99999";//"包"结束


ClientSocket::ClientSocket(const QSqlDatabase &db, QTcpSocket *parent) :
    QTcpSocket(parent)
{
    ownId = 0;
    dbo = new DatabaseOperator(db);

    connect(this,SIGNAL(readyRead()),this,SLOT(receiveData()));
    connect(this,SIGNAL(disconnected()),this,SLOT(disconnection()));
}

ClientSocket::~ClientSocket()
{
    delete dbo;
}

QByteArray lastRemain;
QByteArray type;
ushort where = 0;
uint length;

void ClientSocket::receiveData()
{
    QByteArray byteAll = readAll();
    byteAll = lastRemain+byteAll;
    while(true){
        QByteArray data;
        switch(where){
        case 0:{
            data = readDataLength(5,byteAll);
            if(data.size() < 5){
                lastRemain = data;
                where = 0;
                return ;
            }
            length = byteArrayToUint(data);
            if(length == 99999u) {
                where = 0;
                continue ;
            }
            else if(length >= 90000u && length < 99999u) {
                type = data;
            }
            else {
                sendTips(unknownType,requestFailed);
                emit toServerUpdata(unknownType+"1");
                /*考虑缓冲区剩余数据如何操作*/
                continue ;
            }
        }
        case 1:{
            data = readDataLength(5,byteAll);
            if(data.size() < 5){
                lastRemain = data;
                where = 1;
                return ;
            }
            length = byteArrayToUint(data);

            if(length == 99999u) {
                where = 0;
                continue ;
            }
            else if(length == 0u || length >= 90000u) {
                sendTips(unknownType,requestFailed);
                emit toServerUpdata(unknownType+"2");
                /*考虑缓冲区剩余数据如何操作*/
                continue ;
            }
        }
        case 2:{
            data = readDataLength(length,byteAll);
            if(data.size() < (int)length){
                lastRemain = data;
                where = 2;
                return ;
            }
            QJsonObject json = QJsonDocument::fromJson(data).object();

            if(type == signUpType) signUp(json);//注册账号
            else if(type == loginType) loginVerify(json);//登录请求
            else if(type == modifyOwnPasswordType) modifyOwnPassword(json);//修改个人密码
            else if(type == modifyOwnNameType) modifyOwnName(json);//修改个人昵称
            else if(type == modifyOwnSignType) modifyOwnSign(json);//修改个人签名
            else if(type == modifyGroupNameType) modifyGroupName(json);//修改群昵称
            else if(type == searchFriendType) searchFriend(json);//查找好友
            else if(type == addFriendType) addFriend(json);//添加好友
            else if(type == delFriendType) delFriend(json);//删除好友
            else if(type == createGroupType) createGroup(json);//创建群聊
            else if(type == searchGroupType) searchGroup(json);//查找群
            else if(type == addGroupType) addGroup(json);//添加群
            else if(type == delGroupType) delGroup(json);//删除群
            else if(type == searchFriendsType) searchFriends(ownId);//查找好友列表
            else if(type == searchGroupsType) searchGroups(ownId);//查找群列表
            else if(type == searchFriendsOfGroupType) searchFriendsOfGroup(json);//查找群内好友列表
            else if(type == chatFriendType) chatFriend(json);//好友聊天
            else if(type == chatGroupType) chatGroup(json);//群内聊天
            else {
                sendTips(unknownType,requestFailed);
                emit toServerUpdata(unknownType+"3");
                /*考虑缓冲区剩余数据如何操作*/
            }
            break;
        }
        default:break;
        }
    }
}

void ClientSocket::disconnection()
{
    if(ownId != 0) dbo->logout(ownId);
    emit toServerDisconnection(this->socketDescriptor());
}

qulonglong ClientSocket::getId()
{
    return ownId;
}

void ClientSocket::send(const QByteArray &type, const QByteArray &data)
{
    sendType(type);
    sendData(data);
    sendType(endType);
}

QByteArray ClientSocket::readDataLength(uint dataLength, QByteArray &data)
{
    QByteArray rec;
    if(data.size() <= (int)dataLength){
        rec = data;
        data.clear();
    }
    else{
        for(uint i = 0;i < dataLength;++i) rec.push_back(data[i]);
        for(uint i = dataLength;i < (uint)data.size();++i) data[i-dataLength] = data[i];
        data.resize((uint)data.size()-dataLength);
    }
    return rec;
}

void ClientSocket::sendType(const QByteArray &type)
{
    write(type,5);
}

void ClientSocket::sendData(const QByteArray &data)
{
    QString len = QString::number(data.size());
    len = QString(5-len.size(),'0')+len;
    write(len.toLocal8Bit(),5);
    write(data);
}

void ClientSocket::sendData(const QByteArray &type, const QJsonObject &json)
{
    QByteArray data = QJsonDocument(json).toJson();
    send(type,data);
}

void ClientSocket::sendTips(const QByteArray &type, const QByteArray &data)
{
    QJsonObject json;
    json.insert("information",QString::fromLatin1(data));
    QByteArray data1 = QJsonDocument(json).toJson();
    send(type,data1);
}

void ClientSocket::signUp(const QJsonObject &json)
{
    QString ownName = json.value("ownName").toString();
    QString ownPassword = json.value("ownPassword").toString();
    qulonglong ownId;
    bool isSuccessful = dbo->signUp(ownName,ownPassword,ownId);
    if(!isSuccessful){//注册失败
        sendTips(signUpType,requestFailed);
        emit toServerUpdata("类型:"+signUpType+"  "+requestFailed);
        return ;
    }
    QJsonObject json1;
    json1.insert("ownId",QString::number(ownId));
    sendData(signUpType,json1);//发送注册到的账号
    emit toServerUpdata("类型:"+signUpType+"  "+complete);
}

void ClientSocket::loginVerify(const QJsonObject &json)
{
    qulonglong id = stringToQulonglong(json.value("ownId").toString());
    QString password = json.value("ownPassword").toString();
    if(id == 0){
        sendTips(loginType,numberError);
        emit toServerUpdata("类型:"+loginType+"  "+numberError);
        close();
        return ;
    }
    QByteArray information;
    QString name;
    bool isSuccess = dbo->loginVerify(id,password,peerAddress().toString(),information,name);
    sendTips(loginType,information);//发送尝试登录返回信息
    emit toServerUpdata("类型:"+loginType+"  "+information);
    if(!isSuccess) {
        ownId = 0;
        ownName = "";
        close();//登录失败,关闭连接
        return ;
    }
    ownId = id;//保存tcp连接客户账号
    ownName = name;
    searchFriends(ownId);//发送好友列表信息
    searchGroups(ownId);//发送群列表信息

    QString fileName = "./msg/"+QString::number(id)+".txt";
    QFile file(fileName);
    if(file.exists()){
        file.open(QIODevice::ReadWrite);
        while(true){
            QByteArray type = file.readLine();
            if(type == "") break;
            uint length = file.readLine().toUInt();
            QByteArray data = file.read(length);
            QString str = byteArrayToUnicode(data);
            send(type,str.toUtf8());
        }
        emit toServerUpdata("离线消息已发送.");
        file.close();
        if(!QFile::remove(fileName)){
            file.open(QIODevice::WriteOnly);
            file.close();
        }
    }
}

void ClientSocket::modifyOwnPassword(const QJsonObject &json)
{
    QString oldPassword = json.value("oldPassword").toString();
    QString newPassword = json.value("newPassword").toString();
    bool isSuccessful = dbo->modifyOwnPassword(ownId,oldPassword,newPassword);
    if(!isSuccessful){
        sendTips(modifyOwnPasswordType,requestFailed);
        emit toServerUpdata("类型:"+modifyOwnPasswordType+"  "+requestFailed);
        return ;
    }
    sendTips(modifyOwnPasswordType,requestSuccessful);
    emit toServerUpdata("类型:"+modifyOwnPasswordType+"  "+requestSuccessful);
}

void ClientSocket::modifyOwnName(const QJsonObject &json)
{
    QString newName = json.value("newName").toString();
    bool isSuccessful = dbo->modifyOwnName(ownId,newName);
    if(!isSuccessful){
        sendTips(modifyOwnNameType,requestFailed);
        emit toServerUpdata("类型:"+modifyOwnNameType+"  "+requestFailed);
        return ;
    }
    ownName = newName;
    sendTips(modifyOwnNameType,requestSuccessful);
    emit toServerUpdata("类型:"+modifyOwnNameType+"  "+requestSuccessful);
}

void ClientSocket::modifyOwnSign(const QJsonObject &json)
{
    QString newSign = json.value("newSign").toString();
    bool isSuccessful = dbo->modifyOwnSign(ownId,newSign);
    if(!isSuccessful){
        sendTips(modifyOwnSignType,requestFailed);
        emit toServerUpdata("类型:"+modifyOwnSignType+"  "+requestFailed);
        return ;
    }
    sendTips(modifyOwnSignType,requestSuccessful);
    emit toServerUpdata("类型:"+modifyOwnSignType+"  "+requestSuccessful);
}

void ClientSocket::modifyGroupName(const QJsonObject &json)
{
    QString newName = json.value("newName").toString();
    qulonglong groupId = stringToQulonglong(json.value("groupId").toString());
    if(groupId == 0){
        sendTips(modifyGroupNameType,numberError);
        emit toServerUpdata("类型:"+modifyGroupNameType+"  "+numberError);
        return ;
    }
    bool isSuccessful = dbo->modifyGroupName(groupId,newName);
    if(!isSuccessful){
        sendTips(modifyGroupNameType,requestFailed);
        emit toServerUpdata("类型:"+modifyGroupNameType+"  "+requestFailed);
        return ;
    }
    emit toServerUpdata("类型:"+modifyGroupNameType+"  "+requestSuccessful);
    QSqlQuery *query = dbo->searchFriendsOfGroup(groupId);
    if(query == nullptr){
        sendTips(modifyGroupNameType,unknownError);
        emit toServerUpdata("类型:"+modifyGroupNameType+"  "+unknownError);
        return ;
    }
    QJsonObject json2;
    json2.insert("groupId",QString::number(groupId));
    json2.insert("groupName",newName);
    QByteArray data = QJsonDocument(json2).toJson();
    while(query->next()){
        if(query->value(2).toBool()){//好友在线,在线消息
            emit toServerData(query->value(0).toULongLong(),modifyGroupNameType,data);
        }
    }
    emit toServerUpdata("类型:"+modifyGroupNameType+"  "+complete);
    delete query;
}

void ClientSocket::searchFriend(const QJsonObject &json)
{
    qulonglong friendId = stringToQulonglong(json.value("friendId").toString());
    if(friendId == 0){
        sendTips(searchFriendType,numberError);
        emit toServerUpdata("类型:"+searchFriendType+"  "+numberError);
        return ;
    }
    QSqlQuery *query = dbo->searchFriend(friendId);
    if(query == nullptr){
        sendTips(searchFriendType,unknownError);
        emit toServerUpdata("类型:"+searchFriendType+"  "+unknownError);
        return ;
    }
    if(!query->record().isEmpty()){
        if(query->next()){//发送好友信息
            sendFriend(searchFriendType,query->value(0).toULongLong(),query->value(1).toString(),query->value(2).toString());
            emit toServerUpdata("类型:"+searchFriendType+"  "+complete);
        }
        else{
            sendTips(searchFriendType,notExist);
            emit toServerUpdata("类型:"+searchFriendType+"  "+notExist);
        }
    }
    else{//账号不存在
        sendTips(searchFriendType,notExist);
        emit toServerUpdata("类型:"+searchFriendType+"  "+notExist);
    }
    delete query;
}

void ClientSocket::addFriend(const QJsonObject &json)
{
    qulonglong requestId = stringToQulonglong(json.value("requestId").toString());
    qulonglong friendId = stringToQulonglong(json.value("friendId").toString());
    if(requestId == 0 || friendId == 0){
        sendTips(addFriendType,numberError);
        emit toServerUpdata("类型:"+addFriendType+"  "+numberError);
        return ;
    }
    if(requestId != ownId){//离线,由接收方同意加好友
        QSqlQuery *query = dbo->searchFriend(requestId);
        if(query == nullptr){
            sendTips(addFriendType,unknownError);
            emit toServerUpdata("类型:"+addFriendType+"  "+unknownError);
            return ;
        }
        if(query->record().isEmpty()){//申请加好友者账号注销,不存在
            sendTips(addFriendType,notExist);
            emit toServerUpdata("类型:"+addFriendType+"  "+notExist);
        }
        else{
            /*
             * 离线加好友
             */
        }
        delete query;
        return ;
    }
    //由申请者直接加好友,无需对方同意
    QString information;
    bool isSuccessful = dbo->addFriend(requestId,friendId,information);
    if(!isSuccessful){//添加失败
        if(information == friendExist){//已是好友关系
            sendTips(addFriendType,friendExist);
            emit toServerUpdata("类型:"+addFriendType+"  "+friendExist);
            return ;
        }
        sendTips(addFriendType,unknownError);
        emit toServerUpdata("类型:"+addFriendType+"  "+unknownError);
        return ;
    }
    QSqlQuery *query = dbo->searchFriend(friendId);
    if(query->next()){
        //发送好友信息
        sendFriend(addFriendType,query->value(0).toULongLong(),query->value(1).toString(),query->value(2).toString());
        emit toServerUpdata("类型:"+addFriendType+"  "+complete);
        if(query->value(3).toBool()){//对方在线,及时给对方更新好友信息
            QSqlQuery *query2 = dbo->searchFriend(ownId);
            if(query2->next()){
                QJsonObject json2;
                json2.insert("friendId",QString::number(ownId));
                json2.insert("friendName",query2->value(1).toString());
                json2.insert("friendSign",query2->value(2).toString());
                QByteArray data = QJsonDocument(json2).toJson();
                emit toServerData(friendId,addFriendType,data);
            }
            delete query2;
        }
    }
    delete query;
}

void ClientSocket::delFriend(const QJsonObject &json)
{
    qulonglong friendId = stringToQulonglong(json.value("friendId").toString());
    if(friendId == 0){
        sendTips(delFriendType,numberError);
        emit toServerUpdata("类型:"+delFriendType+"  "+numberError);
        return ;
    }
    bool isSuccessful = dbo->delFriend(ownId,friendId);
    if(!isSuccessful){//删除失败
        sendTips(delFriendType,unknownError);
        emit toServerUpdata("类型:"+delFriendType+"  "+unknownError);
        return ;
    }
    QJsonObject json2;
    json2.insert("friendId",QString::number(friendId));
    sendData(delFriendType,json2);
    emit toServerUpdata("类型:"+delFriendType+"  "+complete);
    QSqlQuery *query = dbo->searchFriend(friendId);
    if(!query->record().isEmpty()){
        if(query->next()){
            if(query->value(3).toBool()){//对方在线,及时更新好友信息
                QJsonObject json3;
                json3.insert("friendId",QString::number(ownId));
                QByteArray data = QJsonDocument(json3).toJson();
                emit toServerData(friendId,delFriendType,data);
            }
        }
    }
    delete query;
}

void ClientSocket::createGroup(const QJsonObject &json)
{
    QString groupName = json.value("groupName").toString();
    qulonglong groupId;
    bool isSuccessful = dbo->createGroup(ownId,groupName,groupId);
    if(!isSuccessful){//创建失败
        sendTips(createGroupType,requestFailed);
        emit toServerUpdata("类型:"+createGroupType+"  "+requestFailed);
        return ;
    }
    QJsonObject json2;
    json2.insert("groupId",QString::number(groupId));
    json2.insert("groupName",groupName);
    sendData(createGroupType,json2);
    emit toServerUpdata("类型:"+createGroupType+"  "+complete);
}

void ClientSocket::searchGroup(const QJsonObject &json)
{
    qulonglong groupId = stringToQulonglong(json.value("groupId").toString());
    if(groupId == 0){
        sendTips(searchGroupType,numberError);
        emit toServerUpdata("类型:"+searchGroupType+"  "+numberError);
        return ;
    }
    QSqlQuery *query = dbo->searchGroup(groupId);
    if(query == nullptr){
        sendTips(searchGroupType,unknownError);
        emit toServerUpdata("类型:"+searchGroupType+"  "+unknownError);
        return ;
    }
    if(!query->record().isEmpty()){
        if(query->next()){//发送群信息
            sendGroup(searchGroupType,query->value(0).toULongLong(),query->value(1).toString());
            emit toServerUpdata("类型:"+searchGroupType+"  "+complete);
        }
        else{
            sendTips(searchGroupType,notExist);
            emit toServerUpdata("类型:"+searchGroupType+"  "+notExist);
        }
    }
    else{//群不存在
        sendTips(searchGroupType,notExist);
        emit toServerUpdata("类型:"+searchGroupType+"  "+notExist);
    }
    delete query;
}

void ClientSocket::addGroup(const QJsonObject &json)
{
    qulonglong requestId = stringToQulonglong(json.value("requestId").toString());
    qulonglong groupId = stringToQulonglong(json.value("groupId").toString());
    if(requestId == 0 || groupId == 0){
        sendTips(addGroupType,numberError);
        emit toServerUpdata("类型:"+addGroupType+"  "+numberError);
        return ;
    }
    if(requestId != ownId){//离线,由群内人员同意加群
        QSqlQuery *query = dbo->searchFriend(requestId);
        if(query == nullptr){
            sendTips(addGroupType,unknownError);
            emit toServerUpdata("类型:"+addGroupType+"  "+unknownError);
            return ;
        }
        if(query->record().isEmpty()){//申请加群者账号注销,不存在
            sendTips(addGroupType,notExist);
            emit toServerUpdata("类型:"+addGroupType+"  "+notExist);
        }
        else{
            /*
             * 离线加群
            */
        }
        delete query;
        return ;
    }
    //由申请者直接加群,无需群内人员同意
    QString information;
    bool isSuccessful = dbo->addGroup(requestId,groupId,information);
    if(!isSuccessful){//加群失败
        if(information == groupExist){//已在群中
            sendTips(addGroupType,groupExist);
            emit toServerUpdata("类型:"+addGroupType+"  "+groupExist);
            return ;
        }
        sendTips(addGroupType,unknownError);
        emit toServerUpdata("类型:"+addGroupType+"  "+unknownError);
        return ;
    }
    QSqlQuery *query = dbo->searchGroup(groupId);
    if(query->next()){
        //发送群信息
        sendGroup(addGroupType,query->value(0).toULongLong(),query->value(1).toString());
        emit toServerUpdata("类型:"+addGroupType+"  "+complete);
        /*
         * 未及时更新给其他群内人员.如果其他群内人员已打开聊天窗口,刚进来的好友可以接收消息,未显示此好友
         */
    }
    delete query;
}

void ClientSocket::delGroup(const QJsonObject &json)
{
    qulonglong groupId = stringToQulonglong(json.value("groupId").toString());
    if(groupId == 0){
        sendTips(delGroupType,numberError);
        emit toServerUpdata("类型:"+delGroupType+"  "+numberError);
        return ;
    }
    bool isSuccessful = dbo->delGroup(ownId,groupId);
    if(isSuccessful){//删除群成功
        QJsonObject json1;
        json1.insert("groupId",QString::number(groupId));
        sendData(delGroupType,json1);
        emit toServerUpdata("类型:"+delGroupType+"  "+complete);
        /*
         * 未及时更新给其他群内人员.如果其他群内人员已打开聊天窗口,刚退出的好友不可接收消息,但显示此好友
         */
    }
    else{//删除群失败
        sendTips(delGroupType,unknownError);
        emit toServerUpdata("类型:"+delGroupType+"  "+unknownError);
    }
}

void ClientSocket::searchFriends(qulonglong ownId)
{
    QSqlQuery *query = dbo->searchFriends(ownId);
    if(query == nullptr){
        sendTips(searchFriendsType,unknownError);
        emit toServerUpdata("类型:"+searchFriendsType+"  "+unknownError);
        return ;
    }
    while(query->next()){//至少有一个自己
        sendFriend(searchFriendsType,query->value(0).toULongLong(),query->value(1).toString(),query->value(2).toString());
    }
    emit toServerUpdata("类型:"+searchFriendsType+"  "+complete);
    delete query;
}

void ClientSocket::searchGroups(qulonglong ownId)
{
    QSqlQuery *query = dbo->searchGroups(ownId);
    if(query == nullptr){
        sendTips(searchGroupsType,unknownError);
        emit toServerUpdata("类型:"+searchGroupsType+"  "+unknownError);
        return ;
    }
    if(!query->record().isEmpty()){
        while(query->next()){
            sendGroup(searchGroupsType,query->value(0).toULongLong(),query->value(1).toString());
        }
        emit toServerUpdata("类型:"+searchGroupsType+"  "+complete);
    }
    delete query;
}

void ClientSocket::searchFriendsOfGroup(const QJsonObject &json)
{
    qulonglong groupId = stringToQulonglong(json.value("groupId").toString());
    if(groupId == 0){
        sendTips(searchFriendsOfGroupType,numberError);
        emit toServerUpdata("类型:"+searchFriendsOfGroupType+"  "+numberError);
        return ;
    }
    searchFriendsOfGroup(groupId);
}

void ClientSocket::searchFriendsOfGroup(qulonglong groupId)
{
    QSqlQuery *query = dbo->searchFriendsOfGroup(groupId);
    if(query == nullptr){
        sendTips(searchFriendsOfGroupType,unknownError);
        emit toServerUpdata("类型:"+searchFriendsOfGroupType+"  "+unknownError);
    }
    while(query->next()){//群内至少有一人
        QJsonObject json;
        json.insert("friendId",QString::number(query->value(0).toULongLong()));
        json.insert("friendName",query->value(1).toString());
        json.insert("friendSign",query->value(2).toString());
        json.insert("groupId",QString::number(groupId));
        sendData(searchFriendsOfGroupType,json);
    }
    emit toServerUpdata("类型:"+searchFriendsOfGroupType+"  "+complete);
    delete query;
}

void ClientSocket::chatFriend(const QJsonObject &json)
{
    qulonglong friendId = stringToQulonglong(json.value("friendId").toString());
    if(friendId == 0){
        sendTips(chatFriendType,numberError);
        emit toServerUpdata("类型:"+chatFriendType+"  "+numberError);
        return ;
    }
    QSqlQuery *query = dbo->searchFriend(friendId);
    if(query == nullptr){
        sendTips(chatFriendType,unknownError);
        emit toServerUpdata("类型:"+chatFriendType+"  "+unknownError);
        return ;
    }
    QJsonObject json2;
    json2.insert("msg",json.value("msg").toString());
    json2.insert("sendId",QString::number(ownId));
    json2.insert("sendName",ownName);
    QByteArray data = QJsonDocument(json2).toJson();
    if(query->next()){
        if(query->value(3).toBool()){//好友在线,在线消息
            emit toServerData(friendId,chatFriendType,data);
            emit toServerUpdata("类型:"+chatFriendType+"  对方在线,发送完成.");
        }
        else{
            QString fileName = "./msg/"+QString::number(friendId)+".txt";
            QFile file(fileName);
            file.open(QIODevice::Append);
            file.write(chatFriendType+'\n'+QString::number(data.size()).toLocal8Bit()+'\n');
            file.write(QString(data).toUtf8());
            file.close();
            emit toServerUpdata("类型:"+chatFriendType+"  对方不在线,已保存.");
        }
    }
    delete query;
}

void ClientSocket::chatGroup(const QJsonObject &json)
{
    qulonglong groupId = stringToQulonglong(json.value("groupId").toString());
    QString groupName = json.value("groupName").toString();
    if(groupId == 0){
        sendTips(chatGroupType,numberError);
        emit toServerUpdata("类型:"+chatGroupType+"  "+numberError);
        return ;
    }
    QSqlQuery *query = dbo->searchFriendsOfGroup(groupId);
    if(query == nullptr){
        sendTips(chatGroupType,unknownError);
        emit toServerUpdata("类型:"+chatGroupType+"  "+unknownError);
        return ;
    }
    QJsonObject json2;
    json2.insert("msg",json.value("msg").toString());
    json2.insert("sendId",QString::number(ownId));
    json2.insert("sendName",ownName);
    json2.insert("groupId",QString::number(groupId));
    json2.insert("groupName",groupName);
    QByteArray data = QJsonDocument(json2).toJson();
    while(query->next()){
        if(query->value(0).toULongLong() == ownId) continue;//不发给自己
        if(query->value(2).toBool()){//好友在线,在线消息
            emit toServerData(query->value(0).toULongLong(),chatGroupType,data);
            emit toServerUpdata("类型:"+chatFriendType+"  群成员:"+query->value(0).toString()+"在线,发送完成.");
        }
        else{
            QString fileName = "./msg/"+query->value(0).toString()+".txt";
            QFile file(fileName);
            file.open(QIODevice::Append);
            file.write(chatGroupType+'\n'+QString::number(data.size()).toLocal8Bit()+'\n');
            file.write(QString(data).toUtf8());
            file.close();
            emit toServerUpdata("类型:"+chatFriendType+"  群成员:"+query->value(0).toString()+"不在线,已保存.");
        }
    }
    delete query;
}

void ClientSocket::sendFriend(const QByteArray &type, const qulonglong &friendId, const QString &friendName, const QString &friendSign)
{
    QJsonObject json;
    json.insert("friendId",QString::number(friendId));
    json.insert("friendName",friendName);
    json.insert("friendSign",friendSign);
    sendData(type,json);
}

void ClientSocket::sendGroup(const QByteArray &type, const qulonglong &groupId, const QString &groupName)
{
    QJsonObject json;
    json.insert("groupId",QString::number(groupId));
    json.insert("groupName",groupName);
    sendData(type,json);
}

uint ClientSocket::byteArrayToUint(const QByteArray &byteArray)
{
    bool isSuccess;
    uint number = byteArray.toUInt(&isSuccess,10);//数据长度
    emit toServerUpdata(byteArray);
    emit toServerUpdata(QString::number(number,10));
    if(!isSuccess) {
        emit toServerUpdata("QByteArray转换uint失败");
        return 0;
    }
    else {
        emit toServerUpdata("QByteArray转换uint成功");
        return number;
    }
}

qulonglong ClientSocket::stringToQulonglong(const QString &string)
{
    bool isSuccess;
    qulonglong number = string.toULongLong(&isSuccess,10);
    emit toServerUpdata(string);
    emit toServerUpdata(QString::number(number,10));
    if(!isSuccess) {
        emit toServerUpdata("QString转换qulonglong失败");
        return 0;
    }
    else {
        emit toServerUpdata("QString转换qulonglong成功");
        return number;
    }
}

QString ClientSocket::byteArrayToUnicode(const QByteArray &array)
{
    QString text = QTextCodec::codecForName("UTF-8")->toUnicode(array);
    return text;
}


