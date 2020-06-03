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
}
