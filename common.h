#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>

#define SITE_SIZE 3
#define TYPE_SIZE 2
#define PROGRAM_ARGS 3
#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define MAX_MSG_SIZE 13

typedef struct {
    char* type;
    int limit;
    char* players;
} Site;

typedef struct {
    char id;
    int position;
    pid_t pid;
    int money;
    int v1;
    int v2;
    int points;
    int a;
    int b;
    int c;
    int d;
    int e;
    FILE* in;
    FILE* out;
} Player;

typedef struct {
    Site* sites;
    Player* players;
    char* deck;
    int numPlayers;
    int pathSize;
    bool sighup;
} Game;

typedef enum {
    YT,
    EARLY,
    DONE,
    HAP
} DealerMessage;

#endif
