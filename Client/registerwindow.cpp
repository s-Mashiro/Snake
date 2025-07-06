#include "registerwindow.h"

#include <arpa/inet.h>

#include <QByteArray>
#include <QMessageBox>
#include <QTcpSocket>

#include "global.h"
#include "ui_registerwindow.h"

RegisterWindow::RegisterWindow(QWidget *parent) : QWidget(parent), ui(new Ui::RegisterWindow) {
    ui->setupUi(this);

    connect(ui->pushButton_reg,
            &QPushButton::clicked,
            this,
            &RegisterWindow::on_pushButton_reg_clicked);
}

RegisterWindow::~RegisterWindow() {
    delete ui;
}

void RegisterWindow::on_pushButton_reg_clicked() {
    currentUserName = ui->lineEdit_Uname->text();

    QString uname = currentUserName;
    QString pwd = ui->lineEdit_pwd->text();
    QString email = ui->lineEdit_email->text();

    if (uname.trimmed().isEmpty()) {
        QMessageBox::warning(this, "注册失败", "用户名不能为空！");
        return;
    }
    if (pwd.trimmed().isEmpty()) {
        QMessageBox::warning(this, "注册失败", "密码不能为空！");
        return;
    }
    if (email.trimmed().isEmpty()) {
        QMessageBox::warning(this, "注册失败", "邮箱不能为空！");
        return;
    }

    uint8_t header = 0x01;
    QByteArray ub = uname.toUtf8();
    QByteArray pb = pwd.toUtf8();
    QByteArray eb = email.toUtf8();
    uint8_t uname_len = static_cast<uint8_t>(ub.size());
    uint8_t pwd_len = static_cast<uint8_t>(pb.size());
    uint8_t email_len = static_cast<uint8_t>(eb.size());

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
    clientSocket->write(reinterpret_cast<char *>(&email_len), 1);
    clientSocket->write(eb);
    clientSocket->waitForBytesWritten();
}

void RegisterWindow::onSocketReadyRead() {
    QByteArray response = clientSocket->readAll();
    if (response.isEmpty()) {
        QMessageBox::warning(this, "Reg:服务器错误", "未收到服务器响应！");
        return;
    }

    uint8_t status = static_cast<uint8_t>(response.at(0));
    if (status == 0x01) {
        if (clientSocket->state() == QAbstractSocket::ConnectedState) {
            disconnect(
                clientSocket, &QTcpSocket::readyRead, this, &RegisterWindow::onSocketReadyRead);
        }
        emit registerSuccessful();
    } else if (status == 0x02) {
        QMessageBox::warning(this, "注册失败", "用户名已存在！");
    } else {
        QMessageBox::warning(
            this,
            "注册失败",
            QString("服务器返回未知状态：0x%1").arg(status, 2, 16, QLatin1Char('0')));
    }
}

void RegisterWindow::showEvent(QShowEvent *e) {
    QWidget::showEvent(e);
    QObject::connect(
        clientSocket, &QTcpSocket::readyRead, this, &RegisterWindow::onSocketReadyRead);
}

void RegisterWindow::hideEvent(QHideEvent *e) {
    QWidget::hideEvent(e);
    QObject::disconnect(
        clientSocket, &QTcpSocket::readyRead, this, &RegisterWindow::onSocketReadyRead);
}
