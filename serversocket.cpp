#include "serversocket.h"

#include <QSqlDatabase>
#include <QSqlError>

ServerSocket::ServerSocket(quint16 port, QTcpServer *parent) :
    QTcpServer(parent)
{
    numberOfPeople = 0;
    clientTemporary = new QHash<qintptr, ClientSocket*>;
    clientSocket = new QHash<qulonglong, ClientSocket*>;

    db = new QSqlDatabase(QSqlDatabase::addDatabase("QODBC","PureChat"));
    QString dsn = QString::fromLocal8Bit("PureChatDataSource");//数据源名称
    db->setHostName("localhost");//选择本地主机，127.0.0.1
    db->setDatabaseName(dsn);    //设置数据源名称
    db->setUserName("G_bg");     //登录用户
    db->setPassword("********"); //密码
    db->setPort(****);          //数据库端口

    listen(QHostAddress("172.16.225.210"),port);//监听端口
}

ServerSocket::~ServerSocket()
{
    while(!clientTemporary->isEmpty()){
        clientTemporary->begin().value()->close();
        clientTemporary->erase(clientTemporary->begin());
    }
    delete clientTemporary;

    while(!clientSocket->isEmpty()){
        clientSocket->begin().value()->close();
        clientSocket->erase(clientSocket->begin());
    }
    delete clientSocket;

    if(db->isOpen()) db->close();
    delete db;
}

void ServerSocket::incomingConnection(qintptr handle)
{
    ClientSocket *client = new ClientSocket(QSqlDatabase::database("PureChat"));//建立新的客户tcp

    client->setSocketOption(QAbstractSocket::KeepAliveOption,true);
    connect(client,SIGNAL(toServerData(qulonglong,const QByteArray&)),
            this,SLOT(clientData(qulonglong,const QByteArray&)));
    connect(client,SIGNAL(toServerUpdate(QString)),this,SLOT(clientUpdate(QString)));
    connect(client,SIGNAL(toServerDisconnection(qintptr,qulonglong)),
            this,SLOT(clientDisconnection(qintptr,qulonglong)));
    connect(client,SIGNAL(toServerSuccessful(qulonglong, qintptr)),
            this,SLOT(signUpSuccessful(qulonglong, qintptr)));

    client->setSocketDescriptor(handle);//设置socket描述符
    clientTemporary->insert(handle,client);//添加到socket列表

    emit toUiUpdate("<br><br><br>新的连接：IP:"+client->peerAddress().toString());
    emit toUiNumber(++numberOfPeople);
}

void ServerSocket::clientData(qulonglong friendId, const QByteArray &data)
{
    auto ite = clientSocket->find(friendId);
    if(ite != clientSocket->end()){
        ite.value()->sendData(data);
    }
}

void ServerSocket::clientUpdate(QString msg)
{
    emit toUiUpdate(msg);//发送信息给PureChatServer界面
}

void ServerSocket::clientDisconnection(qintptr descriptor, qulonglong id)
{
    auto ite = clientTemporary->find(descriptor);
    if(ite != clientTemporary->end()){//临时连接断开
        clientTemporary->erase(ite);
        emit toUiUpdate("IP:"+ite.value()->peerAddress().toString()+" is disconnected!<br><br><br>");
        emit toUiNumber(--numberOfPeople);
        return ;
    }
    auto ite2 = clientSocket->find(id);
    if(ite2 != clientSocket->end()){
        emit toUiUpdate("IP:"+ite2.value()->peerAddress().toString()+" is disconnected!<br><br><br>");
        emit toUiNumber(--numberOfPeople);
        clientSocket->erase(ite2);//客户端断开连接,删除套接字
    }
}

void ServerSocket::signUpSuccessful(qulonglong id, qintptr handle)
{
    //登录成功,转移临时连接至常连接
    auto ite = clientTemporary->find(handle);
    if(ite != clientTemporary->end()){
        clientSocket->insert(id,ite.value());
        clientTemporary->erase(ite);
    }
}
