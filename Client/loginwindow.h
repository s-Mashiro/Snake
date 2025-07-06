#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QTcpSocket>
#include <QWidget>

namespace Ui {
class LoginWindow;
}

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();
    QString currentUserName;

protected:
    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent *e);

signals:
    void loginSuccessful(const QString &Uname);
    void gotoRegister();

private slots:
    void on_pushButton_login_clicked();
    void on_pushButton_register_clicked();
    void onSocketReadyRead();

private:
    Ui::LoginWindow *ui;
    QTcpSocket *Logsocket;
};

#endif // LOGINWINDOW_H
