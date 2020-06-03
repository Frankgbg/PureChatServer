#include "purechatserver.h"
#include "databaseoperator.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QApplication>
#include <QJsonObject>
#include <QHostAddress>
#include <QString>
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    PureChatServer w;
    w.show();
    return a.exec();


    QString fileName = "./msg/"+QString::number(100001)+".txt";
    QFile file(fileName);
    if(file.exists()){
        file.open(QIODevice::ReadWrite);
        while(true){
            QByteArray type = file.readLine();
            if(type == "") break;
            uint length = file.readLine().toUInt();
            QByteArray data = file.read(length);
            QTextCodec::ConverterState state;
            QString text = QTextCodec::codecForName("UTF-8")->toUnicode(data.constData(), data.size(), &state);
            if (state.invalidChars > 0){
                text = QTextCodec::codecForName("GBK")->toUnicode(data);
            }
            qDebug()<<type;
            qDebug()<<length;
            qDebug()<<text;
        }
        file.close();
    }
    return 0;

    QSqlDatabase *db = new QSqlDatabase(QSqlDatabase::addDatabase("QODBC"));
    QString dsn = QString::fromLocal8Bit("PureChatDataSource");//数据源名称
    db->setHostName("localhost");//选择本地主机，127.0.0.1
    db->setDatabaseName(dsn);    //设置数据源名称
    db->setUserName("G_bg");     //登录用户
    db->setPassword("19980728"); //密码
    db->setPort(1433);          //数据库端口
    DatabaseOperator dbo(QSqlDatabase::database());
    bool isSuccess;
    //isSuccess = dbo.createGroup(1446965342,"哈哈哈",id);
    //isSuccess = dbo.insertGroups("12345","hhh");
    QString information;
    isSuccess = dbo.modifyOwnPassword(1446965342,"123456","12345");
    qDebug()<<isSuccess << " " <<information;

    //qulonglong maUserId = dbo.maxUserId();
    //qulonglong maGroupId = dbo.maxGroupId();
    //qDebug()<<maUserId;
    //qDebug()<<maGroupId;
    delete db;
    return 0;
}
