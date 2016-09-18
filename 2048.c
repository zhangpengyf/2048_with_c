
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>

#define SIZE 5
uint32_t score = 0;
uint8_t scheme = 0;

bool findPairDown(uint8_t board[SIZE][SIZE]) {
    bool success = false;
    uint8_t x,y;
    for (x = 0; x < SIZE; x++) {
        for (y = 0; y < SIZE-1; y++) {
            if (board[x][y] == board[x][y+1]) return true;
        }
    }
    return success;
}

uint8_t countEmpty(uint8_t board[SIZE][SIZE]) {
    uint8_t x,y;
    uint8_t count=0;
    for (x = 0; x<SIZE; x++) {
        for (y = 0; y < SIZE; y++) {
            if (board[x][y]==0) {
                count++;
            }
        }
    }
    return count;
}

bool gameEnded(uint8_t board[SIZE][SIZE]) {
    bool ended = true;
    if (countEmpty(board)>0) return false;
    if (findPairDown(board)) return false;
    rotateBoard(board);
    if (findPairDown(board)) ended = false;
    rotateBoard(board);
    rotateBoard(board);
    rotateBoard(board);
    return ended;
}

void addRandom(uint8_t board[SIZE][SIZE]) {
    static bool initialized = false;
    uint8_t x, y;
    uint8_t r, len=0;
    uint8_t n, list[SIZE*SIZE][2];
    
    if (!initialized) {
        srand(time(NULL));
        initialized = true;
    }
    
    for (x=0;x<SIZE;x++) {
        for (y=0;y<SIZE;y++) {
            if (board[x][y]==0) {
                list[len][0]=x;
                list[len][1]=y;
                len++;
            }
        }
    }
    
    if (len>0) {
        r = rand()%len;
        x = list[r][0];
        y = list[r][1];
        n = (rand()%10)/9+1;
        board[x][y]=n;
    }
}

void initBoard(uint8_t board[SIZE][SIZE]) {
    uint8_t x,y;
    for (x=0;x<SIZE;x++) {
        for (y=0;y<SIZE;y++) {
            board[x][y]=0;
        }
    }
    addRandom(board);
    addRandom(board);
    drawBoard(board);
    score = 0;
}

void setBufferedInput(bool enable) {
    static bool enabled = true;
    static struct termios old;
    struct termios new;
    
    if (enable && !enabled) {
        // restore the former settings
        tcsetattr(STDIN_FILENO,TCSANOW,&old);
        // set the new state
        enabled = true;
    } else if (!enable && enabled) {
        // get the terminal settings for standard input
        tcgetattr(STDIN_FILENO,&new);
        // we want to keep the old setting to restore them at the end
        old = new;
        // disable canonical mode (buffered i/o) and local echo
        new.c_lflag &=(~ICANON & ~ECHO);
        // set the new settings immediately
        tcsetattr(STDIN_FILENO,TCSANOW,&new);
        // set the new state
        enabled = false;
    }
}

void rotateBoard(uint8_t board[SIZE][SIZE]) {
    uint8_t i,j,n=SIZE;
    uint8_t tmp;
    for (i=0; i<n/2; i++) {
        for (j=i; j<n-i-1; j++) {
            tmp = board[i][j];
            board[i][j] = board[j][n-i-1];
            board[j][n-i-1] = board[n-i-1][n-j-1];
            board[n-i-1][n-j-1] = board[n-j-1][i];
            board[n-j-1][i] = tmp;
        }
    }
}

uint8_t findTarget(uint8_t array[SIZE], uint8_t x, uint8_t stop) {
    for (int t = x - 1; t >= stop; t--){
        if (array[t] != 0) {
            if (array[t] == array[x]){
                //can merge
                return t;
            }else{
                return t + 1;
            }
        }
    }
    //all is zero
    return stop;
}

bool slideArray(uint8_t array[SIZE]) {
    int stop = 0;
    bool moved = false;
    for (int i = 0; i < SIZE; ++i) {
        if (array[i] != 0) {
            int t = findTarget(array, i, stop);
            if (t != i){
                if (array[t] == array[i]){
                    //merge
                    array[t]++;
                    score += (uint32_t)1<<array[t];
                    
                }else {
                    //move to zero
                    assert(array[t] == 0);
                    array[t] = array[x];
                }
                array[x] = 0;
                moved = true;
            }
        }
        
    }
    return moved;
}


bool moveUp(uint8_t board[SIZE][SIZE]) {
    bool moved = false;
    uint8_t x;
    for (x = 0;x < SIZE;x++) {
        moved |= slideArray(board[x]);
    }
    return moved;
}

bool moveLeft(uint8_t board[SIZE][SIZE]) {
    bool moved = false;
    rotateBoard(board);
    moved = moveUp(board);
    rotateBoard(board);
    rotateBoard(board);
    rotateBoard(board);
    return moved;
}

bool moveDown(uint8_t board[SIZE][SIZE]) {
    bool moved = false;
    rotateBoard(board);
    rotateBoard(board);
    moved = moveUp(board);
    rotateBoard(board);
    rotateBoard(board);
    return moved;
}

bool moveRight(uint8_t board[SIZE][SIZE]) {
    bool moved = false;
    rotateBoard(board);
    rotateBoard(board);
    rotateBoard(board);
    moved = moveUp(board);
    rotateBoard(board);
    return moved;
}

void signal_callback_handler(int sig) {
    printf("         TERMINATED         \n");
    setBufferedInput(true);
    printf("\033[?25h\033[m");
    exit(sig);
}

int main(int argc, const char* argv[]){
    uint8_t borad[SIZE][SIZE];
    char c = 0;
    bool moved = false;
    
    printf("\033[?25l\033[2J");
    
    signal(SIGINT, signal_callback_handler);
    
    initBoard(board);
    setBufferedInput(false);
    
    while (true) {
        c = getchar();
        switch(c) {
            case 97:	// 'a' key
            case 104:	// 'h' key
            case 68:	// left arrow
                moved = moveLeft(board);  break;
            case 100:	// 'd' key
            case 108:	// 'l' key
            case 67:	// right arrow
                moved = moveRight(board); break;
            case 119:	// 'w' key
            case 107:	// 'k' key
            case 65:	// up arrow
                moved = moveUp(board);    break;
            case 115:	// 's' key
            case 106:	// 'j' key
            case 66:	// down arrow
                moved = moveDown(board);  break;
            default: moved = false;
        }
        if (moved) {
            drawBoard(board);
            usleep(150000);
            addRandom(board);
            drawBoard(board);
            if (gameEnded(board)) {
                printf("         GAME OVER          \n");
                break;
            }
        }
        if (c == 'q') {
            printf("        QUIT? (y/n)         \n");
            c = getchar();
            if (c == 'y') {
                break;
            }
            drawBoard(board);
        }
        if (c == 'r') {
            printf("       RESTART? (y/n)       \n");
            c = getchar();
            if (c == 'y') {
                initBoard(board);
            }
            drawBoard(board);
        }
    }
    setBufferedInput(true);
    
    printf("\033[?25h\033[m");
    
    return EXIT_SUCCESS;
    
}
