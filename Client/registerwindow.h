#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H

#include <QTcpSocket>
#include <QWidget>

namespace Ui {
class RegisterWindow;
}

class RegisterWindow : public QWidget {
    Q_OBJECT

public:
    explicit RegisterWindow(QWidget *parent = nullptr);
    QString currentUserName;
    ~RegisterWindow();

protected:
    void showEvent(QShowEvent *e);
    void hideEvent(QHideEvent *e);

signals:
    void registerSuccessful();

private slots:
    void on_pushButton_reg_clicked();
    void onSocketReadyRead();

private:
    Ui::RegisterWindow *ui;
    QTcpSocket *Regsocket;
};

#endif // REGISTERWINDOW_H
