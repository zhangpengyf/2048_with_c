
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <signal.h>

#define SIZE 4
uint32_t score = 0;
uint8_t scheme = 0;

void getColor(uint8_t value, char *color, size_t length) {
	uint8_t original[] = {8,255,1,255,2,255,3,255,4,255,5,255,6,255,7,255,9,0,10,0,11,0,12,0,13,0,14,0,255,0,255,0};
	uint8_t blackwhite[] = {232,255,234,255,236,255,238,255,240,255,242,255,244,255,246,0,248,0,249,0,250,0,251,0,252,0,253,0,254,0,255,0};
	uint8_t bluered[] = {235,255,63,255,57,255,93,255,129,255,165,255,201,255,200,255,199,255,198,255,197,255,196,255,196,255,196,255,196,255,196,255};
	uint8_t *schemes[] = {original,blackwhite,bluered};
	uint8_t *background = schemes[scheme] + 0;
	uint8_t *foreground = schemes[scheme] + 1;
	if (value > 0) while (value--) {
		if (background + 2 < schemes[scheme] + sizeof(original)) {
			background += 2;
			foreground += 2;
		}
	}
	snprintf(color, length, "\033[38;5;%d;48;5;%dm", *foreground, *background);
}

void drawBoard(uint8_t board[SIZE][SIZE]) {
	int i,j;
	char c;
	char color[40], reset[] = "\033[m";
	printf("\033[H");

	printf("2048_with_c %17d score\n\n",score);

	for (j = 0; j < SIZE; j++) {
		for (i = 0; i < SIZE; i++) {
			getColor(board[i][j], color, 40);
			printf("%s", color);
			printf("       ");
			printf("%s", reset);
		}
		printf("\n");
		for (i = 0; i < SIZE; i++) {
			getColor(board[i][j], color, 40);
			printf("%s",color);
			if (board[i][j] != 0) {
				char s[8];
				snprintf(s, 8, "%u", (uint32_t)1<<board[i][j]);
				uint8_t t = 7 - strlen(s);
				printf("%*s%s%*s", t-t/2, "", s, t/2, "");
			} else {
				printf("   ·   ");
			}
			printf("%s",reset);
		}
		printf("\n");
		for (i = 0; i < SIZE; i++) {
			getColor(board[i][j], color, 40);
			printf("%s", color);
			printf("       ");
			printf("%s", reset);
		}
		printf("\n");
	}
	printf("\n");
	printf("        ←,↑,→,↓ or q        \n");
	printf("\033[A"); // one line up
}

bool hasPair(uint8_t board[SIZE][SIZE]) {
    int i,j;
    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE-1; j++) {
            if (board[i][j] == board[i][j+1]) return true;
        }
    }
    return false;
}

uint8_t getEmptyCount(uint8_t board[SIZE][SIZE]) {
    int i,j;
    int count=0;
    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            if (board[i][j]==0) {
                count++;
            }
        }
    }
    return count;
}

void rotateBoardClockwise(uint8_t board[SIZE][SIZE]) {
    int i,j,n = SIZE;
    uint8_t tmp;
    for (i = 0; i < n/2; i++) {
        for (j = i; j < n-i-1; j++) {
            tmp = board[i][j];
            board[i][j] = board[j][n-i-1];
            board[j][n-i-1] = board[n-i-1][n-j-1];
            board[n-i-1][n-j-1] = board[n-j-1][i];
            board[n-j-1][i] = tmp;
        }
    }
}

bool doesGameOver(uint8_t board[SIZE][SIZE]) {
    bool ended = true;
    if (getEmptyCount(board) > 0) return false;
    if (hasPair(board)) { 
      return false;
    }
    rotateBoardClockwise(board);
    if (hasPair(board)) {
      ended = false;
    }
    rotateBoardClockwise(board);
    rotateBoardClockwise(board);
    rotateBoardClockwise(board);
    return ended;
}

void addRandom(uint8_t board[SIZE][SIZE]) {
    static bool initialized = false;
    int i, j;
    int r, len = 0;
    int list[SIZE*SIZE][2];
    
    if (!initialized) {
        srand(time(NULL));
        initialized = true;
    }
    
    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            if (board[i][j] == 0) {
                list[len][0] = i;
                list[len][1] = j;
                len++;
            }
        }
    }
    
    if (len > 0) {
        r = rand()%len;
        i = list[r][0];
        j = list[r][1];
        board[i][j] = 1;//always 2
    }
}

void initGame(uint8_t board[SIZE][SIZE]) {
    int i,j;
    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            board[i][j]=0;
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
                    array[t] = array[i];
                }
                array[i] = 0;
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
    rotateBoardClockwise(board);
    moved = moveUp(board);
    rotateBoardClockwise(board);
    rotateBoardClockwise(board);
    rotateBoardClockwise(board);
    return moved;
}

bool moveDown(uint8_t board[SIZE][SIZE]) {
    bool moved = false;
    rotateBoardClockwise(board);
    rotateBoardClockwise(board);
    moved = moveUp(board);
    rotateBoardClockwise(board);
    rotateBoardClockwise(board);
    return moved;
}

bool moveRight(uint8_t board[SIZE][SIZE]) {
    bool moved = false;
    rotateBoardClockwise(board);
    rotateBoardClockwise(board);
    rotateBoardClockwise(board);
    moved = moveUp(board);
    rotateBoardClockwise(board);
    return moved;
}

void signal_callback_handler(int sig) {
    printf("         TERMINATED         \n");
    setBufferedInput(true);
    printf("\033[?25h\033[m");
    exit(sig);
}

int main(int argc, const char* argv[]){
    uint8_t board[SIZE][SIZE];
    char c = 0;
    bool moved = false;
    
    printf("\033[?25l\033[2J");
    
    signal(SIGINT, signal_callback_handler);
    
    initGame(board);
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
            usleep(100000);
            addRandom(board);
            drawBoard(board);
            if (doesGameOver(board)) {
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
                initGame(board);
            }
            drawBoard(board);
        }
    }
    setBufferedInput(true);
    
    printf("\033[?25h\033[m");
    
    return EXIT_SUCCESS;
    
}
