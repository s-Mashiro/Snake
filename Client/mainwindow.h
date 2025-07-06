#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "gameboard.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onGameOver(int finalScore);

    void onActionRank();
    void onSocketReadyRead();

protected:
    // void showEvent(QShowEvent *e);
    // void hideEvent(QHideEvent *e);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
