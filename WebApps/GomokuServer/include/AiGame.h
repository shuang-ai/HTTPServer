#pragma once

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>
#include <mutex>

const int BOARD_SIZE = 15;

const std::string EMPTY = "empty";
const std::string AI_PLAYER = "white";   // AI玩家白棋
const std::string HUMAN_PLAYER = "black"; // 人类玩家黑棋

/**
 * @brief 五子棋AI游戏逻辑类
 * 
 * 该类封装了15x15棋盘的游戏状态管理、规则判定以及AI自动落子逻辑。
 * 支持多线程安全访问，通过互斥锁保护共享状态。
 */
class AiGame
{
public:
    AiGame(int userId);

    /**
     * @brief 检查当前棋局是否以平局结束。
     *
     * 当已落子数量达到棋盘总格数时，判定为平局。
     * 该函数是线程安全的，内部通过互斥锁保护共享状态。
     *
     * @return bool 如果棋局为平局则返回 true，否则返回 false。
     */
    bool isDraw() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return moveCount_ >= BOARD_SIZE * BOARD_SIZE;
    }

    /**
     * @brief 处理人类玩家的移动操作。
     * 
     * @param x 目标位置的横坐标。
     * @param y 目标位置的纵坐标。
     * @return bool 如果移动合法并成功执行则返回 true，否则返回 false。
     */
    bool humanMove(int x, int y);

    /**
     * @brief 检查指定玩家在给定位置落子后是否获胜。
     * 
     * @param x 最新落子的横坐标。
     * @param y 最新落子的纵坐标。
     * @param player 当前玩家的身份标识字符串。
     * @return bool 如果该玩家达成胜利条件则返回 true，否则返回 false。
     */
    bool checkWin(int x,int y, const std::string& player);

    /**
     * @brief 执行人工智能玩家的移动逻辑。
     * 
     * 该函数内部包含AI决策算法，计算最佳落子位置并执行移动。
     */
    void aiMove();

    /**
     * @brief 获取最后一步移动的坐标。
     * 
     * @return std::pair<int, int> 最后一步移动的坐标。
     */
    std::pair<int, int> getLastMove() const 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return lastMove_;
    }

    /**
     * @brief 获取当前棋盘状态。
     * 
     * @return const std::vector<std::vector<std::string>>& 当前棋盘状态的引用。
     */
    const std::vector<std::vector<std::string>>& getBoard() const 
    { 
        std::lock_guard<std::mutex> lock(mutex_);
        return board_; 
    }

    /**
     * @brief 检查游戏是否已经结束。
     * 
     * 该函数通过加锁确保线程安全地读取游戏结束状态。
     * 
     * @return true 如果游戏已经结束。
     * @return false 如果游戏仍在进行中。
     */
    bool isGameOver() const 
    { 
        std::lock_guard<std::mutex> lock(mutex_);
        return gameOver_; 
    }

    /**
     * @brief 获取当前比赛的获胜者。
     * 
     * 该函数是线程安全的，通过互斥锁保护对共享状态 winner_ 的访问。
     * 
     * @return std::string 返回获胜者的名称或标识。如果没有获胜者，可能返回空字符串。
     */
    std::string getWinner() const 
    { 
        std::lock_guard<std::mutex> lock(mutex_);
        return winner_; 
    }

private:
    /**
     * @brief 检查指定坐标的移动是否合法。
     * 
     * 验证移动合法性的条件包括：
     * 1. 坐标在棋盘范围内。
     * 2. 目标位置为空。
     * 3. 游戏尚未结束且未平局。
     *
     * @param x 目标位置的横坐标。
     * @param y 目标位置的纵坐标。
     * @return 如果移动合法返回 true，否则返回 false。
     */
    bool isValidMove(int x, int y) const 
    {
        if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) return false;
        if (board_[x][y] != EMPTY) return false;
        if (gameOver_ || isDraw()) return false;
        return true;
    }

    /**
     * @brief 检查坐标是否在棋盘内。
     * 
     * @param x 横坐标。
     * @param y 纵坐标。
     * @return bool 如果坐标在棋盘内返回 true，否则返回 false。
     */
    bool isInBoard(int x, int y) const 
    {
        return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
    }

    /**
     * @brief 获取AI的最佳移动位置。
     * 
     * @return std::pair<int, int> 最佳落子位置的坐标。
     */
    std::pair<int, int> getBestMove();

    /**
     * @brief 评估某个位置的威胁程度。
     * 
     * @param r 行坐标。
     * @param c 列坐标。
     * @return int 威胁分数。
     */
    int evaluateThreat(int r, int c);
    
    /**
     * @brief 判断某个空位是否靠近已有棋子。
     * 
     * @param r 行坐标。
     * @param c 列坐标。
     * @return bool 如果靠近已有棋子返回 true，否则返回 false。
     */
    bool isNearOccupied(int r, int c);

private:
    bool                                  gameOver_;
    int                                   userId_;
    int                                   moveCount_;
    std::string                           winner_{"none"};
    std::pair<int, int>                   lastMove_{-1, -1};  // 上一次落子位置
    std::vector<std::vector<std::string>> board_;
    mutable std::mutex                    mutex_;  // 添加互斥锁
};