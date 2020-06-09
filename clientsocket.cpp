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
#include <QPixmap>
#include <QBuffer>
#include <QTime>
#include <QCoreApplication>

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

const int signUpType = 9000000;//注册
const int loginType = 9000001;//登录
const int modifyOwnPasswordType = 9000002;//修改个人密码
const int modifyOwnNameType = 9000003;//修改个人昵称
const int modifyOwnSignType = 9000004;//修改个人签名
const int modifyGroupNameType = 9000005;//修改群昵称
const int searchFriendType = 9000006;//查找好友
const int addFriendType = 9000007;//添加好友
const int delFriendType = 9000008;//删除好友
const int createGroupType = 9000009;//创建群聊
const int searchGroupType = 9000010;//查找群
const int addGroupType = 9000011;//添加群
const int delGroupType = 9000012;//删除群
const int searchFriendsType = 9000013;//查找好友列表
const int searchGroupsType = 9000014;//查找群列表
const int searchFriendsOfGroupType = 9000015;//查找群内好友列表
const int chatFriendType = 9000016;//好友聊天
const int chatGroupType = 9000017;//群内聊天
const int modifyOwnImageType = 9000018;//修改个人头像
const int modifyGroupImageType = 9000019;//修改群头像

const int unknownType = 9999998;//未知类型,数据错误


ClientSocket::ClientSocket(const QSqlDatabase &db, QTcpSocket *parent) :
    QTcpSocket(parent)
{
    ownId = 0;
    ownName = "";
    dbo = new DatabaseOperator(db);

    connect(this,SIGNAL(readyRead()),this,SLOT(receiveData()));
    connect(this,SIGNAL(disconnected()),this,SLOT(disconnection()));
}

ClientSocket::~ClientSocket()
{

}

QByteArray ClientSocket::readDataLength(QByteArray &data, uint dataLength)
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

QByteArray lastRemain;
QByteArray type;
ushort where = 0;
uint length;

void ClientSocket::receiveData()
{
    QByteArray byteAll = readAll();
    byteAll = lastRemain+byteAll;

    while(byteAll.size() > 0){
        QByteArray data;
        switch(where){
        case 0:{
            data = readDataLength(byteAll,7);
            if(data.size() < 7){
                lastRemain = data;
                where = 0;
                return ;
            }
            lastRemain.clear();
            where = 0;
            length = stringToInt(data);
            if(length == 0) {
                where = 0;
                continue ;
            }
        }
        case 1:{
            data = readDataLength(byteAll,length);
            if(data.size() < (int)length){
                lastRemain = data;
                where = 1;
                return ;
            }
            lastRemain.clear();
            where = 0;

            QJsonObject json = QJsonDocument::fromJson(data).object();
            int type = stringToInt(json.value("type").toString());
            if(type == 0) continue;

            switch(type){
            case signUpType: signUp(json);break;//注册账号
            case loginType: loginVerify(json);break;//登录请求
            case modifyOwnPasswordType: modifyOwnPassword(json);break;//修改个人密码
            case modifyOwnNameType: modifyOwnName(json);break;//修改个人昵称
            case modifyOwnSignType: modifyOwnSign(json);break;//修改个人签名
            case modifyGroupNameType: modifyGroupName(json);break;//修改群昵称
            case searchFriendType: searchFriend(json);break;//查找好友
            case addFriendType: addFriend(json);break;//添加好友
            case delFriendType: delFriend(json);break;//删除好友
            case createGroupType: createGroup(json);break;//创建群聊
            case searchGroupType: searchGroup(json);break;//查找群
            case addGroupType: addGroup(json);break;//添加群
            case delGroupType: delGroup(json);break;//删除群
            case searchFriendsType: searchFriends(ownId);break;//查找好友列表
            case searchGroupsType: searchGroups(ownId);break;//查找群列表
            case searchFriendsOfGroupType: searchFriendsOfGroup(json);break;//查找群内好友列表
            case chatFriendType: chatFriend(json);break;//好友聊天
            case chatGroupType: chatGroup(json);break;//群内聊天
            case modifyOwnImageType: modifyOwnImage(json);break;//修改个人头像
            case modifyGroupImageType: modifyGroupImage(json);break;//修改群头像
            default:{
                QJsonObject json;
                json.insert("type",QString::number(unknownType));
                QByteArray data = QJsonDocument(json).toJson();
                sendData(data);
                break;
            }
            }
        }
        default:break;
        }
    }
}

void ClientSocket::disconnection()
{
    if(ownId != 0) dbo->logout(ownId);
    delete dbo;
    emit toServerDisconnection(this->socketDescriptor(),ownId);
}

qulonglong ClientSocket::getId()
{
    return ownId;
}

void sleep(unsigned int msec){
    QTime reachTime = QTime::currentTime().addMSecs(msec);
    while(QTime::currentTime() < reachTime){
        QCoreApplication::processEvents(QEventLoop::AllEvents,msec);
    }
}

void ClientSocket::sendData(const QByteArray &data)
{
    QString len = QString::number(data.size());
    len = QString(7-len.size(),'0')+len;
    write(len.toLocal8Bit(),7);
    if(data.size() > 0) write(data,data.size());
}

void ClientSocket::sendTips(int type, const QByteArray &data)
{
    QJsonObject json;
    json.insert("type",QString::number(type));
    json.insert("information",QString::fromLatin1(data));
    QByteArray data2 = QJsonDocument(json).toJson();
    sendData(data2);
}

void ClientSocket::signUp(const QJsonObject &json)
{
    QString ownName = json.value("ownName").toString();
    QString ownPassword = json.value("ownPassword").toString();
    qulonglong ownId;
    bool isSuccessful = dbo->signUp(ownName,ownPassword,ownId);
    if(!isSuccessful){//注册失败
        sendTips(signUpType,requestFailed);
        emit toServerUpdate("类型:"+QString::number(signUpType)+"  "+requestFailed);
        close();
        return ;
    }
    QJsonObject json2;
    json2.insert("type",QString::number(signUpType));
    json2.insert("ownId",QString::number(ownId));
    QByteArray data = QJsonDocument(json2).toJson();
    sendData(data);//发送注册到的账号
    emit toServerUpdate("类型:"+QString::number(signUpType)+"  "+complete);
    close();
}

void ClientSocket::loginVerify(const QJsonObject &json)
{
    qulonglong id = stringToQulonglong(json.value("ownId").toString());
    QString password = json.value("ownPassword").toString();
    if(id == 0){
        sendTips(loginType,numberError);
        emit toServerUpdate("类型:"+QString::number(loginType)+"  "+numberError);
        close();
        return ;
    }
    QByteArray information;
    QString name;
    bool isSuccess = dbo->loginVerify(id,password,peerAddress().toString(),information,name);
    sendTips(loginType,information);//发送尝试登录返回信息
    emit toServerUpdate("类型:"+QString::number(loginType)+"  "+information);
    if(!isSuccess) {
        ownId = 0;
        ownName = "";
        close();//登录失败,关闭连接
        return ;
    }
    emit toServerSuccessful(id,this->socketDescriptor());//登录成功
    ownId = id;//保存tcp连接客户账号
    ownName = name;
    searchFriends(ownId);//发送好友列表信息
    searchGroups(ownId);//发送群列表信息

    sendOfflineMsg(ownId);//发送离线消息
}

void ClientSocket::modifyOwnPassword(const QJsonObject &json)
{
    QString oldPassword = json.value("oldPassword").toString();
    QString newPassword = json.value("newPassword").toString();
    bool isSuccessful = dbo->modifyOwnPassword(ownId,oldPassword,newPassword);
    if(!isSuccessful){
        sendTips(modifyOwnPasswordType,requestFailed);
        emit toServerUpdate("类型:"+QString::number(modifyOwnPasswordType)+"  "+requestFailed);
        return ;
    }
    sendTips(modifyOwnPasswordType,requestSuccessful);
    emit toServerUpdate("类型:"+QString::number(modifyOwnPasswordType)+"  "+requestSuccessful);
}

void ClientSocket::modifyOwnName(const QJsonObject &json)
{
    QString newName = json.value("newName").toString();
    bool isSuccessful = dbo->modifyOwnName(ownId,newName);
    if(!isSuccessful){
        sendTips(modifyOwnNameType,requestFailed);
        emit toServerUpdate("类型:"+QString::number(modifyOwnNameType)+"  "+requestFailed);
        return ;
    }
    ownName = newName;
    sendTips(modifyOwnNameType,requestSuccessful);
    emit toServerUpdate("类型:"+QString::number(modifyOwnNameType)+"  "+requestSuccessful);
}

void ClientSocket::modifyOwnSign(const QJsonObject &json)
{
    QString newSign = json.value("newSign").toString();
    bool isSuccessful = dbo->modifyOwnSign(ownId,newSign);
    if(!isSuccessful){
        sendTips(modifyOwnSignType,requestFailed);
        emit toServerUpdate("类型:"+QString::number(modifyOwnSignType)+"  "+requestFailed);
        return ;
    }
    sendTips(modifyOwnSignType,requestSuccessful);
    emit toServerUpdate("类型:"+QString::number(modifyOwnSignType)+"  "+requestSuccessful);
}

void ClientSocket::modifyOwnImage(const QJsonObject &json)
{
    QByteArray newImage = json.value("newImage").toString().toLatin1();
    QString imageType = json.value("imageType").toString();
    QPixmap image;
    image.loadFromData(newImage);
    QString filePath = "./images/persons/"+QString::number(ownId)+".jpg";
    QPixmap oldImage(filePath);
    if(!oldImage.isNull()){
        QFile::remove(filePath);
    }
    bool isSuccessful = image.save("./images/persons/"+QString::number(ownId)+"."+imageType);
    if(!isSuccessful){
        if(!oldImage.isNull()){
            oldImage.save(filePath);
        }
        sendTips(modifyOwnImageType,requestFailed);
        emit toServerUpdate("类型:"+QString::number(modifyOwnImageType)+"  "+requestFailed);
        return ;
    }
    sendTips(modifyOwnImageType,requestSuccessful);
    emit toServerUpdate("类型:"+QString::number(modifyOwnImageType)+"  "+requestSuccessful);
}

void ClientSocket::modifyGroupName(const QJsonObject &json)
{
    QString newName = json.value("newName").toString();
    qulonglong groupId = stringToQulonglong(json.value("groupId").toString());
    if(groupId == 0){
        sendTips(modifyGroupNameType,numberError);
        emit toServerUpdate("类型:"+QString::number(modifyGroupNameType)+"  "+numberError);
        return ;
    }
    bool isSuccessful = dbo->modifyGroupName(groupId,newName);
    if(!isSuccessful){
        sendTips(modifyGroupNameType,requestFailed);
        emit toServerUpdate("类型:"+QString::number(modifyGroupNameType)+"  "+requestFailed);
        return ;
    }
    emit toServerUpdate("类型:"+QString::number(modifyGroupNameType)+"  "+requestSuccessful);
    QSqlQuery *query = dbo->searchFriendsOfGroup(groupId);
    if(query == nullptr){
        sendTips(modifyGroupNameType,unknownError);
        emit toServerUpdate("类型:"+QString::number(modifyGroupNameType)+"  "+unknownError);
        return ;
    }
    QJsonObject json2;
    json2.insert("type",QString::number(modifyGroupNameType));
    json2.insert("groupId",QString::number(groupId));
    json2.insert("groupName",newName);
    QByteArray data = QJsonDocument(json2).toJson();
    while(query->next()){
        if(query->value(2).toBool()){//好友在线,在线消息
            emit toServerData(query->value(0).toULongLong(),data);
        }
    }
    emit toServerUpdate("类型:"+QString::number(modifyGroupNameType)+"  "+complete);
    delete query;
}

void ClientSocket::modifyGroupImage(const QJsonObject &json)
{
    QByteArray newImage = json.value("newImage").toString().toLatin1();
    QString imageType = json.value("imageType").toString();
    qulonglong groupId = stringToQulonglong(json.value("groupId").toString());
    if(groupId == 0){
        sendTips(modifyGroupImageType,numberError);
        emit toServerUpdate("类型:"+QString::number(modifyGroupImageType)+"  "+numberError);
        return ;
    }
    QPixmap image;
    image.loadFromData(newImage);
    QString filePath = "./images/groups/"+QString::number(groupId)+".jpg";
    QPixmap oldImage(filePath);
    if(!oldImage.isNull()){
        QFile::remove(filePath);
    }
    bool isSuccessful = image.save("./images/groups/"+QString::number(groupId)+"."+imageType);
    if(!isSuccessful){
        if(!oldImage.isNull()){
            oldImage.save(filePath);
        }
        sendTips(modifyGroupImageType,requestFailed);
        emit toServerUpdate("类型:"+QString::number(modifyGroupImageType)+"  "+requestFailed);
        return ;
    }
    emit toServerUpdate("类型:"+QString::number(modifyGroupImageType)+"  "+requestSuccessful);

    QSqlQuery *query = dbo->searchFriendsOfGroup(groupId);
    if(query == nullptr){
        sendTips(modifyGroupImageType,unknownError);
        emit toServerUpdate("类型:"+QString::number(modifyGroupImageType)+"  "+unknownError);
        return ;
    }
    QJsonObject json2;
    json2.insert("type",QString::number(modifyGroupImageType));
    json2.insert("groupId",QString::number(groupId));
    json2.insert("groupImage",QString::fromLatin1(newImage,newImage.size()));
    QByteArray data = QJsonDocument(json2).toJson();
    while(query->next()){
        if(query->value(2).toBool()){//好友在线,在线消息
            emit toServerData(query->value(0).toULongLong(),data);
        }
    }
    emit toServerUpdate("类型:"+QString::number(modifyGroupImageType)+"  "+complete);
    delete query;
}

void ClientSocket::searchFriend(const QJsonObject &json)
{
    qulonglong friendId = stringToQulonglong(json.value("friendId").toString());
    if(friendId == 0){
        sendTips(searchFriendType,numberError);
        emit toServerUpdate("类型:"+QString::number(searchFriendType)+"  "+numberError);
        return ;
    }
    QSqlQuery *query = dbo->searchFriend(friendId);
    if(query == nullptr){
        sendTips(searchFriendType,unknownError);
        emit toServerUpdate("类型:"+QString::number(searchFriendType)+"  "+unknownError);
        return ;
    }
    if(!query->record().isEmpty()){
        if(query->next()){//发送好友信息
            QPixmap pixmap("./images/persons/"+QString::number(friendId)+".jpg");
            sendFriend(searchFriendType,query->value(0).toULongLong(),query->value(1).toString(),query->value(2).toString(),pixmap);
            emit toServerUpdate("类型:"+QString::number(searchFriendType)+"  "+complete);
        }
        else{
            sendTips(searchFriendType,notExist);
            emit toServerUpdate("类型:"+QString::number(searchFriendType)+"  "+notExist);
        }
    }
    else{//账号不存在
        sendTips(searchFriendType,notExist);
        emit toServerUpdate("类型:"+QString::number(searchFriendType)+"  "+notExist);
    }
    delete query;
}

void ClientSocket::addFriend(const QJsonObject &json)
{
    qulonglong requestId = stringToQulonglong(json.value("requestId").toString());
    qulonglong friendId = stringToQulonglong(json.value("friendId").toString());
    if(requestId == 0 || friendId == 0){
        sendTips(addFriendType,numberError);
        emit toServerUpdate("类型:"+QString::number(addFriendType)+"  "+numberError);
        return ;
    }
    if(requestId != ownId){//离线,由接收方同意加好友
        QSqlQuery *query = dbo->searchFriend(requestId);
        if(query == nullptr){
            sendTips(addFriendType,unknownError);
            emit toServerUpdate("类型:"+QString::number(addFriendType)+"  "+unknownError);
            return ;
        }
        if(query->record().isEmpty()){//申请加好友者账号注销,不存在
            sendTips(addFriendType,notExist);
            emit toServerUpdate("类型:"+QString::number(addFriendType)+"  "+notExist);
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
            emit toServerUpdate("类型:"+QString::number(addFriendType)+"  "+friendExist);
            return ;
        }
        sendTips(addFriendType,unknownError);
        emit toServerUpdate("类型:"+QString::number(addFriendType)+"  "+unknownError);
        return ;
    }
    QSqlQuery *query = dbo->searchFriend(friendId);
    if(query == nullptr){
        sendTips(addFriendType,unknownError);
        emit toServerUpdate("类型:"+QString::number(addFriendType)+"  "+unknownError);
        return ;
    }
    if(query->next()){
        //发送好友信息
        QPixmap pixmap("./images/persons/"+QString::number(friendId)+".jpg");
        sendFriend(addFriendType,query->value(0).toULongLong(),query->value(1).toString(),query->value(2).toString(),pixmap);
        emit toServerUpdate("类型:"+QString::number(addFriendType)+"  "+complete);
        if(query->value(3).toBool()){//对方在线,及时给对方更新好友信息
            delete query;
            query = nullptr;
            QSqlQuery *query2 = dbo->searchFriend(ownId);
            if(query2 == nullptr){
                sendTips(addFriendType,unknownError);
                emit toServerUpdate("类型:"+QString::number(addFriendType)+"  "+unknownError);
                return ;
            }
            if(query2->next()){
                QPixmap pixmap("./images/persons/"+QString::number(ownId)+".jpg");
                QByteArray image;
                pixmapToByteArray(image,pixmap);

                QJsonObject json2;
                json2.insert("type",QString::number(addFriendType));
                json2.insert("friendId",QString::number(ownId));
                json2.insert("friendName",query2->value(1).toString());
                json2.insert("friendSign",query2->value(2).toString());
                json2.insert("friendImage",QString::fromLatin1(image,image.size()));
                QByteArray data = QJsonDocument(json2).toJson();
                emit toServerData(friendId,data);
            }
            delete query2;
        }
    }
    if(query != nullptr){
    	delete query;
        query = nullptr;
    }
}

void ClientSocket::delFriend(const QJsonObject &json)
{
    qulonglong friendId = stringToQulonglong(json.value("friendId").toString());
    if(friendId == 0){
        sendTips(delFriendType,numberError);
        emit toServerUpdate("类型:"+QString::number(delFriendType)+"  "+numberError);
        return ;
    }
    bool isSuccessful = dbo->delFriend(ownId,friendId);
    if(!isSuccessful){//删除失败
        sendTips(delFriendType,unknownError);
        emit toServerUpdate("类型:"+QString::number(delFriendType)+"  "+unknownError);
        return ;
    }
    QJsonObject json2;
    json2.insert("type",QString::number(delFriendType));
    json2.insert("friendId",QString::number(friendId));
    QByteArray data = QJsonDocument(json2).toJson();
    sendData(data);
    emit toServerUpdate("类型:"+QString::number(delFriendType)+"  "+complete);
    QSqlQuery *query = dbo->searchFriend(friendId);
    if(!query->record().isEmpty()){
        if(query->next()){
            if(query->value(3).toBool()){//对方在线,及时更新好友信息
                QJsonObject json3;
                json3.insert("type",QString::number(delFriendType));
                json3.insert("friendId",QString::number(ownId));
                QByteArray data = QJsonDocument(json3).toJson();
                emit toServerData(friendId,data);
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
        emit toServerUpdate("类型:"+QString::number(createGroupType)+"  "+requestFailed);
        return ;
    }
    sendGroup(createGroupType,groupId,groupName,QPixmap());
    emit toServerUpdate("类型:"+QString::number(createGroupType)+"  "+complete);
}

void ClientSocket::searchGroup(const QJsonObject &json)
{
    qulonglong groupId = stringToQulonglong(json.value("groupId").toString());
    if(groupId == 0){
        sendTips(searchGroupType,numberError);
        emit toServerUpdate("类型:"+QString::number(searchGroupType)+"  "+numberError);
        return ;
    }
    QSqlQuery *query = dbo->searchGroup(groupId);
    if(query == nullptr){
        sendTips(searchGroupType,unknownError);
        emit toServerUpdate("类型:"+QString::number(searchGroupType)+"  "+unknownError);
        return ;
    }
    if(!query->record().isEmpty()){
        if(query->next()){//发送群信息
            QPixmap pixmap("./images/groups/"+QString::number(groupId)+".jpg");
            sendGroup(searchGroupType,query->value(0).toULongLong(),query->value(1).toString(),pixmap);
            emit toServerUpdate("类型:"+QString::number(searchGroupType)+"  "+complete);
        }
        else{
            sendTips(searchGroupType,notExist);
            emit toServerUpdate("类型:"+QString::number(searchGroupType)+"  "+notExist);
        }
    }
    else{//群不存在
        sendTips(searchGroupType,notExist);
        emit toServerUpdate("类型:"+QString::number(searchGroupType)+"  "+notExist);
    }
    delete query;
}

void ClientSocket::addGroup(const QJsonObject &json)
{
    qulonglong requestId = stringToQulonglong(json.value("requestId").toString());
    qulonglong groupId = stringToQulonglong(json.value("groupId").toString());
    if(requestId == 0 || groupId == 0){
        sendTips(addGroupType,numberError);
        emit toServerUpdate("类型:"+QString::number(addGroupType)+"  "+numberError);
        return ;
    }
    if(requestId != ownId){//离线,由群内人员同意加群
        QSqlQuery *query = dbo->searchFriend(requestId);
        if(query == nullptr){
            sendTips(addGroupType,unknownError);
            emit toServerUpdate("类型:"+QString::number(addGroupType)+"  "+unknownError);
            return ;
        }
        if(query->record().isEmpty()){//申请加群者账号注销,不存在
            sendTips(addGroupType,notExist);
            emit toServerUpdate("类型:"+QString::number(addGroupType)+"  "+notExist);
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
            emit toServerUpdate("类型:"+QString::number(addGroupType)+"  "+groupExist);
            return ;
        }
        sendTips(addGroupType,unknownError);
        emit toServerUpdate("类型:"+QString::number(addGroupType)+"  "+unknownError);
        return ;
    }
    QSqlQuery *query = dbo->searchGroup(groupId);
    if(query->next()){
        //发送群信息
        QPixmap pixmap("./images/groups/"+QString::number(groupId)+".jpg");
        sendGroup(addGroupType,query->value(0).toULongLong(),query->value(1).toString(),pixmap);
        emit toServerUpdate("类型:"+QString::number(addGroupType)+"  "+complete);
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
        emit toServerUpdate("类型:"+QString::number(delGroupType)+"  "+numberError);
        return ;
    }
    bool isSuccessful = dbo->delGroup(ownId,groupId);
    if(!isSuccessful){//删除群失败
        sendTips(delGroupType,unknownError);
        emit toServerUpdate("类型:"+QString::number(delGroupType)+"  "+unknownError);
        return ;
    }
    QJsonObject json2;
    json2.insert("type",QString::number(delGroupType));
    json2.insert("groupId",QString::number(groupId));
    QByteArray data = QJsonDocument(json2).toJson();
    sendData(data);
    emit toServerUpdate("类型:"+QString::number(delGroupType)+"  "+complete);
    /*
     * 未及时更新给其他群内人员.如果其他群内人员已打开聊天窗口,刚退出的好友不可接收消息,但显示此好友
     */
}

void ClientSocket::searchFriends(qulonglong ownId)
{
    QSqlQuery *query = dbo->searchFriends(ownId);
    if(query == nullptr){
        sendTips(searchFriendsType,unknownError);
        emit toServerUpdate("类型:"+QString::number(searchFriendsType)+"  "+unknownError);
        return ;
    }
    while(query->next()){//至少有一个自己
        QPixmap pixmap("./images/persons/"+query->value(0).toString()+".jpg");
        qDebug()<<pixmap.isNull();
        sendFriend(searchFriendsType,query->value(0).toULongLong(),query->value(1).toString(),query->value(2).toString(),pixmap);
    }
    emit toServerUpdate("类型:"+QString::number(searchFriendsType)+"  "+complete);
    delete query;
}

void ClientSocket::searchGroups(qulonglong ownId)
{
    QSqlQuery *query = dbo->searchGroups(ownId);
    if(query == nullptr){
        sendTips(searchGroupsType,unknownError);
        emit toServerUpdate("类型:"+QString::number(searchGroupsType)+"  "+unknownError);
        return ;
    }
    if(!query->record().isEmpty()){
        while(query->next()){
            QPixmap pixmap("./images/groups/"+query->value(0).toString()+".jpg");
            sendGroup(searchGroupsType,query->value(0).toULongLong(),query->value(1).toString(),pixmap);
        }
        emit toServerUpdate("类型:"+QString::number(searchGroupsType)+"  "+complete);
    }
    delete query;
}

void ClientSocket::searchFriendsOfGroup(const QJsonObject &json)
{
    qulonglong groupId = stringToQulonglong(json.value("groupId").toString());
    if(groupId == 0){
        sendTips(searchFriendsOfGroupType,numberError);
        emit toServerUpdate("类型:"+QString::number(searchFriendsOfGroupType)+"  "+numberError);
        return ;
    }
    searchFriendsOfGroup(groupId);
}

void ClientSocket::searchFriendsOfGroup(qulonglong groupId)
{
    QSqlQuery *query = dbo->searchFriendsOfGroup(groupId);
    if(query == nullptr){
        sendTips(searchFriendsOfGroupType,unknownError);
        emit toServerUpdate("类型:"+QString::number(searchFriendsOfGroupType)+"  "+unknownError);
    }
    while(query->next()){//群内至少有一人,由数据库控制群内无人解散群
        QPixmap pixmap("./images/persons/"+query->value(0).toString()+".jpg");
        QByteArray image;
        pixmapToByteArray(image,pixmap);
        QJsonObject json;
        json.insert("type",QString::number(searchFriendsOfGroupType));
        json.insert("friendId",QString::number(query->value(0).toULongLong()));
        json.insert("friendName",query->value(1).toString());
        json.insert("friendSign",query->value(2).toString());
        json.insert("friendImage",QString::fromLatin1(image,image.size()));
        json.insert("groupId",QString::number(groupId));
        QByteArray data = QJsonDocument(json).toJson();
        sendData(data);
    }
    emit toServerUpdate("类型:"+QString::number(searchFriendsOfGroupType)+"  "+complete);
    delete query;
}

void ClientSocket::chatFriend(const QJsonObject &json)
{
    qulonglong friendId = stringToQulonglong(json.value("friendId").toString());
    if(friendId == 0){
        sendTips(chatFriendType,numberError);
        emit toServerUpdate("类型:"+QString::number(chatFriendType)+"  "+numberError);
        return ;
    }
    QSqlQuery *query = dbo->searchFriend(friendId);
    if(query == nullptr){
        sendTips(chatFriendType,unknownError);
        emit toServerUpdate("类型:"+QString::number(chatFriendType)+"  "+unknownError);
        return ;
    }
    QJsonObject json2;
    json2.insert("type",QString::number(chatFriendType));
    json2.insert("msg",json.value("msg").toString());
    json2.insert("sendId",QString::number(ownId));
    json2.insert("sendName",ownName);
    QByteArray data = QJsonDocument(json2).toJson();
    if(query->next()){
        if(query->value(3).toBool()){//好友在线,在线消息
            emit toServerData(friendId,data);
            emit toServerUpdate("类型:"+QString::number(chatFriendType)+"  对方在线,发送完成.");
        }
        else{
            QString fileName = "./msg/"+QString::number(friendId)+".txt";
            QFile file(fileName);
            file.open(QIODevice::Append);
            file.write(QString::number(data.size()).toLocal8Bit()+'\n');
            file.write(QString(data).toUtf8());
            file.close();
            emit toServerUpdate("类型:"+QString::number(chatFriendType)+"  对方不在线,已保存.");
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
        emit toServerUpdate("类型:"+QString::number(chatGroupType)+"  "+numberError);
        return ;
    }
    QSqlQuery *query = dbo->searchFriendsOfGroup(groupId);
    if(query == nullptr){
        sendTips(chatGroupType,unknownError);
        emit toServerUpdate("类型:"+QString::number(chatGroupType)+"  "+unknownError);
        return ;
    }
    QJsonObject json2;
    json2.insert("type",QString::number(chatGroupType));
    json2.insert("msg",json.value("msg").toString());
    json2.insert("sendId",QString::number(ownId));
    json2.insert("sendName",ownName);
    json2.insert("groupId",QString::number(groupId));
    json2.insert("groupName",groupName);
    QByteArray data = QJsonDocument(json2).toJson();
    while(query->next()){
        if(query->value(0).toULongLong() == ownId) continue;//不发给自己
        if(query->value(2).toBool()){//好友在线,在线消息
            emit toServerData(query->value(0).toULongLong(),data);
            emit toServerUpdate("类型:"+QString::number(chatGroupType)+"  群成员:"+query->value(0).toString()+"在线,发送完成.");
        }
        else{
            QString fileName = "./msg/"+query->value(0).toString()+".txt";
            QFile file(fileName);
            file.open(QIODevice::Append);
            file.write(QString::number(data.size()).toLocal8Bit()+'\n');
            file.write(QString(data).toUtf8());
            file.close();
            emit toServerUpdate("类型:"+QString::number(chatGroupType)+"  群成员:"+query->value(0).toString()+"不在线,已保存.");
        }
    }
    delete query;
}

void ClientSocket::sendFriend(int type, const qulonglong &friendId,
                              const QString &friendName, const QString &friendSign,
                              const QPixmap &friendImage)
{
    QByteArray image;
    pixmapToByteArray(image,friendImage);

    QJsonObject json;
    json.insert("type",QString::number(type));
    json.insert("friendId",QString::number(friendId));
    json.insert("friendName",friendName);
    json.insert("friendSign",friendSign);
    json.insert("friendImage",QString::fromLatin1(image,image.size()));
    QByteArray data = QJsonDocument(json).toJson();
    sendData(data);
}

void ClientSocket::sendGroup(int type, const qulonglong &groupId,
                             const QString &groupName, const QPixmap &groupImage)
{
    QByteArray image;
    pixmapToByteArray(image,groupImage);

    QJsonObject json;
    json.insert("type",QString::number(type));
    json.insert("groupId",QString::number(groupId));
    json.insert("groupName",groupName);
    json.insert("groupImage",QString::fromLatin1(image,image.size()));
    QByteArray data = QJsonDocument(json).toJson();
    sendData(data);
}

void ClientSocket::sendOfflineMsg(qulonglong ownId)
{
    QString filePath = "./msg/"+QString::number(ownId)+".txt";
    QFile file(filePath);
    if(file.exists()){
        file.open(QIODevice::ReadWrite);
        while(true){
            QByteArray len = file.readLine();
            if(len == "") break;
            uint length = len.toUInt();
            QByteArray data = file.read(length);
            QString str = byteArrayToUnicode(data);
            QByteArray data_ut8 = str.toUtf8();
            sendData(data_ut8);

            /*改完自动弹出聊天窗口删除下面代码*/
            QJsonObject json = QJsonDocument::fromJson(data).object();
            if(json.value("type").toInt() == chatGroupType)
                searchFriendsOfGroup(json.value("groupId").toString().toULongLong());
        }
        emit toServerUpdate("离线消息已发送.");
        file.close();
        if(!QFile::remove(filePath)){
            file.open(QIODevice::WriteOnly);
            file.close();
        }
    }
}

void ClientSocket::pixmapToByteArray(QByteArray &imageByteArray, const QPixmap &pixmap)
{
    if(pixmap.isNull()){
        imageByteArray = "";
        return ;
    }
    QBuffer buffer;
    pixmap.save(&buffer,"jpg");
    imageByteArray.resize(buffer.data().size());
    imageByteArray = buffer.data();
}

int ClientSocket::stringToInt(const QString &string)
{
    bool isSuccess;
    int number = string.toInt(&isSuccess,10);//数据长度
    emit toServerUpdate(string);
    emit toServerUpdate(QString::number(number,10));
    if(!isSuccess) {
        emit toServerUpdate("QString转换int失败");
        return 0;
    }
    else {
        emit toServerUpdate("QString转换int成功");
        return number;
    }
}

qulonglong ClientSocket::stringToQulonglong(const QString &string)
{
    bool isSuccess;
    qulonglong number = string.toULongLong(&isSuccess,10);
    emit toServerUpdate(string);
    emit toServerUpdate(QString::number(number,10));
    if(!isSuccess) {
        emit toServerUpdate("QString转换qulonglong失败");
        return 0;
    }
    else {
        emit toServerUpdate("QString转换qulonglong成功");
        return number;
    }
}

QString ClientSocket::byteArrayToUnicode(const QByteArray &array)
{
    QString text = QTextCodec::codecForName("UTF-8")->toUnicode(array);
    return text;
}


