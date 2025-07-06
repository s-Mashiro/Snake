#ifndef GLOBAL_H
#define GLOBAL_H

#include <QHostAddress>
#include <QObject>
#include <QTcpSocket>

extern QTcpSocket *clientSocket;
extern QHostAddress ipAddress;
extern qint32 port;
extern QString currentUserName;

#endif // GLOBAL_H
