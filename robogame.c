//Operating System: WindowsOS

#include <ncurses/ncurses.h>
#include <unistd.h>    //for usleep()
#include <stdlib.h>   //for srand() and rand()
#include <time.h>    //for time()

//define constants
#define RESCUE_SCORE 10    //score for rescuing a person
#define MAX_PEOPLE 5     //maximum number of people at any time visible on screen
#define X_BUFFER 4     //buffer for x coordinate of danger zones and people (so that it is not too close to edges)
#define Y_BUFFER 5     //buffer for y coordinate of danger zones and people 
#define MIN_XDELAY 30000     //minimum delay for horizontal movement (prevents game from being too fast)
#define MIN_YDELAY 80000     //minimum delay for vertical movement

//global variables
int X_DELAY = 70000;      //initial delay for horizontal movement
int Y_DELAY = 120000;     //initial delay for vertical movement
char DANGER_ZONE, PERSON;

//all function prototypes
//functions for displaying the game
void displayStartScreen(WINDOW *win);
void displayGameOver(WINDOW *win, int score, int duration);
void flashScreen(WINDOW *win, int duration);

//function for player movement
void roboMovement(WINDOW *win, int *lives, int *score, int *level);

//functions for initialising the game
void initDangerZones(char matrix[][COLS]);
void initPeopleToRescue(char rescueMatrix[][COLS], char dangerMatrix[][COLS]);

//functions for handling collisions
void handleDangerZoneCollision(WINDOW *win, int *lives, int x, int y, int max_x, int max_y, char dangerMatrix[max_y][max_x]);
void handlePeopleRescue(int *score, int *level, int x, int y, int max_x, int max_y, char dangerMatrix[max_y][max_x], char rescueMatrix[max_y][max_x]);

//main function
int main() {
    initscr(); //initialise ncurses
    noecho();  //don't echo keypresses
    curs_set(0);  //hide cursor
    
    start_color();  //enable colour

    //create new window
    int max_x, max_y;   //max_x and max_y store the width and height of the terminal window
    getmaxyx(stdscr, max_y, max_x);   //get window dimensions
    WINDOW *mainwin = newwin(max_y, max_x, 0, 0);   
    keypad(mainwin, TRUE);   //enable keyboard input for the window

    //seed for randomization
    srand(time(NULL));

    //display start screen
    displayStartScreen(mainwin);

    //initial gameplay values
    int lives = 3;
    int score = 0;
    int level = 1;

    roboMovement(mainwin, &lives, &score, &level);  //call function to start the game

    if (lives == 0) {
        displayGameOver(mainwin, score, 5000);
    }

    delwin(mainwin);

    endwin();   // Restore normal terminal behavior
    return 0;
}

//function for player movement, i.e. the main game
void roboMovement(WINDOW *win, int *lives, int *score, int *level) {
    //get window dimensions
    int max_x, max_y;
    getmaxyx(win, max_y, max_x);

    int x = (max_x - 2) / 2, y = max_y - 5;   //initial position of robot
    int next_x = 0, next_y = 0;
    int x_direction = 0, y_direction = -1;  //set initial position to move upwards

    int startMovement = 1; //flag to start movement

    char rescueMatrix[max_y][max_x]; //matrix to store people to rescue
    char dangerMatrix[max_y][max_x]; //matrix to store danger zones

    // Initialise danger zones and spawn people to rescue
    initDangerZones(dangerMatrix);
    initPeopleToRescue(rescueMatrix, dangerMatrix);

    nodelay(win, TRUE); //don't wait for user input when calling getch()

    while (*lives > 0) {    //'lives' is a pointer because it is modified in the function
        int ch = wgetch(win);  //get user input

        //quit game
        if (ch == 'q'){
            break;   
        }

        //game over
        if (*lives == 0) {
            break;  
        }
        

        //control robot movement
        switch (ch) {    
            case KEY_LEFT:
                x_direction = -1;
                y_direction = 0;
                break;
            case KEY_RIGHT:
                x_direction = 1;
                y_direction = 0;
                break;
            case KEY_UP:
                x_direction = 0;
                y_direction = -1;
                break;
            case KEY_DOWN:
                x_direction = 0;
                y_direction = 1;
                break;
            default:
                break;
        }

        werase(win);  //clear window

        box(win, 0, 0);   //0, 0 gives default characters for the vertical and horizontal lines

        //print arrow based on direction
        if (x_direction == 1) {
            mvwaddch(win, y, x + x_direction, ACS_RARROW);
            mvwaddch(win, y, x, ACS_DIAMOND);
        } else if (x_direction == -1) {
            mvwaddch(win, y, x, ACS_DIAMOND);
            mvwaddch(win, y, x + x_direction, ACS_LARROW);
        } else if (y_direction == 1) {
            mvwaddch(win, y + y_direction, x, ACS_DARROW);
            mvwaddch(win, y, x, ACS_DIAMOND);
        } else if (y_direction == -1) {
            mvwaddch(win, y, x, ACS_DIAMOND);
            mvwaddch(win, y + y_direction, x, ACS_UARROW);
        }

        init_pair(3, COLOR_CYAN, COLOR_BLACK);
        init_pair(4, COLOR_RED, COLOR_BLACK);

        //print danger zones
        for (int i = 0; i < max_y; ++i) {
            for (int j = 0; j < max_x; ++j) {
                if (dangerMatrix[i][j] == DANGER_ZONE) {
                    wattron(win, COLOR_PAIR(4));
                    mvwaddch(win, i, j, ACS_PLMINUS);
                    wattroff(win, COLOR_PAIR(4));  
                }
            }
        }

        //print people 
        for (int i = 0; i < max_y; ++i) {
            for (int j = 0; j < max_x; ++j) {
                if (rescueMatrix[i][j] == PERSON) {
                    wattron(win, COLOR_PAIR(3));
                    mvwaddch(win, i, j, ACS_BLOCK);
                    wattroff(win, COLOR_PAIR(3));
                }
            }
        }

        //print lives, score, level and quit message at bottom of window
        mvwprintw(win, max_y - 2, max_x/2, "Lives: %d", *lives);
        mvwprintw(win, max_y - 2, 2, "Score: %d", *score);
        mvwprintw(win, max_y - 2, max_x/2 - 35, "Level: %d", *level);
        mvwprintw(win, max_y - 2, max_x - 17, "Press q to quit");

        wrefresh(win);

        if (x_direction != 0) {
            usleep(X_DELAY);   //delay for horizontal movement
        } else {
            usleep(Y_DELAY);  //delay for vertical movement
        }

        //update position
        next_x = x + x_direction;
        next_y = y + y_direction;

        //check for collision with box edges (NOT screen edges)
        if (next_x >= max_x - 2|| next_x < 2) {
            *lives -= 1;
            flashScreen(win, 100);
            x_direction *= -1;   
        } else {
            x += x_direction;
        }

        if (next_y >= max_y - 2|| next_y < 2) {
            *lives -= 1;
            flashScreen(win, 100);
            y_direction *= -1;
        } else {
            y += y_direction;
        }

        //check for collision with danger zones
        handleDangerZoneCollision(win, lives, x, y, max_x, max_y, dangerMatrix);

        //check for collision with people (easier to take out if statement from function)
        if (rescueMatrix[y][x] == PERSON) {
            handlePeopleRescue(score, level, x, y, max_x, max_y, dangerMatrix, rescueMatrix);
        }
    }
}

//function to display start screen
void displayStartScreen(WINDOW *win) {
    //set color and attributes for the main text
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    wattron(win, COLOR_PAIR(1) | A_BOLD);

    mvwprintw(win, LINES / 2 - 3, COLS / 2 - 15, "ROBO-RESCUE");

    wattroff(win, A_BOLD | COLOR_PAIR(1));

    wmove(win, LINES / 2 - 5, COLS / 2 - 17);
    whline(win, ACS_HLINE, 38);   //draw horizontal line for top border

    //rest of the content
    mvwprintw(win, LINES / 2 - 1, COLS / 2 - 15, "Your mission is to rescue people");
    wmove(win, LINES / 2 - 1, COLS / 2 + 19);
    waddch(win, ACS_BLOCK);
    mvwprintw(win, LINES / 2, COLS / 2 - 15, "Beware of danger zones");
    wmove(win, LINES / 2, COLS / 2 + 9);
    waddch(win, ACS_PLMINUS);
    mvwprintw(win, LINES / 2 + 1, COLS / 2 - 15, "Good luck soldier");
    mvwprintw(win, LINES / 2 + 2, COLS / 2 - 15, " ");

    //set color and attributes for the bottom text
    init_pair(2, COLOR_BLACK, COLOR_WHITE);
    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, LINES / 2 + 3, COLS / 2 - 15, "Press any key to start");
    wattroff(win, COLOR_PAIR(2));

    wmove(win, LINES / 2 + 5, COLS / 2 - 17);
    whline(win, ACS_HLINE, 38);    //draw horizontal line for bottom border

    wrefresh(win);

    //wait for any key press to start the game
    wgetch(win);
}

//function to display game over screen
void displayGameOver(WINDOW *win, int score, int duration) {    //duration is needed because of flashScreen()
    //similar (basically identical) logic to displayStartScreen()
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    wattron(win, COLOR_PAIR(1) | A_BOLD);

    mvwprintw(win, LINES / 2 - 3, COLS / 2 - 15, "GAME OVER!");

    wattroff(win, A_BOLD | COLOR_PAIR(1));

    wmove(win, LINES / 2 - 5, COLS / 2 - 17);
    whline(win, ACS_HLINE, 38);   //draw horizontal line for top border

    //rest of the content
    mvwprintw(win, LINES / 2 - 1, COLS / 2 - 15, "Score: %d", score);

    if (score >= 150) {
        mvwprintw(win, LINES / 2, COLS / 2 - 15, "Valiant effort, soldier. Impressive");
    } else if (score >= 50) {
        mvwprintw(win, LINES / 2, COLS / 2 - 15, "Not bad, soldier. Well done");
    } else {
        mvwprintw(win, LINES / 2, COLS / 2 - 15, "You can do better, soldier. Try again");
    }

    //set color and attributes for the bottom text
    init_pair(2, COLOR_BLACK, COLOR_WHITE);

    int remainingSeconds = duration / 1000;

    for (int seconds = remainingSeconds; seconds >= 0; seconds--) {
        wattron(win, COLOR_PAIR(2));
        mvwprintw(win, LINES / 2 + 3, COLS / 2 - 15, "Automatically exiting in %d second(s)", seconds);

        wattron(win, COLOR_PAIR(1));
        wmove(win, LINES / 2 + 5, COLS / 2 - 17);
        whline(win, ACS_HLINE, 38);       //draw horizontal line for bottom border
        wattroff(win, COLOR_PAIR(1));    //don't want the bottom border to be white

        wrefresh(win);

        napms(1000);  //delay for 1 second (as counted in milliseconds)

        mvwprintw(win, LINES / 2 + 3, COLS / 2 - 15, "                             ");  //clear line of text
    }
    wattroff(win, COLOR_PAIR(2));
    wrefresh(win);
}

//function to generate random position for danger zones and people
int generateRandomPosition(int buffer, int max_limit){
    return rand() % (max_limit - 2 * buffer) + buffer;   //buffer is used to avoid being too close to edges
}

//function to initialise danger zones
void initDangerZones(char matrix[][COLS]) {
    //clear danger zones
    for (int i = 0; i < LINES; ++i) {
        for (int j = 0; j < COLS; ++j) {
            matrix[i][j] = ' ';
        }
    }

    //randomly place two danger zones
    for (int i = 0; i < 2; ++i) {
        int rand_x, rand_y;
        do {
            rand_x = generateRandomPosition(X_BUFFER, COLS);
            rand_y = generateRandomPosition(Y_BUFFER, LINES);    //avoid placing danger zones too close to edges
        } while (matrix[rand_y][rand_x] == DANGER_ZONE);

        matrix[rand_y][rand_x] = DANGER_ZONE;
    }
}

//function to initialise people to rescue
void initPeopleToRescue(char rescueMatrix[][COLS], char dangerMatrix[][COLS]) {
    int currentPeople = 0;
    //clear existing people
    for (int i = 0; i < LINES; ++i) {
        for (int j = 0; j < COLS; ++j) {
            if (rescueMatrix[i][j] == PERSON) {
                rescueMatrix[i][j] = ' ';
            }
        }
    }

   //count existing people (so that we know how many more to add)
    for (int i = 0; i < LINES; ++i) {
        for (int j = 0; j < COLS; ++j) {
            if (rescueMatrix[i][j] == PERSON) {
                currentPeople++;
            }
        }
    }

    //add new people to reach the total of 5 (at any time in game)
    while (currentPeople < MAX_PEOPLE) {
        int rand_x, rand_y;
        do {
            rand_x = generateRandomPosition(X_BUFFER, COLS);   //avoid placing people too close to edges
            rand_y = generateRandomPosition(Y_BUFFER, LINES);
        } while (rescueMatrix[rand_y][rand_x] == PERSON || dangerMatrix[rand_y][rand_x] == DANGER_ZONE);

        rescueMatrix[rand_y][rand_x] = PERSON;
        currentPeople++;
    }
}

//function to handle collision with danger zones
void handleDangerZoneCollision(WINDOW *win, int *lives, int x, int y, int max_x, int max_y, char dangerMatrix[max_y][max_x]){
    if (dangerMatrix[y][x] == DANGER_ZONE) {
        *lives -= 1;   //decrease lives by 1
        flashScreen(win, 100);    //flash screen for 100 milliseconds (so that user knows they lost a life)
        dangerMatrix[y][x] = ' ';  //remove the collided danger zone

        //add a new danger zone elsewhere
        int rand_x, rand_y;    //random coordinates for new danger zone
        do {
            rand_x = generateRandomPosition(X_BUFFER, COLS);
            rand_y = generateRandomPosition(Y_BUFFER, LINES);
        } while (dangerMatrix[rand_y][rand_x] == DANGER_ZONE);

        dangerMatrix[rand_y][rand_x] = DANGER_ZONE;
    }
}

//function to handle collision with people
void handlePeopleRescue(int *score, int *level, int x, int y, int max_x, int max_y, char dangerMatrix[max_y][max_x], char rescueMatrix[max_y][max_x]) {
    int rand_x, rand_y;   //random coordinates for new person to rescue
    //rescuing a person results in adding two more danger zones
    for (int i = 0; i < 2; ++i) {
        do {
            rand_x = generateRandomPosition(X_BUFFER, COLS);
            rand_y = generateRandomPosition(Y_BUFFER, LINES);
        } while (dangerMatrix[rand_y][rand_x] == DANGER_ZONE || rescueMatrix[rand_y][rand_x] == PERSON);

        dangerMatrix[rand_y][rand_x] = DANGER_ZONE;
    }

    //increase score
    *score += RESCUE_SCORE;

    //find the rescued person and remove it from the matrix
   if (rescueMatrix[y][x] == PERSON) {
       rescueMatrix[y][x] = ' ';
   }
   //add new person to rescue
   do {
       rand_x = generateRandomPosition(X_BUFFER, COLS);
       rand_y = generateRandomPosition(Y_BUFFER, LINES);
    } while (rescueMatrix[rand_y][rand_x] == PERSON || dangerMatrix[rand_y][rand_x] == DANGER_ZONE);

    rescueMatrix[rand_y][rand_x] = PERSON;

    //check for level advancement
    if (*score >= (*level) * 5 * RESCUE_SCORE) {   //this allows cumulative score while still advancing levels
        *level += 1;
         //don't reset score
        //increase the speed (reduce delay)
        if (X_DELAY > MIN_XDELAY && Y_DELAY > MIN_YDELAY) {
            X_DELAY -= 5000; 
            Y_DELAY -= 5000;
        //minium delay introduced to avoid game being too fast
        }
    }
}

//function to flash screen (for collision detection)
void flashScreen(WINDOW *win, int duration){   //duration is passed as a parameter as different for game over screen
    init_pair(5, COLOR_BLACK, COLOR_BLACK);

    werase(win);
    wattron(win, A_BLINK | A_BOLD | COLOR_PAIR(5));
    box(win, 0, 0);   
    wattroff(win, COLOR_PAIR(5));

    wrefresh(win);
    napms(duration);   

    werase(win);
    
    box(win, 0, 0);

    wattroff(win, A_BLINK | A_BOLD);

    wrefresh(win);
    timeout(-1);   //reset timeout; timeout refers to the time to wait for user input
    //timeout of -1 means wait indefinitely
}
