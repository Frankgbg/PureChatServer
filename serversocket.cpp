#include "serversocket.h"

#include <QSqlDatabase>
#include <QSqlError>

ServerSocket::ServerSocket(quint16 port, QTcpServer *parent) :
    QTcpServer(parent)
{
    numberOfPeople = 0;
    clientSocketList = new QList<ClientSocket*>;

    db = new QSqlDatabase(QSqlDatabase::addDatabase("QODBC"));
    QString dsn = QString::fromLocal8Bit("PureChatDataSource");//数据源名称
    db->setHostName("localhost");//选择本地主机，127.0.0.1
    db->setDatabaseName(dsn);    //设置数据源名称
    db->setUserName("G_bg");     //登录用户
    db->setPassword("********"); //密码
    db->setPort(1433);          //数据库端口

    listen(QHostAddress("172.16.225.210"),port);//监听端口
}

ServerSocket::~ServerSocket()
{
    while(clientSocketList->count() > 0){
        clientSocketList->at(0)->close();
        clientSocketList->removeAt(0);
    }
    delete clientSocketList;
    if(db->isOpen()) db->close();
    delete db;
}

void ServerSocket::incomingConnection(qintptr handle)
{
    ClientSocket *clientSocket = new ClientSocket(QSqlDatabase::database());//建立新的客户tcp

    connect(clientSocket,SIGNAL(toServerData(qulonglong,const QByteArray&,const QByteArray&)),
            this,SLOT(clientData(qulonglong,const QByteArray&,const QByteArray&)));
    connect(clientSocket,SIGNAL(toServerUpdata(QString)),this,SLOT(clientUpdate(QString)));
    connect(clientSocket,SIGNAL(toServerDisconnection(qintptr)),this,SLOT(clientDisconnection(qintptr)));

    clientSocket->setSocketDescriptor(handle);//设置socket描述符
    clientSocketList->append(clientSocket);//添加到socket列表

    emit toUiUpdate("<br><br><br>");
    emit toUiUpdate("新的连接：IP:"+clientSocket->peerAddress().toString());
    emit toUiNumber(++numberOfPeople);
}

void ServerSocket::clientData(qulonglong friendId, const QByteArray &type, const QByteArray &data)
{
    emit toUiUpdate("已接收消息");
    for(int i = 0;i < clientSocketList->count();++i){
        ClientSocket *item = clientSocketList->at(i);
        if(item->getId() == friendId){
            item->send(type,data);
            emit toUiUpdate("已发送消息");
            return ;
        }
    }
}

void ServerSocket::clientUpdate(QString msg)
{
    emit toUiUpdate(msg);//发送信号给PureChatServer界面
}

void ServerSocket::clientDisconnection(qintptr descriptor)
{
    for(int i = 0;i < clientSocketList->count();++i){
        ClientSocket *item = clientSocketList->at(i);
        if(item->socketDescriptor() == descriptor){
            emit toUiUpdate("IP:"+item->peerAddress().toString()+" is disconnected!<br><br><br>");
            emit toUiNumber(--numberOfPeople);
            clientSocketList->removeAt(i);//客户端断开连接,删除套接字
            return ;
        }
    }
}
