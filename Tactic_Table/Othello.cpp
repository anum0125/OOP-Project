#include "raylib.h"
#include <iostream>
#include <climits>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 640;
const int BOARD_SIZE = 8;
const int CELL_SIZE = SCREEN_WIDTH / BOARD_SIZE;

enum Cell { EMPTY, Black_Disc, White_Disc };
enum GameState { MENU, MODE_SELECTION, GAMEPLAY };
enum GameResult { NONE, BLACK_WINS, WHITE_WINS, DRAW };

class Board {
    public:
        Cell board[BOARD_SIZE][BOARD_SIZE];
        Cell currentPlayer;
    
        Board() {
            currentPlayer = Black_Disc;
            Initialize_Board();
        }
    
        void Initialize_Board() {
            for (int row = 0; row < BOARD_SIZE; row++)
                for (int col = 0; col < BOARD_SIZE; col++)
                    board[row][col] = EMPTY;
    
            board[3][3] = White_Disc;
            board[3][4] = Black_Disc;
            board[4][3] = Black_Disc;
            board[4][4] = White_Disc;
        }
    
        bool Is_Within_Boundaries(int x, int y) {
            return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
        }
    
        bool TryFlip(int x, int y, int dx, int dy, bool flip) {
            int cx = x + dx;
            int cy = y + dy;
            int count = 0;
    
            while (Is_Within_Boundaries(cx, cy) && board[cy][cx] == (currentPlayer == Black_Disc ? White_Disc : Black_Disc)) {
                cx += dx;
                cy += dy;
                count++;
            }
    
            if (count > 0 && Is_Within_Boundaries(cx, cy) && board[cy][cx] == currentPlayer) {
                if (flip) {
                    for (int i = 1; i <= count; i++)
                        board[y + i * dy][x + i * dx] = currentPlayer;
                }
                return true;
            }
    
            return false;
        }
    
        bool CanPlace(int x, int y, bool flip) {
            if (board[y][x] != EMPTY) return false;
    
            bool valid = false;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    if (!(dx == 0 && dy == 0))
                        valid |= TryFlip(x, y, dx, dy, flip);
    
            return valid;
        }
    
        void PlacePiece(int x, int y) {
            if (CanPlace(x, y, true)) {
                board[y][x] = currentPlayer;
                currentPlayer = (currentPlayer == Black_Disc) ? White_Disc : Black_Disc;
            }
        }
    
        void DrawBoard() {
            Color Board_Background_Color = LIGHTGRAY;
            Color Grid_Line_Color = BROWN;
            Color Black_Disc_Color = BLACK;
            Color White_Disc_Color = WHITE;
    
            ClearBackground(Board_Background_Color);
    
            for (int y = 0; y < BOARD_SIZE; y++) {
                for (int x = 0; x < BOARD_SIZE; x++) {
                    DrawRectangleLines(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, Grid_Line_Color);
                    if (board[y][x] != EMPTY) {
                        Color Disc_Color = (board[y][x] == Black_Disc) ? Black_Disc_Color : White_Disc_Color;
                        DrawCircle(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, CELL_SIZE / 2 - 5, Disc_Color);
                    }
                }
            }
        }

        Board Clone() {
            Board copy;
            for (int y = 0; y < BOARD_SIZE; y++)
                for (int x = 0; x < BOARD_SIZE; x++)
                    copy.board[y][x] = board[y][x];
            copy.currentPlayer = currentPlayer;
            return copy;
        }
        bool HasValidMove(bool isWhite) {
            for (int y = 0; y < BOARD_SIZE; y++) {
                for (int x = 0; x < BOARD_SIZE; x++) {
                    if (board[y][x] == EMPTY && CanPlace(x, y, isWhite)) {
                        return true;
                    }
                }
            }
            return false;
        }  
                      
    };
    // Utility for drawing button
    bool DrawButton(Rectangle bounds, const char* text) {
        Vector2 mouse = GetMousePosition();
        bool hovered = CheckCollisionPointRec(mouse, bounds);
        DrawRectangleRec(bounds, hovered ? GRAY : LIGHTGRAY);
        DrawText(text, bounds.x + 10, bounds.y + 10, 20, BLACK);
        return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    }

class Player {
    public:
        virtual void MakeMove(Board& board, GameResult& result, bool& gameOver) = 0;
        virtual void ShowScore(int blackCount, int whiteCount) = 0;
        virtual void EndGame(GameResult result) = 0;
        virtual void ReturnToMenu(GameState& gameState) = 0;
    
        virtual ~Player() {}
    };
 
class HumanPlayer : public Player {
public:
    void MakeMove(Board& board, GameResult& result, bool& gameOver) override {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();
            int x = mouse.x / CELL_SIZE;
            int y = mouse.y / CELL_SIZE;
            if (board.Is_Within_Boundaries(x, y)) {
                if (board.CanPlace(x, y, false)) {
                    board.PlacePiece(x, y);
                }
            }
        }
    }

    void ShowScore(int blackCount, int whiteCount) override {
        std::cout << "Black: " << blackCount << " | White: " << whiteCount << "\n";
    }

    void EndGame(GameResult result) override {
        // Optional: print message or set flags
    }

    void ReturnToMenu(GameState& gameState) override {
        gameState = MENU;
    }
};

class AIPlayer : public Player {
public:
    // Improved evaluation function with corner control
    int EvaluateBoard(Board& board) {
        int blackScore = 0, whiteScore = 0;
    
        // Weight map: more value to corners and edges
        int weight[8][8] = {
            {100, -20, 10, 5, 5, 10, -20, 100},
            {-20, -50, -2, -2, -2, -2, -50, -20},
            {10, -2, 0, 0, 0, 0, -2, 10},
            {5, -2, 0, 0, 0, 0, -2, 5},
            {5, -2, 0, 0, 0, 0, -2, 5},
            {10, -2, 0, 0, 0, 0, -2, 10},
            {-20, -50, -2, -2, -2, -2, -50, -20},
            {100, -20, 10, 5, 5, 10, -20, 100}
        };
    
        for (int y = 0; y < BOARD_SIZE; y++) {
            for (int x = 0; x < BOARD_SIZE; x++) {
                if (board.board[y][x] == Black_Disc) blackScore += weight[y][x];
                else if (board.board[y][x] == White_Disc) whiteScore += weight[y][x];
            }
        }
    
        return blackScore - whiteScore;
    }
    
    int Minimax(Board& board, int depth, bool isMaximizingPlayer, int alpha, int beta) {
        if (depth == 0) return EvaluateBoard(board);
    
        int bestScore = isMaximizingPlayer ? INT_MIN : INT_MAX;
        bool moveFound = false;
    
        for (int y = 0; y < BOARD_SIZE; y++) {
            for (int x = 0; x < BOARD_SIZE; x++) {
                if (board.board[y][x] == EMPTY && board.CanPlace(x, y, false)) {
                    moveFound = true;
    
                    Board simulatedBoard = board.Clone();
                    simulatedBoard.PlacePiece(x, y);
    
                    int score = Minimax(simulatedBoard, depth - 1, !isMaximizingPlayer, alpha, beta);
    
                    if (isMaximizingPlayer) {
                        bestScore = std::max(bestScore, score);
                        alpha = std::max(alpha, bestScore);
                    } else {
                        bestScore = std::min(bestScore, score);
                        beta = std::min(beta, bestScore);
                    }
    
                    if (beta <= alpha) break;  // Alpha-Beta Pruning
                }
            }
        }
    
        // If no move found, return evaluation of current board
        if (!moveFound) return EvaluateBoard(board);
        return bestScore;
    }
    
    void MakeMove(Board& board, GameResult& result, bool& gameOver) override {
        int bestScore = INT_MIN;
        int bestX = -1, bestY = -1;
        bool moveFound = false;
    
        for (int y = 0; y < BOARD_SIZE; y++) {
            for (int x = 0; x < BOARD_SIZE; x++) {
                if (board.board[y][x] == EMPTY && board.CanPlace(x, y, false)) {
                    moveFound = true;
    
                    Board simulatedBoard = board.Clone();
                    simulatedBoard.PlacePiece(x, y);
    
                    int score = Minimax(simulatedBoard, 3, false, INT_MIN, INT_MAX);
                    if (score > bestScore) {
                        bestScore = score;
                        bestX = x;
                        bestY = y;
                    }
                }
            }
        }
    
        if (moveFound && bestX != -1 && bestY != -1) {
            board.PlacePiece(bestX, bestY); // Perform the best move
        } else {
            // No move found: could mean game over or AI passes
            std::cout << "AI has no valid moves and passes.\n";
        }
    }
    
    void ShowScore(int blackCount, int whiteCount) override {
        std::cout << "AI sees - Black: " << blackCount << " | White: " << whiteCount << "\n";
    }

    void EndGame(GameResult result) override {
        // Optional: print message or set flags
    }

    void ReturnToMenu(GameState& gameState) override {
        gameState = MENU;
    }
};
  
GameState gameState = MENU;
     
class Game {
    public:
        Board board;
        bool vsAI = false;
        bool gameOver = false;
        GameResult result = NONE;
    
        Player* blackPlayer = nullptr;
        Player* whitePlayer = nullptr;

        int blackCount, whiteCount;

        bool resetGame = false;
    
        Game() : board() {}
        
        void InitPlayers(bool vsAI_mode) {
            vsAI = vsAI_mode;
            delete blackPlayer;
            delete whitePlayer;
    
            blackPlayer = new HumanPlayer();
            whitePlayer = vsAI_mode ? (Player*) new AIPlayer() : (Player*) new HumanPlayer();
        }
    
        void HandleInput() {
            if (gameOver) return;
    
            Player* currentPlayer = (board.currentPlayer == Black_Disc) ? blackPlayer : whitePlayer;
            currentPlayer->MakeMove(board, result, gameOver);
            CheckGameOver();
        }
        
        void Draw() {
            board.DrawBoard();
        
            // Disc counters
            int blackCount = 0, whiteCount = 0;
            for (int y = 0; y < BOARD_SIZE; y++) {
                for (int x = 0; x < BOARD_SIZE; x++) {
                    if (board.board[y][x] == Black_Disc) blackCount++;
                    else if (board.board[y][x] == White_Disc) whiteCount++;
                }
            }

            // Display the score (black and white counts)
            DrawText(TextFormat("Black: %d | White: %d", blackCount, whiteCount), 10, SCREEN_HEIGHT - 30, 20, DARKGREEN);  

            // Turn message in center top
            if (!gameOver) {
                const char* turnMsg = "";
                if (vsAI) {
                    if (board.currentPlayer == Black_Disc)
                        turnMsg = "Your Turn";
                    else
                        turnMsg = "Computer's Turn";
                } else {
                    turnMsg = (board.currentPlayer == Black_Disc) ? "Player 1's Turn" : "Player 2's Turn";
                }
        
                int textWidth = MeasureText(turnMsg, 24);
                DrawText(turnMsg, (GetScreenWidth() - textWidth) / 2, 10, 24, MAROON);
            }
        
            // Game Over screen
            if (gameOver) {
                DrawRectangle(100, 200, 440, 240, Fade(RAYWHITE, 0.9f));
        
                const char* winnerMsg;
                if (result == BLACK_WINS) {
                    winnerMsg = vsAI ? "You Won!" : "Player 1 Won!";
                } else if (result == WHITE_WINS) {
                    winnerMsg = vsAI ? "Computer Won!" : "Player 2 Won!";
                } else {
                    winnerMsg = "It's a Draw!";
                }
        
                DrawText("Game Over", 230, 220, 30, RED);
                DrawText(winnerMsg, 230, 260, 30, DARKGRAY);
        
                if (DrawButton({ 220, 310, 200, 40 }, "Main Menu")) {
                    ResetToMenu(::gameState);
                } else if (DrawButton({ 220, 360, 200, 40 }, "Exit")) {
                    CloseWindow();
                }
            }
        }                             
          
            void CheckGameOver() {
                int blackCount = 0, whiteCount = 0;
                bool blackCanMove = false, whiteCanMove = false;
            
                for (int y = 0; y < BOARD_SIZE; y++) {
                    for (int x = 0; x < BOARD_SIZE; x++) {
                        if (board.board[y][x] == Black_Disc) blackCount++;
                        else if (board.board[y][x] == White_Disc) whiteCount++;
            
                        if (board.board[y][x] == EMPTY) {
                            if (!blackCanMove) {
                                Cell temp = board.currentPlayer;
                                board.currentPlayer = Black_Disc;
                                if (board.CanPlace(x, y, false)) blackCanMove = true;
                                board.currentPlayer = temp;
                            }
            
                            if (!whiteCanMove) {
                                Cell temp = board.currentPlayer;
                                board.currentPlayer = White_Disc;
                                if (board.CanPlace(x, y, false)) whiteCanMove = true;
                                board.currentPlayer = temp;
                            }
                        }
                    }
                }
            
                if (!blackCanMove && !whiteCanMove) {
                    gameOver = true;
                    if (blackCount > whiteCount) result = BLACK_WINS;
                    else if (whiteCount > blackCount) result = WHITE_WINS;
                    else result = DRAW;
                } else if ((board.currentPlayer == Black_Disc && !blackCanMove) ||
                           (board.currentPlayer == White_Disc && !whiteCanMove)) {
                    // Skip turn
                    board.currentPlayer = (board.currentPlayer == Black_Disc) ? White_Disc : Black_Disc;
                }
            }            
    
        void ResetToMenu(GameState& gameState) {
            board = Board();
            gameOver = false;
            result = NONE;
            delete blackPlayer;
            delete whitePlayer;
            blackPlayer = nullptr;
            whitePlayer = nullptr;
            gameState = MENU;
        }
    
        ~Game() {
            delete blackPlayer;
            delete whitePlayer;
        }
    };
         
int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Othello");
    SetTargetFPS(60);

    Game game;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (gameState == MENU) {
            DrawText("OTHELLO", 220, 100, 40, DARKGREEN);

            if (DrawButton({ 250, 200, 150, 50 }, "Play"))
                gameState = MODE_SELECTION;

            if (DrawButton({ 250, 270, 150, 50 }, "Exit"))
                break;

        } else if (gameState == MODE_SELECTION) {
            DrawText("Select Mode", 230, 100, 30, DARKBLUE);

            if (DrawButton({ 200, 180, 240, 50 }, "Two Players")) {
                game.InitPlayers(false);
                gameState = GAMEPLAY;
            }
            
            if (DrawButton({ 200, 250, 240, 50 }, "Player vs Computer")) {
                game.InitPlayers(true);
                gameState = GAMEPLAY;
            }
            
            if (DrawButton({ 200, 320, 240, 50 }, "Back"))
                gameState = MENU;

        } else if (gameState == GAMEPLAY) {
            game.HandleInput();
            game.Draw();
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

