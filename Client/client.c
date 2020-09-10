#include <fcntl.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/socket.h>

#define ENTER 10
#define FILENAME "./2017203028.txt"
#define PERMS 0666
#define MAX_NAME_SIZE 64
#define MAX_BUF_SIZE 1024

typedef struct {
    char id[64];
    char password[64];
    int win;
    int lose;
    double percent;
}Player_info;

void init_main_menu(void);
void sign_up(void);
void befor_sign_in(void);
void after_sign_in(char *id);
void information(char *id);
void withdrawal(char *id);
void game_play(Player_info Player1, Player_info Player2);
void signalHandler(int signum);
void errorHandler(const char* msg);
int sign_up_in(WINDOW *window1, WINDOW *window2, char *id, char *password, int kind);

struct sockaddr_in servAddr = {0x00, };
int sockfd = 0;
int callnum = 4;
int temp_mim = 0;

int main(int argc, char const *argv[])
{
    struct sockaddr_in servAddr = { 0x00, };
    signal(SIGINT, signalHandler);
    signal(SIGPIPE, signalHandler);
    if (argc != 3) {
        printf("Usage : %s [server IP] [port]\n", argv[0]);
        exit(1);
    }
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        errorHandler("sock() error!");
    }
    servAddr.sin_family = PF_INET;
    servAddr.sin_addr.s_addr = inet_addr(argv[1]);
    servAddr.sin_port = htons(atoi(argv[2]));

    if (connect(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) == -1) {
        errorHandler("connect() error!");
    }
    initscr();

    if (has_colors() == FALSE)
    {
        puts("Terminal does not support colors!");
        endwin();
        return 1;
    }
    else
    {
        start_color();
        init_pair(1, COLOR_BLUE, COLOR_WHITE);
        init_pair(2, COLOR_WHITE, COLOR_BLUE);
    }

    refresh();

    init_main_menu(); // init cursor & input enter
    endwin();

    return 0;
}

void init_main_menu(void)
{
    WINDOW *window1;
    WINDOW *window2;

    WINDOW **items;
    items = (WINDOW **)malloc(4 * sizeof(WINDOW *));

    curs_set(0);

    window1 = newwin(18, 80, 0, 0);
    window2 = newwin(6, 80, 18, 0);

    wbkgd(window1, COLOR_PAIR(1));
    wbkgd(window2, COLOR_PAIR(2));

    mvwprintw(window1, 5, 28, "System Software Practice");
    mvwprintw(window1, 7, 37, "OTHELLO");
    mvwprintw(window1, 14, 68, "2017203028");
    mvwprintw(window1, 16, 68, "MinJu Kim");
    mvwprintw(window2, 3, 15, "SIGN IN");
    mvwprintw(window2, 3, 38, "SIGN UP");
    mvwprintw(window2, 3, 61, "EXIT");
    wmove(window2, 3, 15);

    items[0] = window2;
    items[1] = newwin(1, 7, 21, 15);
    items[2] = newwin(1, 7, 21, 38);
    items[3] = newwin(1, 4, 21, 61);
    wbkgd(items[0], COLOR_PAIR(2));
    wbkgd(items[1], COLOR_PAIR(1));
    wprintw(items[1], "SIGN IN");
    wprintw(items[2], "SIGN UP");
    wprintw(items[3], "EXIT");

    wrefresh(window1);
    wrefresh(items[0]);
    wrefresh(items[1]);

    int index = 0;
    wbkgd(items[1], COLOR_PAIR(1));
    keypad(stdscr, true);
    while (1)
    {
        int key = getch();
        wrefresh(window1);

        for (int i = 0; i < 4; i++)
            wrefresh(items[i]);

        if (key == KEY_RIGHT)
        {
            ++index;
        }
        else if (key == KEY_LEFT)
        {
            --index;
        }

        if (index == 3)
            index = 0;
        else if (index == -1)
            index = 2;

        // print screen
        for (int i = 1; i < 4; i++)
        {
            if (i == index + 1)
                wbkgd(items[i], COLOR_PAIR(1));
            else
                wbkgd(items[i], COLOR_PAIR(2));
            for (int i = 1; i < 4; i++)
                wrefresh(items[i]);
        }

        if (key == ENTER) //call another function
            break;
    }
    free(items);
    keypad(stdscr, true);

    if (index == 0) // SING IN
        befor_sign_in();
    else if (index == 1) // SIGN UP
        sign_up();
    else if (index == 2) // EXIT
    {
        callnum = -1;
        send(sockfd, &callnum, sizeof(int), 0);
        return;
    }
}

void sign_up(void)
{
    WINDOW *window1;
    WINDOW *window2;

    window1 = newwin(18, 80, 0, 0);
    window2 = newwin(6, 80, 18, 0);

    char id[64];          //remain
    char password[64];    //remain
    char next_line = '\n';
    char zero = '0';
    int index = 0;
    int kind = 1; // 1 is sign_up
    int check_overlap = 0;
    Player_info Player1;

    index = sign_up_in(window1, window2, id, password, kind);
    refresh();

    if (index == 0) // SIGN UP
    {
        callnum = 0;
        memset(&Player1, 0x00, sizeof(Player1));
        strcpy(Player1.id, id);
        strcpy(Player1.password, password);
        Player1.lose = 0;
        Player1.win = 0;
        send(sockfd, &callnum, sizeof(int), 0);
        send(sockfd, &Player1, sizeof(Player_info), 0);
        recv(sockfd, &check_overlap, sizeof(int), 0);

        if (check_overlap == 1) // id overlap
        {
            mvwprintw(window2, 5, 0, ">>> %s has already exist in DB! (Please any key...)", id);
            wrefresh(window2);
            getch();
            sign_up();
        }
        else // no overlap
        {
            mvwprintw(window2, 5, 0, ">>> Welcome to OTHELLO World! (Press any key...)");
            wrefresh(window2);
            getch();
            init_main_menu();
        }
    }
    else if (index == 1) // BACK
    {
        init_main_menu();
    }
}

void befor_sign_in(void)
{
    WINDOW *window1;
    WINDOW *window2;

    window1 = newwin(18, 80, 0, 0);
    window2 = newwin(6, 80, 18, 0);

    char id[64];          //remain
    char password[64];    //remain
    char next_line = '\n';
    char zero = '0';
    int index = 0;
    int kind = 2; // 2 is befor_sign_in
    int check_info = 0;

    index = sign_up_in(window1, window2, id, password, kind);
    Player_info Player1;

    if (index == 0) // SIGN IN
    {
        callnum = 1; // SIGN IN
        memset(&Player1, 0x00, sizeof(Player1));
        strcpy(Player1.id, id);
        strcpy(Player1.password, password);
        Player1.lose = 0;
        Player1.win = 0;    //look later
        send(sockfd, &callnum, sizeof(int), 0);
        send(sockfd, &Player1, sizeof(Player_info), 0);
        recv(sockfd, &check_info, sizeof(int), 0);

        if (check_info == 1) // id & pw correct
        {
            after_sign_in(id);
        }
        else if(check_info == 0) // id & pw wrong
        {
            mvwprintw(window2, 5, 0, ">>> Wrong information! (Please any key...)");
            wrefresh(window2);
            getch();
            befor_sign_in();
        }
        else if(check_info == 2)
        {
            mvwprintw(window2, 5, 0, ">>> Same information with another Player! (Please any key...)");
            wrefresh(window2);
            getch();
            befor_sign_in();
        }
    }
    else if (index == 1) // BACK
    {
        init_main_menu();
    }
}

void after_sign_in(char *id)
{
    WINDOW *window1;
    WINDOW *window2;

    WINDOW **items;
    items = (WINDOW **)malloc(4 * sizeof(WINDOW *));

    curs_set(0);

    window1 = newwin(18, 80, 0, 0);
    window2 = newwin(6, 80, 18, 0);

    wbkgd(window1, COLOR_PAIR(1));
    wbkgd(window2, COLOR_PAIR(2));

    mvwprintw(window1, 5, 31, "PLAYER ID : ");
    wprintw(window1, "%s", id);
    mvwprintw(window2, 3, 14, "PLAY");
    mvwprintw(window2, 3, 36, "SIGN OUT");
    mvwprintw(window2, 3, 60, "WITHDRAWAL");
    wmove(window2, 3, 15);

    items[0] = window2;
    items[1] = newwin(1, 4, 21, 14);
    items[2] = newwin(1, 8, 21, 36);
    items[3] = newwin(1, 10, 21, 60);
    wbkgd(items[0], COLOR_PAIR(2));
    wbkgd(items[1], COLOR_PAIR(1));
    wprintw(items[1], "PLAY");
    wprintw(items[2], "SIGN OUT");
    wprintw(items[3], "WITHDRAWAL");

    wrefresh(window1);
    wrefresh(items[0]);
    wrefresh(items[1]);

    int index = 0;
    wbkgd(items[1], COLOR_PAIR(1));
    keypad(stdscr, true);
    while (1)
    {
        int key = getch();
        wrefresh(window1);

        for (int i = 0; i < 4; i++)
            wrefresh(items[i]);

        if (key == KEY_RIGHT)
        {
            ++index;
        }
        else if (key == KEY_LEFT)
        {
            --index;
        }

        if (index == 3)
            index = 0;
        else if (index == -1)
            index = 2;

        // print screen
        for (int i = 1; i < 4; i++)
        {
            if (i == index + 1)
                wbkgd(items[i], COLOR_PAIR(1));
            else
                wbkgd(items[i], COLOR_PAIR(2));
            for (int i = 1; i < 4; i++)
                wrefresh(items[i]);
        }

        if (key == ENTER) //call another function
            break;
    }
    free(items);
    keypad(stdscr, true);

    if (index == 0) // PLAY
        return information(id);
    else if (index == 1) // SIGN OUT
        return init_main_menu();
    else if (index == 2) // WITHDRAWAL
        return withdrawal(id);
}

void information(char *id) // member's game information -> server
{
    WINDOW *window1;
    WINDOW *window2;
    WINDOW *ok_win;

    int key;
    int win = 0;
    int lose = 0;
    double percent = 0.0;
    int player_num = 0;
    char id1[64];
    char id2[64];

    curs_set(0);

    window1 = newwin(18, 80, 0, 0);
    window2 = newwin(6, 80, 18, 0);

    wbkgd(window1, COLOR_PAIR(1));
    wbkgd(window2, COLOR_PAIR(2));

    int callnum = 3; //waiting room
    Player_info Player1;
    Player_info Player2;
    send(sockfd, &callnum, sizeof(int), 0); //what kind of function

    Player_info temp_player[2] = {0};
    while(1) {
        recv(sockfd, &player_num, sizeof(int), 0); //how many players
        if (player_num == 1) {
            temp_mim++;
            send(sockfd, id, (size_t)64, 0); //player id

            recv(sockfd, &temp_player[0], sizeof(Player_info), 0);//struct of player info
            strcpy(temp_player[0].id, id);

            mvwprintw(window1, 5, 9, "PLAYER1 ID : ");
            wprintw(window1, "%s", id);
            mvwprintw(window1, 7, 14, "STATISTICS");
            mvwprintw(window1, 9, 5, "WIN : %d / LOSE : %d / (%.3f%%)", temp_player[0].win, temp_player[0].lose, temp_player[0].percent);

            mvwprintw(window1, 5, 49, "PLAYER2 ID : ");
            mvwprintw(window1, 7, 53, "STATISTICS");
            mvwprintw(window1, 9, 45, "WIN :  / LOSE :  / (     %%)");
            mvwprintw(window2, 3, 39, "OK");
            wmove(window2, 3, 15);

            wrefresh(window1);
            wrefresh(window2);
            wrefresh(ok_win);
        }
        else if (player_num == 2 && temp_mim != 0) {//1p
            recv(sockfd, temp_player, sizeof(Player_info)*2, 0);
            win = temp_player[1].win;
            lose = temp_player[1].lose;
            percent = temp_player[1].percent;
            strcpy(id2, temp_player[1].id);

            mvwprintw(window1, 5, 63, "%s", id2);
            mvwprintw(window1, 9, 45, "WIN : %d / LOSE : %d / (%.3f%%)", win, lose, percent);
            wmove(window2, 3, 15);

            wrefresh(window1);
            wrefresh(window2);
            wrefresh(ok_win);

            break;
        }
        else if (player_num == 2) {
            strcpy(id2, id);
            send(sockfd, &id2, (size_t) 64, 0); //player id
            recv(sockfd, &Player2, sizeof(Player_info), 0);//struct of player info
            recv(sockfd, &temp_player, sizeof(Player_info)*2, 0);

            mvwprintw(window1, 5, 9, "PLAYER1  ID : ");
            wprintw(window1, "%s", temp_player[0].id);
            mvwprintw(window1, 7, 14, "STATISTICS");
            mvwprintw(window1, 9, 5, "WIN : %d / LOSE : %d / (%.3f%%)", temp_player[0].win, temp_player[0].lose, temp_player[0].percent);

            mvwprintw(window1, 5, 49, "PLAYER2 ID : ");
            wprintw(window1, "%s", temp_player[1].id);
            mvwprintw(window1, 7, 53, "STATISTICS");
            mvwprintw(window1, 9, 45, "WIN : %d / LOSE : %d / (%.3f%%)", temp_player[1].win, temp_player[1].lose, temp_player[1].percent);
            mvwprintw(window2, 3, 39, "OK");
            wmove(window2, 3, 15);

            wrefresh(window1);
            wrefresh(window2);
            wrefresh(ok_win);

            break;
        }
    }

    ok_win = newwin(1, 2, 21, 39);
    wbkgd(window2, COLOR_PAIR(2));
    wbkgd(ok_win, COLOR_PAIR(1));
    wprintw(ok_win, "OK");

    wrefresh(window1);
    wrefresh(window2);
    wrefresh(ok_win);

    keypad(stdscr, true);
    key = getch();
    if (key == ENTER)
        return game_play(temp_player[0], temp_player[1]);
}

void withdrawal(char *id)
{
    initscr();
    refresh();

    WINDOW *window1;
    WINDOW *window2;
    WINDOW **items;
    items = (WINDOW **)malloc(3 * sizeof(WINDOW *));

    curs_set(0);

    window1 = newwin(18, 80, 0, 0);
    window2 = newwin(6, 80, 18, 0);

    wbkgd(window1, COLOR_PAIR(1));
    wbkgd(window2, COLOR_PAIR(2));

    mvwprintw(window1, 5, 35, "WITHDRAWAL");
    mvwprintw(window1, 7, 31, "PLAYER ID : ");
    wprintw(window1, "%s", id);
    mvwprintw(window1, 9, 30, "PASSWORD : ");
    mvwprintw(window2, 3, 15, "WITHDRAWAL");
    mvwprintw(window2, 3, 58, "BACK");
    wmove(window2, 3, 15);

    items[0] = window2;
    items[1] = newwin(1, 10, 21, 15);
    items[2] = newwin(1, 4, 21, 58);
    wprintw(items[1], "WITHDRAWAL");
    wprintw(items[2], "BACK");

    wrefresh(window1);
    wrefresh(items[0]);

    Player_info Player1;

    char one_id;
    char one_pw;
    char next_line = '\n';
    char zero = '0';
    int i = 0;
    ssize_t rsize = 0;
    int cnt = 0;
    int key;
    int check_info = 0;
    char password[64];
    initscr();
    curs_set(1);
    memset(password, '\0', 30);

    wmove(window1, 9, 41);
    wrefresh(window1);
    while (1)
    {
        noecho(); // later echo()
        one_pw = getch();
        if (one_pw == ENTER)
            break;
        else if (i >= 1 && (one_pw == KEY_BACKSPACE || one_pw == 7))
        {
            --i;
            id[i] = 0;
            mvwprintw(window1, 9, (41 + i), " ");
            wmove(window1, 9, (41 + i));
            wrefresh(window1);
        }
        else if (i < 1 && (one_pw == KEY_BACKSPACE || one_pw == 7))
        {
            noecho();
        }
        else
        {
            password[i] = one_pw;
            echo();
            mvwprintw(window1, 9, (41 + i), "*");
            ++i;
            wrefresh(window1);
        }
    }
    password[i] = '\0';

    int index = 0;
    wbkgd(items[1], COLOR_PAIR(1));
    wrefresh(items[1]);
    keypad(stdscr, true);

    curs_set(0);
    while (1)
    {
        int key = getch();
        wrefresh(window1);

        for (int i = 0; i < 3; i++)
            wrefresh(items[i]);

        if (key == KEY_RIGHT)
        {
            ++index;
        }
        else if (key == KEY_LEFT)
        {
            --index;
        }

        if (index == 2)
            index = 0;
        else if (index == -1)
            index = 1;

        // print screen
        for (int i = 1; i < 3; i++)
        {
            if (i == index + 1)
                wbkgd(items[i], COLOR_PAIR(1));
            else
                wbkgd(items[i], COLOR_PAIR(2));
            for (int i = 1; i < 3; i++)
                wrefresh(items[i]);
        }

        if (key == ENTER && index == 0) //withdrawal
        {
            callnum = 2;
            memset(&Player1, 0x00, sizeof(Player1));
            strcpy(Player1.id, id);
            strcpy(Player1.password, password);
            Player1.lose = 0;
            Player1.win = 0;
            send(sockfd, &callnum, sizeof(int), 0);
            send(sockfd, &Player1, sizeof(Player_info), 0);
            recv(sockfd, &check_info, sizeof(int), 0);

            free(items);
            if (check_info == 1) //success
            {
                mvwprintw(window2, 5, 0, ">>> Correct information! (Please any key...)");
                wrefresh(window2);
                getch();
                return init_main_menu(); //edit
            }
            else //fail
            {
                mvwprintw(window2, 5, 0, ">>> Wrong information! (Please any key...)");
                wrefresh(window2);
                getch();
                return withdrawal(id);
            }
        }
        else if (key == ENTER && index == 1) // back
        {
            free(items);
            return after_sign_in(id);
        }
    }
}

void game_play(Player_info Player1, Player_info Player2)
{
    WINDOW *window1;
    WINDOW *window2;
    WINDOW *win_exit;
    WINDOW *regame;
    WINDOW *next_turn;
    WINDOW **items;
    items = (WINDOW **)malloc(36 * sizeof(WINDOW *));

    curs_set(0);

    window1 = newwin(24, 59, 0, 0);
    window2 = newwin(24, 21, 0, 59);
    win_exit = newwin(1, 4, 16, 65);
    next_turn = newwin(1, 9, 12, 63);
    regame = newwin(1, 6, 14, 63);

    wbkgd(window1, COLOR_PAIR(1));
    wbkgd(window2, COLOR_PAIR(2));
    mvwprintw(window2, 8, 4, "%s(O) : 0", Player1.id);
    mvwprintw(window2, 9, 4, "%s(X) : 0", Player2.id);
    wmove(window2, 12, 4);
    waddch(window2, 'N' | A_UNDERLINE);
    wprintw(window2, "EXT TURN");
    wmove(window2, 14, 6);
    waddch(window2, 'R' | A_UNDERLINE);
    wprintw(window2, "EGAME");
    mvwprintw(window2, 16, 6, "E");
    waddch(window2, 'X' | A_UNDERLINE);
    wprintw(window2, "IT");
    wrefresh(window1);
    wrefresh(window2);

    int height = 3;
    int width = 5;
    int starty = 5;
    int startx = 17;
    int line = 0;
    int key;

    for (int i = 0; i < 6; i++)
    {
        startx = 17;
        for (int j = 0; j < 6; j++)
        {
            items[line + j] = newwin(height, width, starty, startx);
            startx += 4;
        }
        line += 6;
        starty += 2;
    }
    for (int i = 0; i < 36; i++)
    {
        wbkgd(items[i], COLOR_PAIR(1));
        wborder(items[i], '|', '|', '-', '-', '+', '+', '+', '+');
        wrefresh(items[i]);
    }
    height = 1;
    width = 3;
    starty = 6;
    line = 0;
    WINDOW **new_items;
    new_items = (WINDOW **)malloc(36 * sizeof(WINDOW *));
    for (int i = 0; i < 6; i++)
    {
        startx = 18;
        for (int j = 0; j < 6; j++)
        {
            new_items[line + j] = newwin(height, width, starty, startx);
            startx += 4;
        }
        line += 6;
        starty += 2;
    }
    for (int i = 0; i < 36; i++)
    {
        wbkgd(new_items[i], COLOR_PAIR(1));
    }
    mvwprintw(new_items[14], 0, 0, " O ");
    wrefresh(new_items[14]);
    mvwprintw(new_items[15], 0, 0, " X ");
    wrefresh(new_items[15]);
    mvwprintw(new_items[20], 0, 0, " X ");
    wrefresh(new_items[20]);
    mvwprintw(new_items[21], 0, 0, " O ");
    wrefresh(new_items[21]);
    wbkgd(new_items[14], COLOR_PAIR(2));
    wrefresh(new_items[14]);

    int row = 2;
    int col = 2;
    int index = 0;
    keypad(stdscr, true);
    curs_set(0);
    while (1)
    {
        key = getch();
        for (int i = 0; i < 36; i++) // init color
        {
            wbkgd(items[i], COLOR_PAIR(1));
            wrefresh(items[i]);
        }
        if (key == 120) //exit 'x'
        {
            for (int i = 0; i < 36; i++)
                wbkgd(new_items[i], COLOR_PAIR(1));

            mvwprintw(new_items[14], 0, 0, " O ");
            wrefresh(new_items[14]);
            mvwprintw(new_items[15], 0, 0, " X ");
            wrefresh(new_items[15]);
            mvwprintw(new_items[20], 0, 0, " X ");
            wrefresh(new_items[20]);
            mvwprintw(new_items[21], 0, 0, " O ");
            wrefresh(new_items[21]);
            wbkgd(win_exit, COLOR_PAIR(1));
            mvwprintw(win_exit, 0, 0, "E");
            waddch(win_exit, 'X' | A_UNDERLINE);
            wprintw(win_exit, "IT");
            wrefresh(win_exit);

            while (1)
            {
                key = getch();
                if (key == ENTER) {
                    if(temp_mim == 1) { //ip
                        return after_sign_in(Player1.id);
                    }
                    else if(temp_mim == 0) {
                        return after_sign_in(Player2.id);
                    }
                }
                else if (key == 103) // 'g'
                    break;
            }

            wbkgd(win_exit, COLOR_PAIR(2));
            wrefresh(win_exit);
            row = 2;
            col = 2;
        }
        else if(key == 110) //n
        {
            for (int i = 0; i < 36; i++)
                wbkgd(new_items[i], COLOR_PAIR(1));

            mvwprintw(new_items[14], 0, 0, " O ");
            wrefresh(new_items[14]);
            mvwprintw(new_items[15], 0, 0, " X ");
            wrefresh(new_items[15]);
            mvwprintw(new_items[20], 0, 0, " X ");
            wrefresh(new_items[20]);
            mvwprintw(new_items[21], 0, 0, " O ");
            wrefresh(new_items[21]);
            wbkgd(next_turn, COLOR_PAIR(1));
            wmove(next_turn, 0, 0);
            waddch(window2, 'N' | A_UNDERLINE);
            wprintw(window2, "EXT TURN");
            wrefresh(next_turn);

            while (1)
            {
                key = getch();
                if (key == 103) // 'g'
                    break;
            }

            wbkgd(next_turn, COLOR_PAIR(2));
            wrefresh(next_turn);
            row = 2;
            col = 2;
        }
        else if(key == 114) //r
        {
            for (int i = 0; i < 36; i++)
                wbkgd(new_items[i], COLOR_PAIR(1));

            mvwprintw(new_items[14], 0, 0, " O ");
            wrefresh(new_items[14]);
            mvwprintw(new_items[15], 0, 0, " X ");
            wrefresh(new_items[15]);
            mvwprintw(new_items[20], 0, 0, " X ");
            wrefresh(new_items[20]);
            mvwprintw(new_items[21], 0, 0, " O ");
            wrefresh(new_items[21]);
            wbkgd(regame, COLOR_PAIR(1));
            wmove(regame, 0, 0);
            waddch(window2, 'R' | A_UNDERLINE);
            wprintw(window2, "EGAME");
            wrefresh(regame);

            while (1)
            {
                key = getch();
                if (key == 103) // 'g'
                    break;
            }

            wbkgd(regame, COLOR_PAIR(2));
            wrefresh(regame);
            row = 2;
            col = 2;
        }
        wbkgd(win_exit, COLOR_PAIR(2));
        mvwprintw(win_exit, 0, 0, "E");
        waddch(win_exit, 'X' | A_UNDERLINE);
        wprintw(win_exit, "IT");
        wrefresh(win_exit);

        wbkgd(next_turn, COLOR_PAIR(2));
        wmove(next_turn, 0, 0);
        waddch(window2, 'N' | A_UNDERLINE);
        wprintw(window2, "EXT TURN");
        wrefresh(next_turn);

        wbkgd(regame, COLOR_PAIR(2));
        wmove(regame, 0, 0);
        waddch(window2, 'R' | A_UNDERLINE);
        wprintw(window2, "EGAME");
        wrefresh(regame);
        if (key == KEY_RIGHT)
        {
            if (col == 5)
            {
                if (row != 5)
                {
                    ++row;
                }
                else
                {
                    row = 0;
                }
                col = 0;
            }
            else
                ++col;
        }
        if (key == KEY_LEFT)
        {
            if (col == 0)
            {
                if (row != 0)
                {
                    --row;
                }
                else
                {
                    row = 5;
                }
                col = 5;
            }
            else
                --col;
        }
        if (key == KEY_UP)
        {
            if (row == 0)
                row = 5;
            else
                --row;
        }
        if (key == KEY_DOWN)
        {
            if (row == 5)
                row = 0;
            else
                ++row;
        }
        for (int i = 0; i < 36; i++)
        {
            wbkgd(new_items[i], COLOR_PAIR(1));
        }
        mvwprintw(new_items[14], 0, 0, " O ");
        wrefresh(new_items[14]);
        mvwprintw(new_items[15], 0, 0, " X ");
        wrefresh(new_items[15]);
        mvwprintw(new_items[20], 0, 0, " X ");
        wrefresh(new_items[20]);
        mvwprintw(new_items[21], 0, 0, " O ");
        wrefresh(new_items[21]);
        index = row * 6 + col;
        wbkgd(new_items[index], COLOR_PAIR(2));
        wrefresh(new_items[index]);
    }

    getch();

    endwin();
}

int sign_up_in(WINDOW *window1, WINDOW *window2, char *id, char *password, int kind)
{
    initscr();
    refresh();

    wbkgd(window1, COLOR_PAIR(1));
    wbkgd(window2, COLOR_PAIR(2));

    if (kind == 1) // sign up
        mvwprintw(window1, 5, 36, "SIGN UP");
    else if (kind == 2) // sign in
        mvwprintw(window1, 5, 36, "SIGN IN");

    mvwprintw(window1, 7, 31, "ID : ");
    mvwprintw(window1, 9, 25, "PASSWORD : ");
    mvwprintw(window2, 3, 58, "BACK");

    WINDOW **items;
    items = (WINDOW **)malloc(3 * sizeof(WINDOW *));

    items[0] = window2;
    items[1] = newwin(1, 7, 21, 16);
    items[2] = newwin(1, 4, 21, 58);
    wbkgd(items[0], COLOR_PAIR(2));
    wbkgd(items[1], COLOR_PAIR(2));
    if (kind == 1)
        wprintw(items[1], "SIGN UP");
    else if (kind == 2)
        wprintw(items[1], "SIGN IN");
    wprintw(items[2], "BACK");

    wrefresh(window1);
    wrefresh(items[0]);
    wrefresh(items[1]);

    char one_id;
    char one_pw;
    char next_line = '\n';
    char zero = '0';
    int i = 0;
    initscr();
    curs_set(1);
    wmove(window1, 7, 36);
    wrefresh(window1);
    memset(id, '\0', 30);
    memset(password, '\0', 30);
    while (1)
    {
        noecho();
        one_id = getch();
        if (one_id == ENTER)
            break;
        else if (i >= 1 && (one_id == KEY_BACKSPACE || one_id == 7))
        {
            --i;
            id[i] = 0;
            mvwprintw(window1, 7, (36 + i), " ");
            wmove(window1, 7, (36 + i));
            wrefresh(window1);
        }
        else if (i < 1 && (one_id == KEY_BACKSPACE || one_id == 7))
        {
            noecho();
        }
        else
        {
            id[i] = one_id;
            echo();
            mvwprintw(window1, 7, (36 + i), "%c", one_id);
            wrefresh(window1);
            ++i;
        }
    }
    id[i] = '\0';
    wmove(window1, 9, 36);
    wrefresh(window1);
    i = 0; // init i
    while (1)
    {
        noecho(); // later echo()
        one_pw = getch();
        if (one_pw == ENTER)
            break;
        else if (i >= 1 && (one_pw == KEY_BACKSPACE || one_pw == 7))
        {
            --i;
            id[i] = 0;
            mvwprintw(window1, 9, (36 + i), " ");
            wmove(window1, 9, (36 + i));
            wrefresh(window1);
        }
        else if (i < 1 && (one_pw == KEY_BACKSPACE || one_pw == 7))
        {
            noecho();
        }
        else
        {
            password[i] = one_pw;
            echo();
            mvwprintw(window1, 9, (36 + i), "*");
            ++i;
            wrefresh(window1);
        }
    }
    password[i] = '\0';

    int index = 0;
    wbkgd(items[1], COLOR_PAIR(1));
    wrefresh(items[1]);
    keypad(stdscr, true);

    curs_set(0);
    while (1)
    {
        int key = getch();
        wrefresh(window1);

        for (int i = 0; i < 3; i++)
            wrefresh(items[i]);

        if (key == KEY_RIGHT)
        {
            ++index;
        }
        else if (key == KEY_LEFT)
        {
            --index;
        }

        if (index == 2)
            index = 0;
        else if (index == -1)
            index = 1;

        // print screen
        for (int i = 1; i < 3; i++)
        {
            if (i == index + 1)
                wbkgd(items[i], COLOR_PAIR(1));
            else
                wbkgd(items[i], COLOR_PAIR(2));
            for (int i = 1; i < 3; i++)
                wrefresh(items[i]);
        }

        if (key == ENTER) //call another function
            break;
    }
    free(items);
    return index;
}

void errorHandler(const char *msg) {
    close(sockfd);

    fputs(msg, stderr);
    exit(-1);
}

void signalHandler(int signum) {
    if (signum == SIGINT) {
        close(sockfd);
        exit(-1);
    }
    if (signum == SIGPIPE) {
        puts("disconnected!");
        exit(-1);
    }
}