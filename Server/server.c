#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_LQ_SIZE 5
#define MAX_NAME_SIZE 64
#define MAX_BUF_SIZE 1024
#define PERMS 0666
#define FILENAME "./2017203028.txt"

void errorHandler(const char *msg);
void signalHandler(int signum);
void *my_func(int *fd);
void delete_info(char* id);
void waiting_room(char *id, int sockfd);
int check_id(char *id);
int check_pw(char *id, char *password);
int file_open(void);
int sign_up(char *id);
int sockfd = 0;
int clntSock[2];
int callnum = 4;
int count;
int cnt_connect = 0;
int player_num = 0;
int client_fd[2];
pthread_t tid[2];

typedef struct {
    char id[64];
    char password[64];
    int win;
    int lose;
    double percent;
}Player_info;

Player_info temp_player[2];

Player_info Player;
Player_info Player1;
Player_info Player2;

int main(int argc, char const *argv[]) {
    struct sockaddr_in servAddr = { 0x00, };
    struct sockaddr_in clntAddr = { 0x00, };
    int clntAddrSize = 0;
    int overlap = 2;
    int check_info = 2;
    int fd = 0;
    char id[64];
    char password[64];
    char next_line = '\n';
    char zero = '0';

    signal(SIGINT, signalHandler);
    signal(SIGPIPE, signalHandler);

    if (argc < 2) {
        printf("Usage: %s [port]\n", argv[0]);
        return -1;
    }

    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        errorHandler("sock() error!");
    }

    servAddr.sin_family = PF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(atoi(argv[1]));
    if (bind(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr))) {
        errorHandler("bind() error!");
    }
    if (listen(sockfd, MAX_LQ_SIZE) == -1) {
        errorHandler("listen() error!");
    }
    count = 0;
    while(1) {
        if ((clntSock[count] = accept(sockfd, (struct sockaddr *)&clntAddr, &clntAddrSize)) == -1) {
            errorHandler("accept() error!");
        }
        else {
            printf("CONNECT: %d\n", ++cnt_connect);
        }
        pthread_create(&tid[count], NULL, (void*)my_func, &clntSock[count]);
        ++count;
    }
    return 0;
}

void *my_func(int *count) {
    int overlap = 2;
    int check_info = 2;
    int fd = 0;
    char id[64];
    char password[64];
    char next_line = '\n';
    char zero = '0';
    Player_info Player;

    while(1) {
            recv(*count, &callnum, sizeof(int), 0); //what kind of function
            memset(&Player, 0x00, sizeof(Player));
            memset(id, 0x00, sizeof(id));
            memset(password, 0x00, sizeof(password));
            memset(password, 0x00, sizeof(password));
            if(callnum == 0) {  //sign_up
                recv(*count, &Player, sizeof(Player_info), 0);
                strcpy(id, Player.id);
                strcpy(password, Player.password);
                overlap = check_id(id);  //check overlap
                send(*count, &overlap, sizeof(int), 0);

                fd = file_open();
                if(overlap == 0) //no overlap
                {
                    write(fd, (char *)&next_line, 1);
                    write(fd, (char *)id, strlen(id));
                    write(fd, (char *)&next_line, 1);
                    write(fd, (char *)password, strlen(password));
                    for (int i = 0; i < 3; i++) // later change -> game win/lose
                    {
                        write(fd, (char *)&next_line, 1);
                        write(fd, (char *)&zero, 1);
                    }
                    write(fd, (char *)&next_line, 1);
                    printf("SIGN UP: %s\n", id);
                }
                close(fd);
            }
            else if(callnum == 1) {//sign_in
                fd = file_open();
                recv(*count, &Player, sizeof(Player_info), 0);
                strcpy(id, Player.id);
                strcpy(password, Player.password);

                if((check_id(id) == 1) && (check_pw(id, password) == 1)) {
                    check_info = 1;
                    printf("SIGN IN: %s\n", id);
                }
                else
                    check_info = 0;

                if(player_num == 1 && temp_player[0].id == id) {
                    check_info = 2; // overlap with player1
                }

                send(*count, &callnum, sizeof(int), 0); //send for what type
                send(*count, &check_info, sizeof(check_info), 0);
                close(fd);
            }
            else if(callnum == 2) {
                fd = file_open();
                recv(*count, &Player, sizeof(Player_info), 0);
                strcpy(id, Player.id);
                strcpy(password, Player.password);

                check_info = check_pw(id, password);
                send(*count, &callnum, sizeof(int), 0); //send for what type
                send(*count, &check_info, sizeof(int), 0);
                if(check_info == 1) {
                    delete_info(id);
                    printf("WITHDRAWAL: %s\n", id);
                }
                close(fd);
            }
            else if(callnum == 3) {
                client_fd[player_num] = *count;
                player_num++;
                for(int k = 0 ; k < player_num ; k++) {
                    send(client_fd[k], &player_num, sizeof(int), 0);//1
                }
                char temp_id[64];
                recv(*count, temp_id, (size_t)64, 0); //what?? //2


                waiting_room(temp_id, *count);

                if(player_num == 2)
                {
                    send(client_fd[0], &temp_player, sizeof(Player_info) * 2 , 0);
                    send(client_fd[1], &temp_player, sizeof(Player_info) * 2 , 0);
                }
            }
            else if(callnum == -1) {

                printf("CONNECT: %d\n", --cnt_connect);
                pthread_exit(0);
            }
    }
}

void waiting_room(char *id, int sockfd) {
    char id_word;
    char pw_word;
    char win_word;
    char lose_word;
    char per_word;
    ssize_t rsize = 0;
    int cnt = 0;
    int fd = 0;
    int key;
    char buf[30] = {
            '\0',
    };
    char win[30] = {
            '\0',
    };
    char lose[30] = {
            '\0',
    };
    char percent[30] = {
            '\0',
    };

    fd = file_open();
    do
    {
        rsize = read(fd, (char *)&id_word, 1);
        if (rsize == -1)
        {
            perror("read() error!");
            exit(-2);
        }

        if ((cnt - 1) % 6 == 0)
        {
            int i = 0;
            buf[i++] = id_word;
            read(fd, (char *)&id_word, 1);
            buf[i++] = id_word;
            while (id_word != '\n')
            {
                read(fd, (char *)&id_word, 1);
                buf[i] = id_word;
                i++;
            }
            buf[--i] = '\0';

            if (strcmp(id, buf) == 0)
            {
                int j = 0;
                read(fd, (char *)&pw_word, 1);
                while (pw_word != '\n')
                {
                    read(fd, (char *)&pw_word, 1);
                }
                read(fd, (char *)&win_word, 1);
                j = 0;
                win[j++] = win_word;
                while (win_word != '\n')
                {
                    read(fd, (char *)&win_word, 1);
                    win[j] = win_word;
                    j++;
                }
                win[j - 1] = '\0';
                read(fd, (char *)&lose_word, 1);
                j = 0;
                lose[j++] = lose_word;
                lose[j] = '\0';
                while (lose_word != '\n')
                {
                    read(fd, (char *)&lose_word, 1);
                    lose[j] = lose_word;
                    j++;
                }
                lose[j - 1] = '\0';
                read(fd, (char *)&per_word, 1);
                j = 0;
                percent[j++] = per_word;
                percent[j] = '\0';
                while (per_word != '\n')
                {
                    read(fd, (char *)&per_word, 1);
                    percent[j] = per_word;
                    j++;
                }
                percent[j - 1] = '\0';
            }
            memset(buf, 0, 30);
            cnt++;
        }
        else if (id_word == '\n')
        {
            ++cnt;
        }
    } while (rsize > 0);

    close(fd);
    for(int k = 0 ; k < player_num; k++)
    {
        if(sockfd == client_fd[k])
        {
            strcpy(temp_player[k].id, id);
            temp_player[k].win = atoi(win);
            temp_player[k].lose = atoi(lose);
            temp_player[k].percent = atoi(percent);
            send(sockfd, &temp_player[k], sizeof(Player_info), 0);
            break;
        }
    }
}

void delete_info(char* id) {
    char buf[30] = {
            '\0',
    };
    char word;
    char space = ' ';
    ssize_t rsize = 0;
    int cnt = 0;
    int fd = 0;
    char temp[30];
    int size;

    fd = open(FILENAME, O_CREAT | O_RDWR, PERMS);
    do
    {
        rsize = read(fd, (char *)&word, 1);

        if (rsize == -1)
        {
            perror("read() error!");
            exit(-2);
        }

        if ((cnt - 1) % 6 == 0)
        {
            int i = 0;
            buf[i++] = word;
            read(fd, (char *)&word, 1);
            buf[i++] = word;
            while (word != '\n')
            {
                read(fd, (char *)&word, 1);
                buf[i] = word;
                i++;
            }
            buf[--i] = '\0';

            if (strcmp(id, buf) == 0)
            {
                lseek(fd, (strlen(id) + 1) * -1, SEEK_CUR);
                for (int i = 0; i < strlen(id); i++)
                    write(fd, &space, 1);
                break;
            }
            memset(buf, 0, 30);
            cnt++;
        }
        else if (word == '\n')
        {
            ++cnt;
        }
    } while (rsize > 0);
    close(fd);
}

void errorHandler(const char *msg) {
    close(clntSock[count]);
    close(sockfd);
    fputs(msg, stderr);
    exit(-1);
}

void signalHandler(int signum) {
    if (signum == SIGINT) {
        close(clntSock[0]);
        close(clntSock[1]);
        exit(-1);
    }

    if (signum == SIGPIPE) {
        puts("disconnected!");
    }
}

int check_id(char *id)
{
    char buf[30] = {
            '\0',
    };
    char word;
    ssize_t rsize = 0;
    int cnt = 0;
    int fd = 0;

    fd = file_open();
    do
    {
        rsize = read(fd, (char *)&word, 1);

        if (rsize == -1)
        {
            perror("read() error!");
            exit(-2);
        }

        if ((cnt - 1) % 6 == 0)
        {
            int i = 0;
            buf[i++] = word;
            read(fd, (char *)&word, 1);
            buf[i++] = word;
            while (word != '\n')
            {
                read(fd, (char *)&word, 1);
                buf[i] = word;
                i++;
            }
            buf[--i] = '\0';

            if (strcmp(id, buf) == 0)
            {
                close(fd);
                return 1;
            }
            memset(buf, 0, 30);
            cnt++;
        }
        else if (word == '\n')
        {
            ++cnt;
        }
    } while (rsize > 0);

    close(fd);
    return 0;
}

int check_pw(char *id, char *password)
{
    char buf[30] = {
            '\0',
    };
    char pw_buf[30] = {
            '\0',
    };
    char id_word;
    char pw_word;
    ssize_t rsize = 0;
    int cnt = 0;
    int fd = 0;

    fd = file_open();
    do
    {
        rsize = read(fd, (char *)&id_word, 1);

        if (rsize == -1)
        {
            perror("read() error!");
            exit(-2);
        }

        if ((cnt - 1) % 6 == 0)
        {
            int i = 0;
            buf[i++] = id_word;
            read(fd, (char *)&id_word, 1);
            buf[i++] = id_word;
            while (id_word != '\n')
            {
                read(fd, (char *)&id_word, 1);
                buf[i] = id_word;
                i++;
            }
            buf[--i] = '\0';

            if (strcmp(id, buf) == 0)
            {
                int j = 0;
                read(fd, (char *)&pw_word, 1);
                pw_buf[j++] = pw_word;
                while (pw_word != '\n')
                {
                    read(fd, (char *)&pw_word, 1);
                    pw_buf[j++] = pw_word;
                }
                pw_buf[--j] = '\0';
                close(fd);
                if (strcmp(password, pw_buf) == 0)
                    return 1;
                else
                    return 0;
            }
            memset(buf, 0, 30);
            cnt++;
        }
        else if (id_word == '\n')
        {
            ++cnt;
        }
    } while (rsize > 0);

    close(fd);
    return 0;
}

int file_open(void)
{
    int fd = 0;

    fd = open(FILENAME, O_CREAT | O_APPEND | O_RDWR, PERMS);
    if (fd == -1)
    {
        perror("open() error!");
        exit(-1);
    }
    return fd;
}