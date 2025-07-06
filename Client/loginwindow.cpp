#include "loginwindow.h"

#include <arpa/inet.h>

#include <QByteArray>
#include <QMessageBox>
#include <QString>
#include <QTcpSocket>

#include "global.h"
#include "ui_loginwindow.h"

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent), ui(new Ui::LoginWindow) {
    ui->setupUi(this);

    connect(ui->pushButton_login,
            &QPushButton::clicked,
            this,
            &LoginWindow::on_pushButton_login_clicked);
    connect(ui->pushButton_register,
            &QPushButton::clicked,
            this,
            &LoginWindow::on_pushButton_register_clicked);
}

LoginWindow::~LoginWindow() {
    delete ui;
}

void LoginWindow::on_pushButton_login_clicked() {
    currentUserName = ui->lineEdit_Uname->text();

    QString uname = currentUserName;
    QString pwd = ui->lineEdit_pwd->text();

    if (uname.trimmed().isEmpty()) {
        QMessageBox::warning(this, "登录失败", "用户名不能为空！");
        return;
    }
    if (pwd.trimmed().isEmpty()) {
        QMessageBox::warning(this, "登录失败", "密码不能为空！");
        return;
    }

    uint8_t header = 0x02;
    QByteArray ub = uname.toUtf8();
    QByteArray pb = pwd.toUtf8();
    uint8_t uname_len = static_cast<uint8_t>(ub.size());
    uint8_t pwd_len = static_cast<uint8_t>(pb.size());

    if (clientSocket->state() != QAbstractSocket::ConnectedState) {
        clientSocket->connectToHost(ipAddress, port);
        if (!clientSocket->waitForConnected(10000)) {
            QMessageBox::warning(this, "连接失败", "无法连接到服务器！");
            return;
        }
    }

    clientSocket->write(reinterpret_cast<char *>(&header), 1);
    clientSocket->write(reinterpret_cast<char *>(&uname_len), 1);
    clientSocket->write(ub);
    clientSocket->write(reinterpret_cast<char *>(&pwd_len), 1);
    clientSocket->write(pb);
    clientSocket->waitForBytesWritten();

    clientSocket->readAll();
}

void LoginWindow::on_pushButton_register_clicked() {
    if (clientSocket->state() == QAbstractSocket::ConnectedState) {
        disconnect(clientSocket, &QTcpSocket::readyRead, this, &LoginWindow::onSocketReadyRead);
    }

    emit gotoRegister();
}

void LoginWindow::onSocketReadyRead() {
    QByteArray response = clientSocket->readAll();

    if (response.size() < 1) {
        return;
    }

    uint8_t status = static_cast<uint8_t>(response.at(0));

    if (status == 0x01) {
        if (clientSocket->state() == QAbstractSocket::ConnectedState) {
            disconnect(clientSocket, &QTcpSocket::readyRead, this, &LoginWindow::onSocketReadyRead);
        }
        emit loginSuccessful(currentUserName);
    } else if (status == 0x02) {
        QMessageBox::warning(this, "登录失败", "用户名或密码错误！");
    } else {
        QMessageBox::warning(
            this, "登录失败", QString("收到未知响应：0x%1").arg(status, 2, 16, QLatin1Char('0')));
    }
}

void LoginWindow::showEvent(QShowEvent *e) {
    QWidget::showEvent(e);
    QObject::connect(clientSocket, &QTcpSocket::readyRead, this, &LoginWindow::onSocketReadyRead);
}

void LoginWindow::hideEvent(QHideEvent *e) {
    QWidget::hideEvent(e);
    QObject::disconnect(
        clientSocket, &QTcpSocket::readyRead, this, &LoginWindow::onSocketReadyRead);
}
