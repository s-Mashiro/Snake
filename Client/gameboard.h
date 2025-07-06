#ifndef SNAKEGAMEWIDGET_H
#define SNAKEGAMEWIDGET_H

#include <QKeyEvent>
#include <QTimer>
#include <QWidget>
#include <atomic>
#include <deque>

struct SnakeNode {
    int x, y;
};

enum Direction { Up, Down, Left, Right };

class GameBoard : public QWidget {
    Q_OBJECT
public:
    explicit GameBoard(QWidget *parent = nullptr);
    void initGame();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void gameOver(int finalScore);

protected:
    void paintEvent(QPaintEvent *) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateGame();

private:
    // void initGame();
    void moveSnake();
    void generateFood();
    bool checkCollision();
    bool isPaused() const;
    GameBoard *gameBoard;

private:
    std::deque<SnakeNode> snake;
    SnakeNode food;
    Direction direction;
    Direction nextDirection;
    QTimer timer;
    bool paused;
    int score;
    int gridSize;
    int rows, cols;
};

#endif // SNAKEGAMEWIDGET_H
