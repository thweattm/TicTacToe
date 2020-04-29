
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h> // Hardware-specific library
#include <Adafruit_STMPE610.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>

#define STMPE_CS 16
#define TFT_CS   0
#define TFT_DC   15
#define SD_CS    2

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 3800
#define TS_MAXX 100
#define TS_MINY 100
#define TS_MAXY 3750
#define PENRADIUS 3

// Read images from SD Card related //
#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <Adafruit_ImageReader.h> // Image-reading functions
SdFat SD;                         // SD card filesystem
Adafruit_ImageReader reader(SD);  // Image-reader object, pass in SD filesys
Adafruit_Image imgX;              // An image loaded into RAM
Adafruit_Image imgO;              // An image loaded into RAM
ImageReturnCode stat;             // Status from image-reading functions

// Game variables //
int16_t boardDefinitions[9][8];   // Holds defitions for each of the boards squares (edges, XY coordinates)
int8_t gameBoard[3][3];           // Main game board to hold current player moves
int8_t player1Score = 0;
int8_t player2Score = 0;
int8_t currentTurn = 1;           // Current player turn
int8_t startTurn = 1;             // Player to start each game: alternates each game
int8_t winner = 0;                
bool inGame = false;              // Game in progress flag


/* **************************************************************************************************************
 * Set game board layout dimensions and definitions
 * Only run once on board setup
 * Input: Screen height and width in pixels
 * 
 * 1) Creates an evenly spaced game board based on the width, starting from the bottom.
 *    It calculates the needed width of each square, then uses that same value as the height
 *    working up from the bottom to get the square edges.
 *
 * 2) Set definition array:
 *    each array item is as follows:
 *    boardDefinitions[i] = top edge, right edge, bottom edge, left edge, graphicX, graphicY, centerX, centerY
 *    edges represent boundaries for the given square
 *    graphic X and Y are the coordinates to place the 'X' or 'O' graphics in each square
 *    center X and Y are the center coordinates of the square used to draw the winning line on the board
 * **************************************************************************************************************/
void set_definitions(){
  int16_t hSpacing = 76;  // Horizontal spacing of game board squares
  int16_t vSpacing = 73;  // Vertical spacing of game board squares
  int16_t topEdge = 95;
  int16_t bottomEdge = topEdge + vSpacing;
  int16_t leftEdge = 6;
  int16_t rightEdge = leftEdge + hSpacing;
  
  for (int i = 0; i < 9; i++){
    boardDefinitions[i][0] = topEdge;
    boardDefinitions[i][1] = rightEdge;
    boardDefinitions[i][2] = bottomEdge;
    boardDefinitions[i][3] = leftEdge;
    boardDefinitions[i][4] = (((rightEdge - leftEdge) - 50) / 2) + leftEdge;    // Graphic placement x
    boardDefinitions[i][5] = (((bottomEdge - topEdge) - 50) / 2) + topEdge;     // Graphic placement y
    boardDefinitions[i][6] = ((rightEdge - leftEdge) / 2) + leftEdge;           // Center placement x
    boardDefinitions[i][7] = ((bottomEdge - topEdge) / 2) + topEdge;            // Center placement y
    
    if ((i+1) % 3 == 0){
      // New Row
      topEdge += vSpacing;
      bottomEdge += vSpacing;
      leftEdge = 6;
      rightEdge = leftEdge + hSpacing;
      
    } else {
      // Same Row, next square
      leftEdge += hSpacing;
      rightEdge += hSpacing;
    }
  }
}

/* **************************************************************************************************************
 * Start up a new game
 * Set game board to all 0's
 * Alternate the 'current turn' variable
 * Play game
 * **************************************************************************************************************/
void start_game(){
  // Set game board to zeros
  for (int i = 0; i < 3; i++){
    for (int j = 0; j < 3; j++){
      gameBoard[i][j] = 0;
    }
  }
  
  // Set current player turn (alternates every new game)
  currentTurn = startTurn;

  draw_score_board();
  draw_game_board();
  draw_current_turn();
}


void draw_score_board(){
  // Score Board
  tft.fillRect(0, 0, 240, 50, ILI9341_BLACK);
  tft.drawFastVLine(120,0,50,ILI9341_WHITE); // Middle divider
  tft.drawFastHLine(0,50,240,ILI9341_WHITE); // Bottom across
  tft.setFont(&FreeMonoBold12pt7b);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(85, 30);
  tft.print(player1Score);
  tft.setCursor(142, 30);
  tft.print(player2Score);
  reader.drawBMP("/x_35.bmp", tft, 15, 5);
  reader.drawBMP("/o_35.bmp", tft, 190, 5);
}


/* **************************************************************************************************************
 * Print the main game board attributes
 * Game board - Grid and X's & O's
 * **************************************************************************************************************/
void draw_game_board(){
  //tft.fillScreen(0);
  tft.fillRect(0, 51, 240, 269, ILI9341_BLACK);
  tft.setFont(&FreeMono9pt7b);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(52, 75);
  tft.print("Current Turn");

  // Game Board
  tft.drawFastHLine(6,168,228,ILI9341_WHITE);
  tft.drawFastHLine(6,241,228,ILI9341_WHITE);
  tft.drawFastVLine(82,95,219,ILI9341_WHITE);
  tft.drawFastVLine(158,95,219,ILI9341_WHITE);
}


void draw_current_turn(){
  if (currentTurn == 1){
    // Draw black square over-top red arrow
    tft.fillRect(193, 55, 29, 35, ILI9341_BLACK);
    // Print blue arrow
    reader.drawBMP("/blueArrow.bmp", tft, 15, 55);
  } else {
    // Draw black square over top blue arrow
    tft.fillRect(15, 55, 29, 35, ILI9341_BLACK);
    // Print red arrow
    reader.drawBMP("/redArrow.bmp", tft, 193, 55);
  }
}


void draw_logo_start(){
  reader.drawBMP("/logo.bmp", tft, 0, 0);
}



/* **************************************************************************************************************
 * Get the game board square from the given touch points
 * Input: X and Y touch points from screen
 * Output: either the index of the game board square (based on the board_definition list)
 * or -1 if the touch was not within the game board boundaries
 * **************************************************************************************************************/
int8_t get_square(int16_t x, int16_t y){
  for (int i = 0; i < 9; i++){
    // 0 = top edge, 1 = right edge, 2 = bottom edge, 3 = left edge
    if (x > boardDefinitions[i][3] && x < boardDefinitions[i][1]){
      if (y > boardDefinitions[i][0] && y < boardDefinitions[i][2]){
        return i;
      }
    }
  }
  return -1;
}



/* **************************************************************************************************************
 * Make the requested move
 * Input: X and Y touch points from screen
 * Touch points are validated for both boundary and open square
 * **************************************************************************************************************/
void make_move(int16_t x, int16_t y){
  int8_t index = -1;
  if (is_valid_move(x, y, index)){
    int8_t r = index / 3;
    int8_t c = index % 3;
    
    // Mark game board
    gameBoard[r][c] = currentTurn;
    
    // Draw piece
    if (currentTurn == 1){
      imgX.draw(tft,boardDefinitions[index][4], boardDefinitions[index][5]);
    } else {
      imgO.draw(tft,boardDefinitions[index][4], boardDefinitions[index][5]);
    }
    
    // Advance turn
    currentTurn = (currentTurn % 2) + 1;
    draw_current_turn();
  }
}



/* **************************************************************************************************************
 * Check a move for validity
 * Input: X and Y touch points from screen
 * First - checks if touch points are within a game board square
 * Second - checks if game board location is open
 * Returns either -1 if not valid, or i for the index of the 'box' within the board_definition list
 * **************************************************************************************************************/
bool is_valid_move(int16_t x, int16_t y, int8_t& index){
  // get board 'square' based on coordinates
  int8_t i = get_square(x, y);
  
  // -1 indicates move (touch) is not within game board boundaries
  if (i == -1){return false;}
  
  // If within boundaries, check board for '0' (open space)
  int r = i / 3;
  int c = i % 3;
  
  if (gameBoard[r][c] != 0){
    return false;
  } else {
    index = i;
    return true;
  }
}



/* **************************************************************************************************************
 * Check if the game is over by draw
 * Return true or false
 * **************************************************************************************************************/
bool check_game_tie(){
  for (int i = 0; i < 3; i++){
    for (int j = 0; j < 3; j++){
      if (gameBoard[i][j] == 0){return false;}
    }
  }

  // Write in text
  tft.fillRect(0, 51, 240, 48, ILI9341_BLACK);
  tft.setFont(&FreeMono9pt7b);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setCursor(7, 67);
  tft.print("Game Over! No Winner!");
  tft.setCursor(12, 86);
  tft.print("Press to play again");
  
  return true;
}



void write_winner(int winner){
  // Write in text
  tft.fillRect(0, 51, 240, 48, ILI9341_BLACK);
  tft.setCursor(77, 67);
  tft.setFont(&FreeMono9pt7b);
  if (winner == 1){
    player1Score++;
    tft.setTextColor(ILI9341_BLUE);
    tft.print("X Wins!!");
  } else {
    player2Score++;
    tft.setTextColor(ILI9341_RED);
    tft.print("O Wins!!");
  }
  tft.setCursor(12, 86);
  tft.setFont(&FreeMono9pt7b);
  tft.print("Press to play again");
  draw_score_board();
}


/* **************************************************************************************************************
 * Check if the game is over by winner
 * Return true or false
 * **************************************************************************************************************/
bool check_winner(){
  for (int i = 0; i < 3; i++){
    // 3 Columns
    if (gameBoard[0][i] != 0 && gameBoard[0][i] == gameBoard[1][i] && gameBoard[1][i] == gameBoard[2][i]){
      draw_winning_line(i,i+6);
      write_winner(gameBoard[0][i]);
      return true;
    }

    // 3 Rows
    if (gameBoard[i][0] != 0 && gameBoard[i][0] == gameBoard[i][1] && gameBoard[i][1] == gameBoard[i][2]){
      draw_winning_line(i*3,i+2);
      write_winner(gameBoard[i][0]);
      return true;
    }
  }

  // 2 diagnols
  if (gameBoard[0][0] != 0 && gameBoard[0][0] == gameBoard[1][1] && gameBoard[1][1] == gameBoard[2][2]){
    draw_winning_line(0,8);
    write_winner(gameBoard[0][0]);
    return true;
  }
  if (gameBoard[2][0] != 0 && gameBoard[2][0] == gameBoard[1][1] && gameBoard[1][1] == gameBoard[0][2]){
    draw_winning_line(2,6);
    write_winner(gameBoard[2][0]);
    return true;
  }
  
  return false;
}


void draw_winning_line(int a, int b){
  //drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color),
  tft.drawLine(boardDefinitions[a][6], boardDefinitions[a][7], boardDefinitions[b][6], boardDefinitions[b][7], ILI9341_WHITE);
}



void setup() {
  Serial.begin(115200);

  delay(10);
  Serial.println("FeatherWing TFT");
  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }
  Serial.println("Touchscreen started");
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);

  Serial.print(F("Initializing filesystem..."));
  // SD card is pretty straightforward, a single call...
  if(!SD.begin(SD_CS, SD_SCK_MHZ(25))) { // ESP32 requires 25 MHz limit
    Serial.println(F("SD begin() failed"));
    for(;;); // Fatal error, do not continue
  }
  Serial.println(F("OK!"));

  // Load game graphics into RAM
  Serial.println("Loading graphics into RAM...");
  stat = reader.loadBMP("/x_50.bmp", imgX);
  //reader.printStatus(stat); // How'd we do?
  stat = reader.loadBMP("/o_50.bmp", imgO);
  //reader.printStatus(stat); // How'd we do?

  set_definitions();
  draw_logo_start();
  
}

void loop() {
  // Retrieve a point  
  if (! ts.bufferEmpty()) {
    if (!inGame){
      // Start up the game
      start_game();
      inGame = true;

    } else { 
      TS_Point p = ts.getPoint();
  
      // Scale from ~0->4000 to tft.width using the calibration #'s
      p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
      p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
      
      make_move(p.x, p.y);

      if (check_game_tie() || check_winner()){
        inGame = false;
        startTurn = (startTurn % 2) + 1;
      }
    }

    delay(500);

    // Empty out the buffer as I only want first touch
    while (! ts.bufferEmpty()){
      TS_Point p = ts.getPoint();
    }
  }
}
