#include <bits/stdc++.h> // Includes most standard libraries
#include <conio.h>       // For _kbhit() and _getch() (console input)
#include <windows.h>     // For Windows-specific console functions (setColor, gotoxy, Sleep, Beep)
#include <fstream>       // For file input/output (high score, starting level)
#include <ctime>         // For time() and clock() (random seed, game time)

using namespace std;

// --- Game Constants ---
#define MAX_LENGTH 1000         // Maximum snake length
#define WALL_CHAR '#'           // Character for drawing walls
#define FOOD_CHAR '*'           // Character for normal food
#define MONEY_CHAR '$'          // Character for money food
#define SNAKE_HEAD_CHAR '\xFE'  // ASCII character for snake head (filled square)
#define SNAKE_BODY_CHAR '\xFE'  // ASCII character for snake body (filled square)

// --- Direction Constants ---
const char DIR_UP = 'U';
const char DIR_DOWN = 'D';
const char DIR_LEFT = 'L';
const char DIR_RIGHT = 'R';

// --- Global Game State Variables ---
int consoleWidth = 70;      // Width of the game area (changed to 70)
int consoleHeight = 30;     // Height of the game area
bool isPaused = false;      // Flag to check if the game is paused
int fruitsToClearLevel = 15; // Number of fruits required to clear a level (CHANGED TO 15)
int totalMoneyCollected = 0; // Total money collected across all levels
int highScore = 0;          // Stores the highest score achieved
int startTime;              // Stores the starting time of the current game session

// --- Console Utility Functions ---

// Sets the console text color
void setColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// Moves the console cursor to a specific (x, y) coordinate
void gotoxy(int x, int y) {
    COORD coord = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

// Hides the blinking console cursor
void hideCursor() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = false; // Set cursor visibility to false
    SetConsoleCursorInfo(out, &cursorInfo);
}

// Calculates game speed based on the current level
// All levels will now have the same speed as Level 1
int getSpeedForLevel(int level) {
    return 250; // Speed for Level 1 (250 - (1-1)*50 = 250)
}

// Plays a simple beep sound for eating food
void playEatSound() {
    Beep(800, 150); // Frequency 800Hz, Duration 150ms
}

// --- Point Structure ---
// Represents a coordinate (x, y) on the console
struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {} // Constructor
    // Overload == operator for easy comparison of two Point objects
    bool operator==(const Point& p) const { return x == p.x && y == p.y; }
};

// --- Snake Class ---
// Manages the snake's body, movement, and direction
class Snake {
public:
    Point body[MAX_LENGTH]; // Array to store snake body segments
    int length;             // Current length of the snake
    char direction;         // Current direction of the snake

    // Constructor: Initializes snake at a starting position with initial length based on level
    Snake(int startX, int startY, int level) {
        length = 3 + (level - 1); // Snake starts longer in higher levels
        if (length > MAX_LENGTH) length = MAX_LENGTH; // Cap length at MAX_LENGTH
        // Initialize snake body segments
        for (int i = 0; i < length; i++)
            body[i] = Point(startX - i, startY); // Snake starts horizontally
        direction = DIR_RIGHT; // Initial direction
    }

    // Changes the snake's direction, preventing immediate 180-degree turns
    void changeDirection(char newDir) {
        if (newDir == DIR_UP && direction != DIR_DOWN) direction = newDir;
        else if (newDir == DIR_DOWN && direction != DIR_UP) direction = newDir;
        else if (newDir == DIR_LEFT && direction != DIR_RIGHT) direction = newDir;
        else if (newDir == DIR_RIGHT && direction != DIR_LEFT) direction = newDir;
    }

    // Moves the snake one step in its current direction
    // Returns true if snake is alive, false if it collides
    bool move(bool &ateFood, Point food, int level) {
        // Shift body segments: each segment takes the position of the one in front of it
        for (int i = length - 1; i > 0; i--)
            body[i] = body[i - 1];

        // Move the head based on current direction
        switch (direction) {
            case DIR_UP: body[0].y--; break;
            case DIR_DOWN: body[0].y++; break;
            case DIR_LEFT: body[0].x--; break;
            case DIR_RIGHT: body[0].x++; break;
        }

        // --- Collision Detection ---

        // Self-collision: Check if head collides with any part of its body
        for (int i = 1; i < length; i++)
            if (body[0] == body[i]) return false;

        // Wall collision: Check if head hits the outer borders
        if (body[0].x <= 0 || body[0].x >= consoleWidth - 1 ||
            body[0].y <= 0 || body[0].y >= consoleHeight - 1)
            return false;

        // Maze Wall Collision for Levels 2 and 3 (vertical lines)
        if (level >= 2 && body[0].x == consoleWidth / 3 && body[0].y >= 3 && body[0].y < consoleHeight - 3) return false;
        if (level >= 3 && body[0].x == 2 * consoleWidth / 3 && body[0].y >= 3 && body[0].y < consoleHeight - 3) return false;

        // Maze Wall Collision for Levels 4-6 (unique patterns)
        if (level >= 4) {
            // Level 4: Central Cross with Gaps
            int center_x = consoleWidth / 2;
            int center_y = consoleHeight / 2;
            int gap_size = 5; // Size of the gap in the cross
            if ((body[0].x == center_x && (body[0].y < center_y - gap_size || body[0].y > center_y + gap_size)) || // Vertical part
                (body[0].y == center_y && (body[0].x < center_x - gap_size || body[0].x > center_x + gap_size))) { // Horizontal part
                return false;
            }
        }
        if (level >= 5) {
            // Level 5: Diagonal Cross (easier - dashed lines)
            int dash_interval = 5; // How often a wall segment appears on the diagonal
            if (((body[0].x == body[0].y || body[0].x + body[0].y == consoleWidth - 1) && (body[0].x % dash_interval == 0 || body[0].x % dash_interval == 1))) {
                return false;
            }
        }
        if (level >= 6) {
            // Level 6: Single Vertical Barrier with Large Opening
            int barrier_x = consoleWidth / 2; // Middle of the screen
            int opening_y_start = consoleHeight / 2 - 8; // Very large opening
            int opening_y_end = consoleHeight / 2 + 8;   // Very large opening

            if (body[0].x == barrier_x && (body[0].y < opening_y_start || body[0].y > opening_y_end)) {
                return false;
            }
        }

        // Food collision: Check if snake head is on the food
        ateFood = (body[0] == food);
        if (ateFood && length < MAX_LENGTH) {
            // If food eaten, increase snake length by adding a new segment at the tail's last position
            body[length] = body[length - 1];
            length++;
        }
        return true; // Snake is alive and moved successfully
    }
};

// --- Board Class ---
// Manages the game board, food, score, and drawing
class Board {
public:
    Snake *snake;           // Pointer to the snake object
    Point food;             // Position of the current food item
    bool isMoneyFruit = false; // True if current food is a money fruit
    int score, fruitCount, level; // Current score, fruits eaten in level, current level
    int bonusMoney;         // (Not directly used, but can be for future features)
    Point lastTail;         // Stores the position of the snake's tail before it moves (for clearing)
    bool speedBoostRequested; // New: Flag to indicate if speed boost is requested

    // Constructor: Initializes board for a given level
    Board(int level) {
        this->level = level;
        int startX = consoleWidth / 2;
        int startY = consoleHeight / 2;

        // Adjust starting position for levels with central obstacles
        // The position (3,3) is chosen as it is generally clear of all current maze designs.
        if (level >= 4) {
            startX = 3; // Start near the top-left corner
            startY = 3; // Start near the top-left corner
        }

        snake = new Snake(startX, startY, level); // Create new snake with adjusted start
        score = 0;
        fruitCount = 0;
        bonusMoney = 0;
        lastTail = snake->body[snake->length - 1]; // Initialize lastTail
        spawnFood(); // Place initial food
        speedBoostRequested = false; // Initialize speed boost flag
    }

    // Destructor: Cleans up dynamically allocated snake object
    ~Board() { delete snake; }

    // Gets a wall color based on the level for visual variety
    int getWallColor() {
        int colors[] = {1, 2, 3, 4, 5, 6, 9, 10, 11, 13}; // Array of console colors
        return colors[(level - 1) % 10]; // Cycle through colors based on level
    }

    // Draws the game borders and level-specific maze patterns
    void drawBorders() {
        setColor(getWallColor()); // Set color for walls

        // Draw outer borders
        for (int x = 0; x < consoleWidth; x++) {
            gotoxy(x, 0); cout << WALL_CHAR; // Top border
            gotoxy(x, consoleHeight - 1); cout << WALL_CHAR; // Bottom border
        }
        for (int y = 0; y < consoleHeight; y++) {
            gotoxy(0, y); cout << WALL_CHAR; // Left border
            gotoxy(consoleWidth - 1, y); cout << WALL_CHAR; // Right border
        }

        // Level-specific maze patterns
        if (level >= 2) {
            // Level 2: First vertical wall
            for (int y = 3; y < consoleHeight - 3; y++) {
                gotoxy(consoleWidth / 3, y); cout << WALL_CHAR;
            }
        }
        if (level >= 3) {
            // Level 3: Second vertical wall
            for (int y = 3; y < consoleHeight - 3; y++) {
                gotoxy(2 * consoleWidth / 3, y); cout << WALL_CHAR;
            }
        }
        if (level >= 4) {
            // Level 4: Central Cross with Gaps
            int center_x = consoleWidth / 2;
            int center_y = consoleHeight / 2;
            int gap_size = 5; // Size of the gap in the cross
            for (int x = 1; x < consoleWidth - 1; x++) {
                if (x < center_x - gap_size || x > center_x + gap_size) {
                    gotoxy(x, center_y); cout << WALL_CHAR;
                }
            }
            for (int y = 1; y < consoleHeight - 1; y++) {
                if (y < center_y - gap_size || y > center_y + gap_size) {
                    gotoxy(center_x, y); cout << WALL_CHAR;
                }
            }
        }
        if (level >= 5) {
            // Level 5: Diagonal Cross (easier - dashed lines)
            int dash_interval = 5; // How often a wall segment appears on the diagonal
            for (int i = 1; i < consoleWidth - 1; i++) {
                if (i < consoleHeight - 1) {
                    if (i % dash_interval == 0 || i % dash_interval == 1) { // Draw two characters for a dash
                        gotoxy(i, i); cout << WALL_CHAR; // Top-left to bottom-right
                    }
                }
                if (consoleWidth - 1 - i > 0 && i < consoleHeight - 1) {
                    if (i % dash_interval == 0 || i % dash_interval == 1) { // Draw two characters for a dash
                        gotoxy(consoleWidth - 1 - i, i); cout << WALL_CHAR; // Top-right to bottom-left
                    }
                }
            }
        }
        if (level >= 6) {
            // Level 6: Single Vertical Barrier with Large Opening
            int barrier_x = consoleWidth / 2; // Middle of the screen
            int opening_y_start = consoleHeight / 2 - 8; // Very large opening
            int opening_y_end = consoleHeight / 2 + 8;   // Very large opening

            for (int y = 1; y < consoleHeight - 1; y++) {
                if (y < opening_y_start || y > opening_y_end) {
                    gotoxy(barrier_x, y); cout << WALL_CHAR;
                }
            }
        }
        // Levels 7 through 10 have been removed.

        setColor(7); // Reset color to default white
    }

    // Spawns a new food item at a random valid location
    void spawnFood() {
        while (true) {
            int x = 1 + rand() % (consoleWidth - 2); // Random X within borders
            int y = 1 + rand() % (consoleHeight - 2); // Random Y within borders

            // Check if food spawns on any wall (outer or maze walls)
            // This logic mirrors the collision detection in Snake::move()
            bool collisionWithWall = false;

            // Levels 2 and 3 walls
            if ((level >= 2 && x == consoleWidth / 3 && y >= 3 && y < consoleHeight - 3) ||
                (level >= 3 && x == 2 * consoleWidth / 3 && y >= 3 && y < consoleHeight - 3)) {
                collisionWithWall = true;
            }

            // Maze Wall Collision for Levels 4-6 (unique patterns)
            if (!collisionWithWall && level >= 4) {
                // Level 4: Central Cross with Gaps
                int center_x = consoleWidth / 2;
                int center_y = consoleHeight / 2;
                int gap_size = 5;
                if ((x == center_x && (y < center_y - gap_size || y > center_y + gap_size)) ||
                    (y == center_y && (x < center_x - gap_size || x > center_x + gap_size))) {
                    collisionWithWall = true;
                }
            }
            if (!collisionWithWall && level >= 5) {
                // Level 5: Diagonal Cross (easier - dashed lines)
                int dash_interval = 5;
                if (((x == y || x + y == consoleWidth - 1) && (x % dash_interval == 0 || x % dash_interval == 1))) {
                    collisionWithWall = true;
                }
            }
            if (!collisionWithWall && level >= 6) {
                // Level 6: Single Vertical Barrier with Large Opening
                int barrier_x = consoleWidth / 2;
                int opening_y_start = consoleHeight / 2 - 8;
                int opening_y_end = consoleHeight / 2 + 8;

                if (x == barrier_x && (y < opening_y_start || y > opening_y_end)) {
                    collisionWithWall = true;
                }
            }
            // Levels 7 through 10 have been removed.

            if (collisionWithWall) continue; // If collision, try new coordinates

            // Check if food spawns on the snake's body
            bool onSnake = false;
            for (int i = 0; i < snake->length; i++) {
                if (snake->body[i].x == x && snake->body[i].y == y) {
                    onSnake = true;
                    break;
                }
            }
            if (!onSnake) {
                food = Point(x, y); // Valid position found
                isMoneyFruit = ((fruitCount + 1) % 5 == 0); // Every 5th fruit is money fruit
                break;
            }
        }
    }

    // Draws the food item on the console
    void drawFood() {
        gotoxy(food.x, food.y);
        setColor(isMoneyFruit ? 14 : 10); // Yellow for money, green for normal
        cout << (isMoneyFruit ? MONEY_CHAR : FOOD_CHAR);
        setColor(7); // Reset color
    }

    // Draws the snake on the console, clearing its old tail position
    void drawSnake() {
        gotoxy(lastTail.x, lastTail.y);
        cout << ' '; // Clear the old tail position
        gotoxy(snake->body[0].x, snake->body[0].y);
        setColor(10); // Green for head
        cout << SNAKE_HEAD_CHAR;
        if (snake->length > 1) {
            gotoxy(snake->body[1].x, snake->body[1].y);
            setColor(12); // Red for body
            cout << SNAKE_BODY_CHAR;
        }
        setColor(7); // Reset color
        lastTail = snake->body[snake->length - 1]; // Update lastTail for next frame
    }

    // Draws the score, level, fruit count, money, high score, and time on the side
    void drawScore() {
        // Adjusted column for UI elements due to increased console width
        gotoxy(consoleWidth + 2, 2); setColor(11); // Cyan color
        cout << "Level: " << level << "   "; // Added spaces to clear previous longer numbers
        gotoxy(consoleWidth + 2, 3); cout << "Score: " << score << "   ";
        gotoxy(consoleWidth + 2, 4); cout << "Fruits: " << fruitCount << "/" << fruitsToClearLevel << "   ";
        gotoxy(consoleWidth + 2, 5); cout << "Money: $" << totalMoneyCollected << "   ";
        gotoxy(consoleWidth + 2, 6); cout << "High Score: " << highScore << "   ";
        gotoxy(consoleWidth + 2, 7); cout << "Time: " << (clock() - startTime) / CLOCKS_PER_SEC << "s   ";
        setColor(7); // Reset color
    }

    // Updates the game state (snake movement, food eating, level progression)
    // Returns -1 for game over, 1 for level clear, 0 for ongoing
    int update() {
        bool ateFood = false;
        bool alive = snake->move(ateFood, food, level); // Move snake and check for collisions

        if (!alive) return -1; // Game Over if snake is not alive

        if (ateFood) {
            score += isMoneyFruit ? 3 : 1; // Score based on fruit type
            fruitCount++; // Increment fruit count for current level
            if (isMoneyFruit) {
                totalMoneyCollected += 100; // Add bonus money
                gotoxy(10, consoleHeight);
                setColor(14); // Yellow color
                cout << "  Bonus Collected! +$100  "; // Display bonus message
                setColor(7);
                Sleep(1000); // Pause briefly to show message
            }
            spawnFood(); // Place new food
            playEatSound(); // Play sound
        }

        drawSnake(); // Redraw snake
        drawFood();  // Redraw food
        drawScore(); // Update score display

        if (fruitCount >= fruitsToClearLevel) {
            // Level cleared logic
            gotoxy(10, consoleHeight);
            setColor(14);
            cout << " 🎉 Level Cleared!    ";
            setColor(7);
            Sleep(1500);
            return 1; // Indicate level cleared
        }
        return 0; // Game is ongoing
    }

    // Handles user input for snake direction and pause/resume
    void getInput() {
        speedBoostRequested = false; // Reset boost flag at the start of each input check
        if (_kbhit()) { // Check if a key has been pressed
            int key = _getch(); // Get the pressed key
            if (key == 224) key = _getch(); // Handle arrow keys (they send two scan codes)

            // Change snake direction based on input
            if (key == 72 || key == 'w' || key == 'W') snake->changeDirection(DIR_UP);
            else if (key == 80 || key == 's' || key == 'S') snake->changeDirection(DIR_DOWN);
            else if (key == 75 || key == 'a' || key == 'A') { // Left Arrow or 'a'
                snake->changeDirection(DIR_LEFT);
                speedBoostRequested = true; // Request speed boost
            }
            else if (key == 77 || key == 'd' || key == 'D') { // Right Arrow or 'd'
                snake->changeDirection(DIR_RIGHT);
                speedBoostRequested = true; // Request speed boost
            }
            else if (key == 'p' || key == 'P') {
                isPaused = !isPaused; // Toggle pause state
                if (!isPaused) {
                    gotoxy(10, 10);
                    cout << "                                  "; // Clear pause message when unpaused
                }
            }
        }
    }
};

// --- Game Management Functions ---

// Displays game instructions and prompts for starting level
void showInstructions() {
    system("cls"); // Clear console screen
    setColor(11); // Cyan color
    cout << "=============== Snake Game ===============\n";
    cout << "Use W A S D or Arrow Keys to move the snake.\n";
    cout << "Hold LEFT/RIGHT arrow or 'A'/'D' for 2x speed!\n"; // Added instruction
    cout << "* = Normal fruit, $ = Money fruit (+$100)\n";
    cout << "Eat 15 fruits to complete a level.\n"; // Updated instruction
    cout << "Walls appear from level 2 onwards.\n";
    cout << "Press 'P' to pause/resume.\n";
    cout << "==========================================\n";
    setColor(14); // Yellow color
    cout << "Enter Starting Level (1-6): "; // Changed input range to 1-6
    int level;
    cin >> level; // Get user input for starting level
    // Validate input, default to 1 if invalid
    if (level < 1 || level > 6) level = 1; // Changed validation to 1-6
    // Save starting level to a file (for persistence across runs)
    ofstream f("level.txt");
    if (f.is_open()) {
        f << level;
        f.close();
    }
    setColor(7); // Reset color
    cout << "Press any key to start...\n";
    _getch(); // Wait for any key press
}

// Loads the high score from a file
void loadHighScore() {
    ifstream f("highscore.txt");
    if (f.is_open()) {
        f >> highScore; // Read high score if file exists
        f.close();
    }
}

// Saves the current high score to a file
void saveHighScore() {
    ofstream f("highscore.txt");
    if (f.is_open()) {
        f << highScore; // Write high score
        f.close();
    }
}

// --- Main Game Loop ---
int main() {
    srand(time(0)); // Seed random number generator with current time
    hideCursor();   // Hide the console cursor for a cleaner look

    showInstructions(); // Display instructions and get starting level

    int currentLevel = 1;
    // Load starting level from file if it exists (persists last played level)
    ifstream f_level("level.txt");
    if (f_level.is_open()) {
        f_level >> currentLevel;
        f_level.close();
    }

    loadHighScore(); // Load the high score

    startTime = clock(); // Record the start time of the game

    // Main game loop: continues as long as the user doesn't exit
    while (true) {
        int base_speed = getSpeedForLevel(currentLevel); // Get base speed (always 250)
        Board* board = new Board(currentLevel);     // Create a new game board for the level

        system("cls"); // Clear the console for the new level
        board->drawBorders(); // Draw the borders and maze for the current level

        // Inner game loop: runs until game over or level cleared
        while (true) {
            board->getInput(); // Process user input and set speedBoostRequested flag

            if (!isPaused) {
                int effective_speed = base_speed;
                if (board->speedBoostRequested) {
                    effective_speed = base_speed / 2; // Double speed (half sleep time)
                    if (effective_speed < 50) effective_speed = 50; // Ensure a minimum speed (e.g., 50ms)
                }

                int state = board->update(); // Update game state

                if (state == -1) { // Game Over
                    system("cls");
                    gotoxy(10, 10); setColor(12); cout << "💀 GAME OVER!         "; // Red color
                    gotoxy(10, 11); cout << "Total Money Earned: $" << totalMoneyCollected << "      ";
                    gotoxy(10, 12); cout << "Restarting from Level 1...";
                    setColor(7);
                    Sleep(3000); // Pause for 3 seconds
                    currentLevel = 1; // Reset to level 1
                    totalMoneyCollected = 0; // Reset money
                    startTime = clock(); // Reset game timer
                    break; // Exit inner loop to start new game
                } else if (state == 1) { // Level Cleared
                    gotoxy(10, 10); setColor(10); cout << "✅ LEVEL " << currentLevel << " CLEARED!     "; // Green color
                    setColor(7); Sleep(1500);
                    currentLevel++; // Advance to next level
                    if (currentLevel > 6) { // Changed max level to 6
                        // All levels completed
                        system("cls"); // Clear screen for final message
                        gotoxy(consoleWidth / 2 - 15, consoleHeight / 2 - 2); setColor(11); cout << "🏆 CONGRATULATIONS! All levels complete.";
                        gotoxy(consoleWidth / 2 - 15, consoleHeight / 2 - 1); cout << "💰 Total Money Earned: $" << totalMoneyCollected;
                        setColor(7); Sleep(4000); // Pause for 4 seconds
                        if (totalMoneyCollected > highScore) { // Update high score if current money is higher
                            highScore = totalMoneyCollected;
                            saveHighScore();
                        }
                        return 0; // Exit program if all levels are done
                    }
                    break; // Exit inner loop to start next level
                }
                Sleep(effective_speed); // Use the calculated effective speed
            } else {
                // Game is paused
                gotoxy(10, 10); setColor(14);
                cout << "= PAUSED == Press 'P' to resume =";
                setColor(7);
                Sleep(100); // Short sleep to prevent busy-waiting
            }
        }
        delete board; // Clean up board object before starting a new level/game
    }

    return 0; // Program exits
}
