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

//    QSqlDatabase db(QSqlDatabase(QSqlDatabase::addDatabase("QODBC","PureChat")));
//    QString dsn = QString::fromLocal8Bit("PureChatDataSource");//数据源名称
//    db.setHostName("localhost");//选择本地主机，127.0.0.1
//    db.setDatabaseName(dsn);    //设置数据源名称
//    db.setUserName("G_bg");     //登录用户
//    db.setPassword("19980728"); //密码
//    db.setPort(1433);          //数据库端口
//    DatabaseOperator dbo(QSqlDatabase::database("PureChat"));
//    QSqlQuery *query = dbo.searchFriend(100000);
//    if(query == nullptr){
//        qDebug()<<"empty";
//        return 0;
//    }
//    if(query->next()){
//        qDebug()<<query->value(0).toULongLong()<<query->value(1).toString()
//               <<query->value(2).toString()<<query->value(3).toBool();
//    }
//    delete query;
//    return 0;
}
