// Include necessary libraries
#include "raylib.h"     // For graphics and input handling
#include <iostream>     // For console output
#include <climits>      // For INT_MIN/MAX constants
#include <fstream>      // For file handling
#include <ctime>        // For date/time functions
#include <stdexcept>    // For standard exceptions
using namespace std;

const int SCREEN_WIDTH = 640;   // Window width
const int SCREEN_HEIGHT = 640;  // Window height
const int BOARD_SIZE = 8;       // 8x8 Othello board
const int CELL_SIZE = SCREEN_WIDTH / BOARD_SIZE;    // Size of each cell

// Game enumerations
enum Cell { EMPTY, Black_Disc, White_Disc };    // Possible cell states
enum GameState { MENU, MODE_SELECTION, GAMEPLAY, SCORE_HISTORY };  // Game screens
enum GameResult { NONE, BLACK_WINS, WHITE_WINS, DRAW }; // Game outcomes

// Utility for drawing button
static bool DrawButton(Rectangle bounds, const char* text) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    DrawRectangleRec(bounds, hovered ? GRAY : LIGHTGRAY);
    DrawText(text, bounds.x + 10, bounds.y + 10, 20, BLACK);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static bool DrawSmallButton(Rectangle bounds, const char* text, int fontSize = 20) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    DrawRectangleRec(bounds, hovered ? GRAY : LIGHTGRAY);
    DrawText(text, bounds.x + 5, bounds.y + 5, fontSize, BLACK);
    return hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

class Game; //Forward Declaration

// Board class - represents the Othello game board
class Board {
    private:
        float flipProgress[8][8] = {0};     // Animation progress for each cell
        Sound flipSound;                    // Sound reference
    public:
        Cell board[BOARD_SIZE][BOARD_SIZE];     // 2D array representing the board
        Cell currentPlayer;                     // Current player (black or white)
        bool validMoves[8][8];                  // Track valid moves for highlighting

        // Constructor - initialize board and starting player
        Board(){
            currentPlayer = Black_Disc;
            Initialize_Board();
        }

        // Calculate all valid moves for a player
        void ComputeValidMoves(Cell player)
        {
            for (int row = 0; row < 8; ++row) {
                for (int col = 0; col < 8; ++col) {
                    validMoves[row][col] = IsValidMove(row, col, player);
                }
            }
        }

        // Check if a move is valid for a specific player
        bool IsValidMove(int row, int col, Cell player)
        {
                if (board[row][col] != EMPTY) return false;     // Can't place on occupied cell

                // Temporarily set to check moves for this player
                Cell tempPlayer = currentPlayer;
                currentPlayer = player;  
                bool isValid = CanPlace(col, row, false);   // Check without flipping
                currentPlayer = tempPlayer;  // Restore original player
                
                return isValid;
        }

        // Initialize the board with starting positions
        void Initialize_Board() {
            // Clear the board
            for (int row = 0; row < BOARD_SIZE; row++)
                for (int col = 0; col < BOARD_SIZE; col++)
                    board[row][col] = EMPTY;
            
            // Set up the initial 4 pieces in the center
            board[3][3] = White_Disc;
            board[3][4] = Black_Disc;
            board[4][3] = Black_Disc;
            board[4][4] = White_Disc;
        }
        
        // Check if coordinates are within board boundaries
        bool Is_Within_Boundaries(int x, int y) {
            return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
        }
        void UpdateAnimations() {
            for(int y = 0; y < 8; y++) {
                for(int x = 0; x < 8; x++) {
                    if(flipProgress[y][x] > 0) {
                        flipProgress[y][x] -= GetFrameTime() * 4;
                        if(flipProgress[y][x] < 0) flipProgress[y][x] = 0;
                    }
                }
            }
        }
        // Try to flip pieces in a direction (dx, dy)
        bool TryFlip(int x, int y, int dx, int dy, bool flip) {
            int cx = x + dx;
            int cy = y + dy;
            int count = 0;
            
            // Move in direction until we find our own piece or edge
            while (Is_Within_Boundaries(cx, cy) && board[cy][cx] == (currentPlayer == Black_Disc ? White_Disc : Black_Disc)) {
                cx += dx;
                cy += dy;
                count++;
            }
            
            // If we found our own piece after opponent's pieces, it's valid
            if (count > 0 && Is_Within_Boundaries(cx, cy) && board[cy][cx] == currentPlayer) {
                if (flip) {     // Actually flip the pieces if requested
                    for (int i = 1; i <= count; i++){
                        board[y + i * dy][x + i * dx] = currentPlayer;
                        flipProgress[y + i * dy][x + i * dx] = 1.0f; // Start animation
                    }
                }
                return true;
            }
    
            return false;
        }
        
        // Check if a piece can be placed at (x,y)
        bool CanPlace(int x, int y, bool flip) {
            if (board[y][x] != EMPTY) return false; // Cell must be empty
    
            bool valid = false;
            // Check all 8 directions around the cell
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    if (!(dx == 0 && dy == 0))  // Skip the current cell
                        valid |= TryFlip(x, y, dx, dy, flip);
    
            return valid;
        }
        
        // Place a piece on the board if valid
        void PlacePiece(int x, int y) {
            if (CanPlace(x, y, true)) { // Check and flip pieces
                board[y][x] = currentPlayer;
                currentPlayer = (currentPlayer == Black_Disc) ? White_Disc : Black_Disc;
            }
        }

        // Draw the game board
        void DrawBoard(bool showHighlights) {
            // Color definitions
            Color Board_Background_Color = DARKGREEN;
            Color Grid_Line_Color = BLACK;
            Color Black_Disc_Color = BLACK;
            Color White_Disc_Color = WHITE;
            Color Highlight_Color = RED; // Color for valid move highlights

            ClearBackground(Board_Background_Color);

            // Draw each cell
            for (int y = 0; y < BOARD_SIZE; y++) {
                for (int x = 0; x < BOARD_SIZE; x++) {
                    // Draw grid lines
                    DrawRectangleLines(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE, Grid_Line_Color);

                    // Highlights
                    if(showHighlights){
                        Vector2 mousePos = GetMousePosition();
                        int hoverX = mousePos.x / CELL_SIZE;
                        int hoverY = mousePos.y / CELL_SIZE;

                        if (Is_Within_Boundaries(hoverX, hoverY)) {
                            if (board[hoverY][hoverX] == EMPTY && IsValidMove(hoverY, hoverX, currentPlayer)) {
                                DrawRectangle(hoverX * CELL_SIZE, hoverY * CELL_SIZE, 
                                            CELL_SIZE, CELL_SIZE, Fade(LIGHTGRAY, 0.2f));
                            }
                        }
                    }
                    // Draw disc if present
                    if (board[y][x] != EMPTY) {
                        Color Disc_Color = (board[y][x] == Black_Disc) ? Black_Disc_Color : White_Disc_Color;
                        float scale = 1.0f - flipProgress[y][x];
                        float radius = (CELL_SIZE / 2 - 5) * scale;
                        DrawCircle(x * CELL_SIZE + CELL_SIZE / 2, 
                                y * CELL_SIZE + CELL_SIZE / 2, 
                                radius, Disc_Color);
                    }

                    // Highlight valid moves for current player
                    else if (IsValidMove(y, x, currentPlayer)) {
                        DrawCircle(x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2, 7, Highlight_Color);
                    }
                }
            }
        }

        // Create a copy of the board
        Board Clone() {
            Board copy;
            for (int y = 0; y < BOARD_SIZE; y++)
                for (int x = 0; x < BOARD_SIZE; x++)
                    copy.board[y][x] = board[y][x];
            copy.currentPlayer = currentPlayer;
            return copy;
        }

        // Check if a player has any valid moves
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

// Human player implementation
class Player {
    public:
        virtual void MakeMove(Board& board, GameResult& result, bool& gameOver) = 0;
        virtual void ShowScore(int blackCount, int whiteCount) = 0;
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
        cout << "Black: " << blackCount << " | White: " << whiteCount << "\n";
    }

    void ReturnToMenu(GameState& gameState) override {
        gameState = MENU;
    }
};

// AI player implementation
class AIPlayer : public Player {
    public:
        // Simplified evaluation: prioritize corners and discourage edges
        int EvaluateBoard(Board& board) {
            int score = 0;
            const int weight[8][8] = {
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
                    if (board.board[y][x] == Black_Disc) score += weight[y][x];
                    else if (board.board[y][x] == White_Disc) score -= weight[y][x];
                }
            }
            return score;
        }

        int Minimax(Board& board, int depth, bool isMax, int alpha, int beta) {
            if (depth == 0) return EvaluateBoard(board);

            int bestScore = isMax ? INT_MIN : INT_MAX;
            bool moveAvailable = false;

            for (int y = 0; y < BOARD_SIZE; y++) {
                for (int x = 0; x < BOARD_SIZE; x++) {
                    if (board.board[y][x] == EMPTY && board.CanPlace(x, y, isMax)) {
                        moveAvailable = true;
                        Board newBoard = board.Clone();
                        newBoard.PlacePiece(x, y);
                        int score = Minimax(newBoard, depth - 1, !isMax, alpha, beta);

                        if (isMax) {
                            bestScore =  max(bestScore, score);
                            alpha =  max(alpha, bestScore);
                        } else {
                            bestScore =  min(bestScore, score);
                            beta =  min(beta, bestScore);
                        }

                        if (beta <= alpha) break;
                    }
                }
            }
            return moveAvailable ? bestScore : EvaluateBoard(board);
        }

        void MakeMove(Board& board, GameResult& result, bool& gameOver) override {
            int bestScore = INT_MIN;
            int moveX = -1, moveY = -1;

            for (int y = 0; y < BOARD_SIZE; y++) {
                for (int x = 0; x < BOARD_SIZE; x++) {
                    if (board.board[y][x] == EMPTY && board.CanPlace(x, y, false)) {
                        Board newBoard = board.Clone();
                        newBoard.PlacePiece(x, y);
                        int score = Minimax(newBoard, 2, true, INT_MIN, INT_MAX); // Depth = 2 for speed

                        if (score > bestScore) {
                            bestScore = score;
                            moveX = x;
                            moveY = y;
                        }
                    }
                }
            }

            if (moveX != -1 && moveY != -1) {
                board.PlacePiece(moveX, moveY);
            } else {
                cout << "AI has no valid moves. Passing...\n";
            }
        }

        void ShowScore(int blackCount, int whiteCount) override {
            cout << "AI Score - Black: " << blackCount << " | White: " << whiteCount << "\n";
        }

        void ReturnToMenu(GameState& gameState) override {
            gameState = MENU;
        }
};


// Global game state
GameState gameState = MENU;
     
class Game {

    private:
        Sound placeSound;
        Sound flipSound;
        Sound gameOverSound;

        void SaveScore(int blackCount, int whiteCount) {
            try {
                ofstream file("scores.txt", ios::app); // Append mode
                if (!file) throw  runtime_error("Failed to open scores file");

                // Get current timestamp
                time_t now = time(nullptr);
                if (now == -1) throw runtime_error("Failed to get time");
                    tm* tm =  localtime(&now);
                if (!tm) throw runtime_error("Failed to convert time");
                
                char timeStr[80];
                strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm);

                // Write score entry
                file << "[" << timeStr << "] Black: " << blackCount 
                        << " | White: " << whiteCount << " | Winner: ";
                        
                switch (result) {
                    case BLACK_WINS: file << "Black"; break;
                    case WHITE_WINS: file << "White"; break;
                    case DRAW:       file << "Draw";  break;
                    default:         file << "None";  break;
                }
                file << "\n";

            } catch (const  exception& e) {
                    cerr << "Score save error: " << e.what() << "\n";
            }
        }

    public:
        Board board;                    // Game board
        bool vsAI = false;              // Playing against AI?
        bool gameOver = false;          // Is game over?
        GameResult result = NONE;       //Game Result
    
        Player* blackPlayer = nullptr;  // Player 1 (Black)
        Player* whitePlayer = nullptr;  // Player 2 or AI (White)

        double aiStartTime = 0;     // Timer for AI moves
        bool aiThinking = false;    // Is AI thinking?
        const double aiDelay = 3.0; // Delay before AI moves (seconds)

        // Constructor
        Game() : board() {}
        
        // Initialize players based on game mode
        void InitPlayers(bool vsAI_mode) {
            vsAI = vsAI_mode;
            delete blackPlayer;
            delete whitePlayer;
    
            blackPlayer = new HumanPlayer();
            whitePlayer = vsAI_mode ? (Player*) new AIPlayer() : (Player*) new HumanPlayer();
        }
    
        // Handle player input
        void HandleInput() {
            if (gameOver) return;

            Player* currentPlayer = (board.currentPlayer == Black_Disc) ? blackPlayer : whitePlayer;

            // AI turn handling
            if (vsAI && board.currentPlayer == White_Disc) {
                if (!aiThinking) {
                    aiThinking = true;
                    aiStartTime = GetTime();  // Start AI delay timer
                } else if (GetTime() - aiStartTime >= aiDelay) {
                    currentPlayer->MakeMove(board, result, gameOver);
                    aiThinking = false; // Reset the flag after the move
                    CheckGameOver();
                }
            }
            // Human turn handling
            else {
                currentPlayer->MakeMove(board, result, gameOver);
                CheckGameOver();
            }
        }
        
        // Draw the game
        void Draw() {
            board.UpdateAnimations();
            // Determine if we should show highlights
            bool showHighlights = true;
            if (vsAI) {
                // Only show highlights during human player's turn (Black)
                showHighlights = (board.currentPlayer == Black_Disc);
            }
            board.DrawBoard(showHighlights);

                // Navigation buttons
                if (DrawSmallButton({ 10, 10, 150, 30 }, "Back to Menu")) {
                    ResetToMenu(::gameState);
                    return;  // Skip the rest of drawing if we're returning to menu
                }
                    
                if (DrawSmallButton({SCREEN_WIDTH - 110, 10, 100, 30 }, "Restart")) {
                    ResetGame();
                    return;
                }

            // Disc counters
            int blackCount = 0, whiteCount = 0;
            for (int y = 0; y < BOARD_SIZE; y++) {
                for (int x = 0; x < BOARD_SIZE; x++) {
                    if (board.board[y][x] == Black_Disc) blackCount++;
                    else if (board.board[y][x] == White_Disc) whiteCount++;
                }
            }

            // Display the score (black and white counts)
            DrawText(TextFormat("Black: %d | White: %d", blackCount, whiteCount), 10, SCREEN_HEIGHT - 30, 20, WHITE);  

            // Display whose turn it is
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
                DrawText(turnMsg, (GetScreenWidth() - textWidth) / 2, 10, 24, WHITE);
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
        // Check if game should end
        void CheckGameOver() {
            int blackCount = 0, whiteCount = 0;
            bool blackCanMove = false, whiteCanMove = false;
        
            // Count pieces and check possible moves
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
            
            // Determine game outcome
            if (!blackCanMove && !whiteCanMove) {
                gameOver = true;
                if (blackCount > whiteCount) result = BLACK_WINS;
                else if (whiteCount > blackCount) result = WHITE_WINS;
                else result = DRAW;
                SaveScore(blackCount, whiteCount);
            }
            // Skip turn if current player can't move
            else if ((board.currentPlayer == Black_Disc && !blackCanMove) ||
                        (board.currentPlayer == White_Disc && !whiteCanMove)) {
                // Skip turn
                board.currentPlayer = (board.currentPlayer == Black_Disc) ? White_Disc : Black_Disc;
            }
        }
                    
        // Reset to main menu
        void ResetToMenu(GameState& gameState) {
            board = Board();
            gameOver = false;
            result = NONE;
            delete blackPlayer;
            delete whitePlayer;
            blackPlayer = nullptr;
            whitePlayer = nullptr;
            gameState = MENU;
            aiThinking = false;
        }

        // Reset game while keeping mode
        void ResetGame() {
            board = Board();  // Create fresh board
            gameOver = false;
            result = NONE;
            aiThinking = false;
            
            // Keep the same game mode (vsAI or two players)
            bool currentMode = vsAI;
            delete blackPlayer;
            delete whitePlayer;
            blackPlayer = new HumanPlayer();
            whitePlayer = currentMode ? (Player*) new AIPlayer() : (Player*) new HumanPlayer();
        }

        // Destructor
        ~Game() {
            delete blackPlayer;
            delete whitePlayer;
        }
    };

// Main game loop
int main() {

    // Initialize window
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Othello");
    SetTargetFPS(60);
    
    Game game;

    while (!WindowShouldClose()) 
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // State machine for different screens
        if (gameState == MENU) 
        {
            // Main menu screen
            DrawText("OTHELLO", 220, 100, 40, DARKGREEN);
            DrawText("A Minute To Learn... A Lifetime To Master!", 100, 150, 20, BLACK);

            if (DrawButton({ 250, 200, 150, 50 }, "Play"))
                gameState = MODE_SELECTION;

            if (DrawButton({ 250, 270, 150, 50 }, "Scores")) 
                gameState = SCORE_HISTORY;

            if (DrawButton({ 250, 340, 150, 50 }, "Exit"))
                break;

        } 
        else if (gameState == MODE_SELECTION) 
        {
            // Mode selection screen
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
        } 
        else if (gameState == SCORE_HISTORY) {
            DrawRectangle(50, 50, 540, 540, Fade(RAYWHITE, 0.9f));
            DrawText("Score History", 220, 70, 30, DARKGREEN);

            std::ifstream file("scores.txt");
            int yPos = 120;
            char line[256];
            
            if (!file) {
                DrawText("No scores recorded yet!", 200, 200, 20, DARKGRAY);
            } else {
                while (file.getline(line, sizeof(line))) {
                    if (yPos > SCREEN_HEIGHT - 50) break;
                    DrawText(line, 60, yPos, 20, DARKGRAY);
                    yPos += 30;
                }
            }
            
            if (DrawButton({ 250, SCREEN_HEIGHT - 80, 150, 50 }, "Back")) {
                gameState = MENU;
            }
        }
        else if (gameState == GAMEPLAY) 
        {
            // Actual gameplay
            game.HandleInput();
            game.Draw();
        }
        EndDrawing();
    }

    // Clean up
    CloseWindow();
    return 0;
}
