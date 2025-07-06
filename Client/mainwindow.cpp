#include "mainwindow.h"

#include <QDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include "./ui_mainwindow.h"
#include "gameboard.h"
#include "global.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    GameBoard *gameBoard = new GameBoard(this);
    setCentralWidget(gameBoard);
    setFixedSize(gameBoard->sizeHint());

    connect(gameBoard, &GameBoard::gameOver, this, &MainWindow::onGameOver);

    connect(ui->action_restart, &QAction::triggered, this, [this]() {
        if (auto board = qobject_cast<GameBoard *>(centralWidget())) {
            board->initGame();
        }
    });

    connect(ui->action_rank, &QAction::triggered, this, &MainWindow::onActionRank);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onGameOver(int score) {
    uint8_t header = 0x03; // 自定义“分数上报”协议头
    QByteArray ub = currentUserName.toUtf8();
    QByteArray sb = QByteArray::number(score);
    uint8_t uname_len = static_cast<uint8_t>(ub.size());
    uint8_t score_len = static_cast<uint8_t>(sb.size());

    if (clientSocket->state() != QAbstractSocket::ConnectedState) {
        clientSocket->connectToHost(ipAddress, port);
        if (!clientSocket->waitForConnected(10000)) {
            QMessageBox::warning(this, "发送失败", "无法连接到服务器！");
            return;
        }
    }

    clientSocket->write(reinterpret_cast<char *>(&header), 1);
    clientSocket->write(reinterpret_cast<char *>(&uname_len), 1);
    clientSocket->write(ub);
    clientSocket->write(reinterpret_cast<char *>(&score_len), 1);
    clientSocket->write(sb);
    clientSocket->waitForBytesWritten();
}

void MainWindow::onActionRank() {
    uint8_t header = 0x04;
    if (clientSocket->state() != QAbstractSocket::ConnectedState) {
        clientSocket->connectToHost(ipAddress, port);
        if (!clientSocket->waitForConnected(10000)) {
            QMessageBox::warning(this, "排行榜", "无法连接到服务器获取排行榜");
            return;
        }
    }
    clientSocket->write(reinterpret_cast<char *>(&header), 1);
    clientSocket->flush();

    if (!clientSocket->waitForReadyRead(10000)) {
        QMessageBox::warning(this, "排行榜", "读取排行榜超时");
        clientSocket->disconnectFromHost();
        return;
    }

    QByteArray resp = clientSocket->read(1);
    if (resp.size() != 1 || static_cast<uint8_t>(resp.at(0)) != 0x01) {
        QMessageBox::warning(this, "排行榜", "服务器返回错误");
        clientSocket->disconnectFromHost();
        return;
    }

    QByteArray data = clientSocket->readAll();
    clientSocket->disconnectFromHost();

    QString text = QString::fromUtf8(data);
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    int rows = lines.size();
    int cols = 3;

    QDialog dlg(this);
    dlg.setWindowTitle("排行榜");
    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    QTableWidget *table = new QTableWidget(rows, cols, &dlg);
    table->setHorizontalHeaderLabels(QStringList() << "用户名" << "分数" << "时间");
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    for (int i = 0; i < rows; ++i) {
        QStringList parts = lines.at(i).split('\t');
        for (int j = 0; j < parts.size() && j < cols; ++j) {
            table->setItem(i, j, new QTableWidgetItem(parts.at(j)));
        }
    }
    layout->addWidget(table);
    dlg.resize(600, 400);
    dlg.exec();
}

void MainWindow::onSocketReadyRead() {
}
