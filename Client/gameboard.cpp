#include "gameboard.h"

#include <QMessageBox>
#include <QPainter>
#include <QRandomGenerator>

#include "ui_gameboard.h"

GameBoard::GameBoard(QWidget *parent)
    : QWidget(parent)
    , direction(Right)
    , nextDirection(Right)
    , paused(true)
    , score(0)
    , gridSize(20)
    , rows(30)
    , cols(40) {
    setFocusPolicy(Qt::StrongFocus);
    setFixedSize(cols * gridSize, rows * gridSize);
    connect(&timer, &QTimer::timeout, this, &GameBoard::updateGame);
    timer.start(200);
    initGame();
}

QSize GameBoard::sizeHint() const {
    return QSize(cols * gridSize, (rows + 2) * gridSize);
}

QSize GameBoard::minimumSizeHint() const {
    return sizeHint();
}

void GameBoard::initGame() {
    snake.clear();
    score = 0;
    direction = Right;
    nextDirection = Right;
    paused = true;

    for (int i = 5; i >= 1; --i) snake.push_back({i, 5});

    generateFood();
    update();
}

void GameBoard::generateFood() {
    while (true) {
        int fx = QRandomGenerator::global()->bounded(cols);
        int fy = QRandomGenerator::global()->bounded(rows);
        bool conflict = false;
        for (const auto &s : snake) {
            if (s.x == fx && s.y == fy) {
                conflict = true;
                break;
            }
        }
        if (!conflict) {
            food = {fx, fy};
            break;
        }
    }
}

void GameBoard::paintEvent(QPaintEvent *) {
    QPainter p(this);

    p.fillRect(rect(), Qt::black);

    // for (int i = 0; i < cols; ++i)
    //     for (int j = 0; j < rows; ++j)
    //         p.drawRect(i * gridSize, j * gridSize, gridSize, gridSize);

    p.setBrush(Qt::green);
    for (const auto &s : snake) {
        p.drawRect(s.x * gridSize, s.y * gridSize, gridSize, gridSize);
    }

    p.setBrush(Qt::red);
    p.drawEllipse(food.x * gridSize, food.y * gridSize, gridSize, gridSize);
}

void GameBoard::keyPressEvent(QKeyEvent *event) {
    if (paused) paused = false;

    switch (event->key()) {
        case Qt::Key_Up:
            if (direction != Down) nextDirection = Up;
            break;
        case Qt::Key_Down:
            if (direction != Up) nextDirection = Down;
            break;
        case Qt::Key_Left:
            if (direction != Right) nextDirection = Left;
            break;
        case Qt::Key_Right:
            if (direction != Left) nextDirection = Right;
            break;
        case Qt::Key_W: // W = Up
            if (direction != Down) nextDirection = Up;
            break;
        case Qt::Key_S: // S = Down
            if (direction != Up) nextDirection = Down;
            break;
        case Qt::Key_A: // A = Left
            if (direction != Right) nextDirection = Left;
            break;
        case Qt::Key_D: // D = Right
            if (direction != Left) nextDirection = Right;
            break;
        case Qt::Key_Space:
            paused = !paused;
            break;
    }

    QWidget::keyPressEvent(event);
}

void GameBoard::updateGame() {
    if (paused) return;

    moveSnake();

    if (checkCollision()) {
        paused = true;

        emit gameOver(score);

        QMessageBox::information(this, "游戏结束", QString("你的得分: %1").arg(score));
        initGame();
        return;
    }

    update();
}

void GameBoard::moveSnake() {
    direction = nextDirection;
    SnakeNode head = snake.front();

    switch (direction) {
        case Up:
            head.y--;
            break;
        case Down:
            head.y++;
            break;
        case Left:
            head.x--;
            break;
        case Right:
            head.x++;
            break;
    }

    snake.push_front(head);

    if (head.x == food.x && head.y == food.y) {
        score += 1;
        generateFood();
    } else {
        snake.pop_back();
    }
}

bool GameBoard::checkCollision() {
    SnakeNode head = snake.front();

    if (head.x < 0 || head.x >= cols || head.y < 0 || head.y >= rows) return true;

    for (size_t i = 1; i < snake.size(); ++i) {
        if (snake[i].x == head.x && snake[i].y == head.y) return true;
    }

    return false;
}
