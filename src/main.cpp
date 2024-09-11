#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <queue>
#include <vector>
#include <cmath>

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Reset pin # (or -1 if sharing Arduino reset pin)

// Initialize the OLED display with I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Game grid dimensions (snake grid is scaled down for the 128x64 OLED screen)
#define GRID_WIDTH 16
#define GRID_HEIGHT 8

// Directions (up, down, left, right)
enum Direction { UP, DOWN, LEFT, RIGHT };
const int DIRECTIONS[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};  // Up, Down, Left, Right

// Function Prototypes
void newApple();
bool isValidPos(int x, int y);

// Snake properties
struct SnakeSegment {
  int x, y;
};

SnakeSegment snake[100];  // Snake's position with max 100 segments
int snake_length = 2;  // Initial snake length
Direction direction = RIGHT;  // Initial direction (right)
int apple[2] = {5, 5};  // Apple position

// Game state
int score = 0;
int level = 1;
float speed = 500;  // Initial speed (milliseconds between moves)
const float MIN_SPEED = 50;  // Minimum speed limit
unsigned long lastMove = 0;
bool gameOver = false;
unsigned long gameOverTime = 0;  // Time when game over occurred

// A* Node structure
struct Node {
  int x, y;
  int g, h, f;
  Node* parent;
  
  Node(int x, int y, int g, int h, Node* parent = nullptr) 
    : x(x), y(y), g(g), h(h), f(g + h), parent(parent) {}
  
  bool operator>(const Node& other) const {
    return f > other.f;
  }
};

// Function to calculate the heuristic (Manhattan distance)
int heuristic(int x1, int y1, int x2, int y2) {
  return abs(x1 - x2) + abs(y1 - y2);
}

// Function to find the path using A* algorithm
bool findPathAStar(int startX, int startY, int destX, int destY, std::vector<Node>& path) {
  std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;
  bool closedList[GRID_WIDTH][GRID_HEIGHT] = {false};
  
  openList.emplace(startX, startY, 0, heuristic(startX, startY, destX, destY));
  
  while (!openList.empty()) {
    Node current = openList.top();
    openList.pop();
    
    if (current.x == destX && current.y == destY) {
      // Reconstruct path
      Node* node = &current;
      while (node) {
        path.push_back(*node);
        node = node->parent;
      }
      return true;
    }
    
    closedList[current.x][current.y] = true;
    
    for (int i = 0; i < 4; i++) {
      int newX = current.x + DIRECTIONS[i][0];
      int newY = current.y + DIRECTIONS[i][1];
      
      if (newX >= 0 && newX < GRID_WIDTH && newY >= 0 && newY < GRID_HEIGHT && !closedList[newX][newY] && isValidPos(newX, newY)) {
        int g = current.g + 1;
        int h = heuristic(newX, newY, destX, destY);
        openList.emplace(newX, newY, g, h, new Node(current));
      }
    }
  }
  
  return false;  // No path found
}

// Function to reset the game state
void resetGame() {
  snake_length = 2;  // Reset snake length
  direction = RIGHT;  // Reset direction to right
  score = 0;
  level = 1;
  speed = 500;  // Reset speed
  
  // Reset snake position
  snake[0] = {3, 3};
  snake[1] = {3, 2};
  
  // Generate a new apple
  newApple();
  
  // Reset the game over state
  gameOver = false;
}

// Draw the grid (snake, apple, obstacles)
void drawGrid() {
  display.clearDisplay();
  
  // Draw snake
  for (int i = 0; i < snake_length; i++) {
    display.fillRect(snake[i].x * 8, snake[i].y * 8, 4, 4, WHITE);  // Each snake segment is 4x4 pixels
  }

  // Draw apple
  display.fillRect(apple[0] * 8, apple[1] * 8, 4, 4, WHITE);
  
  // Display score and level
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(score);

  display.display();
}

// Check if a position is valid (not occupied by snake or obstacles)
bool isValidPos(int x, int y) {
  if (x < 0 || x >= GRID_WIDTH || y < 0 || y >= GRID_HEIGHT) return false;  // Out of bounds
  for (int i = 0; i < snake_length; i++) {
    if (snake[i].x == x && snake[i].y == y) return false;  // Snake collision
  }
  return true;
}

// Generate a new apple position
void newApple() {
  do {
    apple[0] = random(0, GRID_WIDTH);
    apple[1] = random(0, GRID_HEIGHT);
  } while (!isValidPos(apple[0], apple[1]));
}

// Move the snake based on the current direction
void moveSnake() {
  std::vector<Node> path;
  if (findPathAStar(snake[0].x, snake[0].y, apple[0], apple[1], path)) {
    Node nextMove = path.back();
    path.pop_back();
    if (!path.empty()) {
      nextMove = path.back();
    }
    int dx = nextMove.x - snake[0].x;
    int dy = nextMove.y - snake[0].y;
    if (dx == 1 && dy == 0) direction = RIGHT;
    else if (dx == -1 && dy == 0) direction = LEFT;
    else if (dx == 0 && dy == 1) direction = DOWN;
    else if (dx == 0 && dy == -1) direction = UP;
  }
  
  int new_x = snake[0].x + DIRECTIONS[direction][0];
  int new_y = snake[0].y + DIRECTIONS[direction][1];

  // Check for collisions
  if (!isValidPos(new_x, new_y)) {
    gameOver = true;
    gameOverTime = millis();  // Record the time when the game over occurred
    return;
  }

  // Shift the snake's body forward
  for (int i = snake_length - 1; i > 0; i--) {
    snake[i] = snake[i - 1];
  }

  // Move the snake's head
  snake[0].x = new_x;
  snake[0].y = new_y;

  // Check if the snake has eaten the apple
  if (snake[0].x == apple[0] && snake[0].y == apple[1]) {
    snake_length++;  // Increase snake length
    score += 1;  // Increase score
    speed = max(MIN_SPEED, speed / 5);  // Increase the speed by a factor of 5, but not below MIN_SPEED
    newApple();  // Generate a new apple
  }
}

void setup() {
  randomSeed(analogRead(0));  // Seed random number generator
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Initialize OLED
  display.display();
  delay(2000);  // Pause for 2 seconds
  resetGame();
}

void loop() {
  if (gameOver) {
    if (millis() - gameOverTime >= 1000) {
      resetGame();  // Restart game after 1 second
    }
    return;
  }

  // Move the snake if enough time has passed
  if (millis() - lastMove > speed) {
    moveSnake();
    drawGrid();  // Redraw the grid after movement
    lastMove = millis();
  }
}
