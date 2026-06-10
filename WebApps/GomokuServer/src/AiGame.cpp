#include "AiGame.h"

#include <chrono>
#include <thread>


AiGame::AiGame(int userId)
    : gameOver_(false)
    , userId_(userId)
    , moveCount_(0)
    , lastMove_(-1, -1)
    , board_(BOARD_SIZE, std::vector<std::string>(BOARD_SIZE, EMPTY))
{
	srand(time(0)); // 初始化随机数种子
}

/**
 * @brief 处理人类玩家的落子操作。
 * 
 * 验证移动的有效性，更新棋盘状态，并检查是否有人类玩家获胜。
 * 
 * @param x 落子的行坐标。
 * @param y 落子的列坐标。
 * @return 如果移动有效并成功执行则返回 true，否则返回 false。
 */
bool AiGame::humanMove(int x, int y) 
{
    if (!isValidMove(x, y)) 
        return false;
    
    board_[x][y] = HUMAN_PLAYER;
    moveCount_++;
    lastMove_ = {x, y};
    
    if (checkWin(x, y, HUMAN_PLAYER)) 
    {
        gameOver_ = true;
        winner_ = "human";
    }
    return true;
}

 /**
 * @brief 执行人工智能玩家的移动逻辑。
 * 
 * 该函数内部包含AI决策算法，计算最佳落子位置并执行移动。
 */
void AiGame::aiMove() 
{
    if (gameOver_ || isDraw()) return;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 添加500毫秒延时
    int x, y;
    // 获取AI的最佳移动位置
    std::tie(x, y) = getBestMove();
    board_[x][y] = AI_PLAYER;
    moveCount_++;
    lastMove_ = {x, y};
    
    if (checkWin(x, y, AI_PLAYER)) 
    {
        gameOver_ = true;
        winner_ = "ai";
    }
}


/**
 * @brief 评估指定位置的威胁程度
 * 
 * 通过检查四个主要方向（垂直、水平、两条对角线）上连续的人类棋子数量，
 * 计算给定坐标点的潜在威胁值。威胁值越高，表示该位置附近人类玩家的连子越多，
 * AI越需要关注此位置进行防守或拦截。
 * 
 * @param r 行索引，表示要评估的棋盘行位置
 * @param c 列索引，表示要评估的棋盘列位置
 * @return int 威胁分数，值为各方向上探测到的人类棋子总数（包含起始点本身）
 */
int AiGame::evaluateThreat(int r, int c) 
{
    int threat = 0;

    // 检查四个方向上的玩家连子数
    const int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    for (auto& dir : directions) 
    {
        int count = 1;
        for (int i = 1; i <= 2; i++) 
        { // 探查2步
            int nr = r + i * dir[0], nc = c + i * dir[1];
            if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board_[nr][nc] == HUMAN_PLAYER) 
            {
                count++;
            }
        }
        threat += count; // 威胁分数累加
    }
    return threat;
}

/**
 * @brief 判断某个空位是否靠近已有棋子。
 * 
 * @param r 行坐标。
 * @param c 列坐标。
 * @return bool 如果靠近已有棋子返回 true，否则返回 false。
 */
bool AiGame::isNearOccupied(int r, int c) 
{
    const int directions[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}
    };
    for (auto& dir : directions) 
    {
        int nr = r + dir[0], nc = c + dir[1];
        if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE && board_[nr][nc] != EMPTY) 
        {
            return true; // 该空位靠近已有棋子
        }
    }
    return false;
}

/**
 * @brief 检查指定位置落子后，该玩家是否获胜（五子连珠）
 * 
 * 通过检查水平、垂直、主对角线和副对角线四个方向，
 * 统计连续相同棋子的数量，判断是否达到5个及以上。
 * 
 * @param x 当前落子的横坐标
 * @param y 当前落子的纵坐标
 * @param player 当前玩家标识字符串
 * @return true 如果当前玩家获胜
 * @return false 如果当前玩家未获胜
 */
bool AiGame::checkWin(int x, int y, const std::string& player) 
{
    // 检查方向数组：水平、垂直、对角线、反对角线
    const int dx[] = {1, 0, 1, 1};
    const int dy[] = {0, 1, 1, -1};
    
    for (int dir = 0; dir < 4; dir++) 
    {
        int count = 1;  // 当前位置已经有一个棋子
        
        // 正向检查
        for (int i = 1; i < 5; i++) 
        {
            int newX = x + dx[dir] * i;
            int newY = y + dy[dir] * i;
            if (!isInBoard(newX, newY) || board_[newX][newY] != player) break;
            count++;
        }
        
        // 反向检查
        for (int i = 1; i < 5; i++) 
        {
            int newX = x - dx[dir] * i;
            int newY = y - dy[dir] * i;
            if (!isInBoard(newX, newY) || board_[newX][newY] != player) break;
            count++;
        }
        
        if (count >= 5) return true;
    }
    return false;
}

/**
 * @brief 获取AI的最佳落子位置。
 * 
 * 该函数通过多层策略决定AI的下一步行动，优先级如下：
 * 1. 检查是否存在立即获胜的机会，若有则直接落子。
 * 2. 检查玩家是否存在立即获胜的威胁，若有则进行拦截防守。
 * 3. 若无紧急胜负情况，评估所有空位的威胁程度，选择威胁值最高的位置。
 * 4. 若无法评估出有效威胁点，优先选择靠近已有棋子的空位以维持局势关联。
 * 5. 作为兜底策略，选择棋盘上第一个可用的空位。
 * 
 * @return std::pair<int, int> 返回最佳落子的行号和列号组成的 pair。
 */
std::pair<int, int> AiGame::getBestMove()
{
    std::pair<int, int> bestMove = {-1, -1}; // 最佳落子位置
    int maxThreat = -1;                      // 记录最大的威胁分数

    // 1. 优先尝试进攻获胜或阻止玩家获胜
    for (int r = 0; r < BOARD_SIZE; r++) 
    {
        for (int c = 0; c < BOARD_SIZE; c++) 
        {
            if (board_[r][c] != EMPTY) continue; // 确保当前位置为空闲

            // 模拟 AI 落子，判断是否可以获胜
            board_[r][c] = AI_PLAYER;
            if (checkWin(r, c, AI_PLAYER)) 
            {
                // board_[r][c] = AI_PLAYER; // 恢复棋盘
                return {r, c};      // 立即获胜
            }
            board_[r][c] = EMPTY;

            // 模拟玩家落子，判断是否需要防守
            board_[r][c] = HUMAN_PLAYER;
            if (checkWin(r, c, HUMAN_PLAYER)) 
            {
                board_[r][c] = AI_PLAYER; // 恢复棋盘
                return {r, c};      // 立即防守
            }
            board_[r][c] = EMPTY;
        }
    }

    // 2. 评估每个空位的威胁程度，选择最佳防守位置
    for (int r = 0; r < BOARD_SIZE; r++) 
    {
        for (int c = 0; c < BOARD_SIZE; c++) 
        {
            if (board_[r][c] != EMPTY) continue; // 确保当前位置为空闲

            int threatLevel = evaluateThreat(r, c); // 评估威胁程度
            if (threatLevel > maxThreat) 
            {
                maxThreat = threatLevel;
                bestMove = {r, c};
            }
        }
    }

    // 3. 如果找不到威胁点，选择靠近玩家或已有棋子的空位
    if (bestMove.first == -1) 
    {
        std::vector<std::pair<int, int>> nearCells;

        for (int r = 0; r < BOARD_SIZE; r++) 
        {
            for (int c = 0; c < BOARD_SIZE; c++) 
            {
                if (board_[r][c] == EMPTY && isNearOccupied(r, c)) 
                { // 确保当前位置为空闲且靠近已有棋子
                    nearCells.push_back({r, c});
                }
            }
        }

        // 如果找到靠近已有棋子的空位，随机选择一个
        if (!nearCells.empty()) 
		{
            int num = rand();
			board_[nearCells[num % nearCells.size()].first][nearCells[num % nearCells.size()].second] = AI_PLAYER;
            return nearCells[num % nearCells.size()];
        }

        // 4. 如果所有策略都无效，选择第一个空位（保证 AI 落子）
        for (int r = 0; r < BOARD_SIZE; r++) 
        {
            for (int c = 0; c < BOARD_SIZE; c++) 
            {
                if (board_[r][c] == EMPTY) 
				{
					board_[r][c] = AI_PLAYER;
                    return {r, c}; // 返回第一个空位
                }
            }
        }
    }
	
	board_[bestMove.first][bestMove.second] = AI_PLAYER;
    return bestMove; // 返回最佳防守点或其他策略的结果
}