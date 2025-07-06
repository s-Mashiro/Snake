#include <QApplication>
#include <QDebug>
#include <QMessageBox>

#include "global.h"
#include "loginwindow.h"
#include "mainwindow.h"
#include "registerwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    clientSocket = new QTcpSocket();

    LoginWindow *loginWindow = new LoginWindow();
    MainWindow *mainWindow = new MainWindow();
    RegisterWindow *registerWindow = new RegisterWindow();
    loginWindow->show();

    QObject::connect(
        loginWindow, &LoginWindow::loginSuccessful, mainWindow, [&](const QString &Uname) {
            currentUserName = Uname;
            loginWindow->close();

            mainWindow->show();
        });

    QObject::connect(loginWindow, &LoginWindow::gotoRegister, registerWindow, [&]() {
        loginWindow->hide();
        clientSocket->disconnect(loginWindow);
        registerWindow->show();
    });

    QObject::connect(registerWindow, &RegisterWindow::registerSuccessful, loginWindow, [&]() {
        registerWindow->hide();
        loginWindow->show();
    });

    qDebug() << "Login as: " << currentUserName;

    return a.exec();
}
