#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

/*
 * Represents the path made up of sites
 * */
typedef struct {
    int pathSize;
    Site* sites;
} Path;

void check_arguments(int argc, char** argv);
void read_path(Path* path, int id, int pCount);
bool valid_path(Site* sites, int pathSize, int pCount);
char** initialise_board(Path* path, int pCount);
void update_board(char** board, Path* path, int pCount);
void display_board(char** board, Path* path, int pCount);
int play_move(char** board, Path* path, Player* players, int ID, int pCount);
void send_message(int site);
DealerMessage receive_message(Path* path, Player* players, int pCount);
Player* initialise_players(int pCount);
int v_site(Path* path, Player* players, int id);
int barrier_site(Path* path, Player* players, int id);
int do_site(Path* path, Player* players, int id);
int mo_site(Path* path, Player* players, int id);
void handle_move(Path* path, Player* players, int id, int site, int points, 
	int money, int card);
bool full_site(Path* path, int site);
void print_scores(Player* players, int pCount);
int card_score(Player* players, int id);

int main(int argc, char** argv) {
    check_arguments(argc, argv);
    int pCount = atoi(argv[1]), id = atoi(argv[2]);
    Path* path = (Path*)malloc(sizeof(Path));
    Player* players = initialise_players(pCount);

    fprintf(stdout, "^");
    fflush(stdout);

    read_path(path, id, pCount);
    char** board = initialise_board(path, pCount);
    display_board(board, path, pCount);

    while (1) {
        DealerMessage message = receive_message(path, players, pCount);
        if (message == YT) {
            int site = play_move(board, path, players, id, pCount);
            send_message(site);          
        } else if (message == HAP) {
            update_board(board, path, pCount);
        } else {
            break;
        }
    }

    print_scores(players, pCount);

    return 0;
}

/*
 * Initialise players depending on the number of players specified
 * Returns an array of player structs
 * */
Player* initialise_players(int pCount) {
    int i;
    Player* players = (Player*)malloc(sizeof(Player) * pCount);

    for (i = 0; i < pCount; i++) {
        Player player;
        player.id = i;
        player.position = 0;
        player.money = 7;
        player.v1 = 0;
        player.v2 = 0;
        player.points = 0;
        player.a = 0;
        player.b = 0;
        player.c = 0;
        player.d = 0;
        player.e = 0;
        players[i] = player;
    }

    return players;
}

/*
 * Ensure that the arguments provided are valid
 * Exit if there aren't enough arguments or the player count or id is invalid
 * */
void check_arguments(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: player pcount ID\n");
        exit(1);
    }

    int pCount = atoi(argv[1]), id;
    
    if (pCount < 1) {
        fprintf(stderr, "Invalid player count\n");
        exit(2);
    }

    if (!strcmp(argv[2], "0")) {
        id = 0;
    } else {
        id = atoi(argv[2]);
        if (id <= 0 || id > pCount) {
            fprintf(stderr, "Invalid ID\n");
            exit(3);
        }
    }
}

/*
 * Read the path from STDIN and create the sites that players can move to
 * Exit if the path entered is invalid
 * */
void read_path(Path* path, int id, int pCount) {
    int c, i = 0, j = 0, k;
    char dummy;

    if (fscanf(stdin, "%d%c", &(path->pathSize), &dummy) != 2 || dummy != ';' 
	    || path->pathSize < 2) {
        fprintf(stderr, "Invalid path\n");
        exit(4);
    }

    char* buffer = (char*)malloc(sizeof(char) * path->pathSize * 
	    SITE_SIZE + 1);
    while (c = fgetc(stdin), c != '\n' && c != EOF) {
        buffer[i] = c;
        i++;
    }
    
    path->sites = (Site*)malloc(sizeof(Site) * path->pathSize);

    for (i = 0; i < path->pathSize * SITE_SIZE; i += SITE_SIZE) {
        Site site;
        char* siteType = (char*)malloc(sizeof(char) * TYPE_SIZE);
        strncpy(siteType, buffer + i, TYPE_SIZE);
        site.type = siteType;
        site.type[TYPE_SIZE] = '\0';
        if (buffer[i + TYPE_SIZE] == '-' || (buffer[i + TYPE_SIZE] - '0') > 
		pCount) {
            site.limit = pCount;
        } else {
            site.limit = buffer[i + TYPE_SIZE] - '0';
        }        
        site.players = (char*)malloc(sizeof(char) * site.limit);
        for (k = 0; k < site.limit; k++) {
            site.players[k] = ' ';
        }
        path->sites[j] = site;
        j++;
    }

    if (!valid_path(path->sites, path->pathSize, pCount)) {
        fprintf(stderr, "Invalid path\n");
        exit(4);
    }
}

/*
 * Check to see if the path entered is valid
 * Return true if the path is valid or false if invalid
 * */
bool valid_path(Site* sites, int pathSize, int pCount) {
    int i;
    for (i = 0; i < pathSize; i++) {
        if ((strcmp(sites[i].type, "::") && strcmp(sites[i].type, "Mo") && 
		strcmp(sites[i].type, "V1") && strcmp(sites[i].type, "V2") && 
		strcmp(sites[i].type, "Do") && strcmp(sites[i].type, "Ri")) || 
		!isdigit(sites[i].limit + '0')) {
            return false;
        }
    }

    if (strcmp(sites[0].type, "::") || strcmp(sites[pathSize - 1].type, "::") 
            || sites[0].limit != pCount ||
            sites[pathSize - 1].limit != pCount) {
        return false;
    }

    return true;
}

/*
 * Initialise the board and positions that the players can occupy
 * Return the two-dimensional array of chars for the positions
 * */
char** initialise_board(Path* path, int pCount) {
    int r, c;
    char** board = (char**)malloc(sizeof(char*) * pCount);

    for (r = 0; r < pCount; r++) {
        board[r] = (char*)malloc(sizeof(char) * (path->pathSize * 
		SITE_SIZE + 1));
        for (c = 0; c < path->pathSize * SITE_SIZE + 1; c++) {
            if (c == path->pathSize * SITE_SIZE) {
                board[r][c] = '\n';
            } else {
                board[r][c] = ' ';
            }
        }
    }

    char player = (pCount - 1) + '0';
    for (r = 0; r < pCount; r++) {
        path->sites[0].players[r] = player;
        board[r][0] = player;
        player--;
    }

    return board;
}

/*
 * Update the board and positions after a player has made a move
 * */
void update_board(char** board, Path* path, int pCount) {
    int i = 0, r, c, j;

    for (r = 0; r < pCount; r++) {
        for (c = 0; c < path->pathSize * SITE_SIZE + 1; c++) {
            if (c == path->pathSize * SITE_SIZE) {
                board[r][c] = '\n';
            } else {
                board[r][c] = ' ';
            }
        }
    }

    for (i = 0; i < path->pathSize; i++) {
        if (path->sites[i].players[0] != ' ') {
            for (j = 0; j < path->sites[i].limit; j++) {
                board[j][i * SITE_SIZE] = path->sites[i].players[j];
            }
        }
    }

    display_board(board, path, pCount);
}

/*
 * Print the board and positions that the players occupy to SDERR
 * */
void display_board(char** board, Path* path, int pCount) {
    int i, r, c, count = 0;

    for (r = 0; r < pCount; r++) {
        for (c = 0; c < path->pathSize * SITE_SIZE; c++) {
            if (board[r][c] != ' ' && board[r][c] != '\n') {
                count++;
                break;
            }
        }
    }

    for (i = 0; i < path->pathSize; i++) {
        if (i == path->pathSize - 1) {
            fprintf(stderr, "%s \n", path->sites[i].type);
        } else {
            fprintf(stderr, "%s ", path->sites[i].type);
        }
    }

    for (r = 0; r < count; r++) {
        for (c = 0; c < path->pathSize * SITE_SIZE + 1; c++) {
            fprintf(stderr, "%c", board[r][c]);
        }
    }
}

/*
 * Send a message to STDOUT with the site that the player has chosen to move to
 * */
void send_message(int site) {
    fprintf(stdout, "DO%d\n", site);
    fflush(stdout);
}

/*
 * Read a message from STDIN
 * Return the message received on success or exit if there was a communications
 * error
 * */
DealerMessage receive_message(Path* path, Player* players, int pCount) {
    int c, i = 0;
    char* buffer = (char*)malloc(sizeof(char) * MAX_MSG_SIZE);
    DealerMessage message;

    while (c = fgetc(stdin), c != '\n' && c != EOF) {
        buffer[i] = c;
        i++;
    }

    switch (buffer[0]) {
        case 'Y':
            if (strcmp(buffer, "YT")) {
                fprintf(stderr, "Communications error\n");
                exit(6);
            }
            message = YT;
            break;
        case 'E':
            if (strcmp(buffer, "EARLY")) {
                fprintf(stderr, "Communications error\n");
                exit(6);
            }
            message = EARLY;
            break;
        case 'D':
            if (strcmp(buffer, "DONE")) {
                fprintf(stderr, "Communications error\n");
                exit(6);
            }            
            message = DONE;
            break;
        case 'H':
            message = HAP;
            int pID, site, points, money, card;
            if (sscanf(buffer, "HAP%d,%d,%d,%d,%d", &pID, &site, &points, 
		    &money, &card) != 5 || card > 5 || card < 0 || 
		    site >= path->pathSize) {
                fprintf(stderr, "Communications error\n");
                exit(6);
            }
            handle_move(path, players, pID, site, points, money, card); 
            break;
        default:
            fprintf(stderr, "Communications error\n");
            exit(6);
            break; 
    }

    return message;
}

/*
 * Carry out a move that has been given to the player
 * Handles moves made via the HAP message
 * */
void handle_move(Path* path, Player* players, int id, int site, int points, 
	int money, int card) {
    int currentSite = players[id].position, i, j;

    players[id].points += points;
    players[id].money += money;
    if (!strcmp(path->sites[site].type, "V1")) {
        players[id].v1++;
    }
    if (!strcmp(path->sites[site].type, "V2")) {
        players[id].v2++;
    }
    if (card == 1) {
        players[id].a++;
    } else if (card == 2) {
        players[id].b++;
    } else if (card == 3) {
        players[id].c++;
    } else if (card == 4) {
        players[id].d++;
    } else if (card == 5) {
        players[id].e++;
    } else {
        //
    }
    for (i = 0; i < path->sites[currentSite].limit; i++) {
        if (path->sites[currentSite].players[i] == id + '0') {
            for (j = i; j < path->sites[currentSite].limit; j++) {
                path->sites[currentSite].players[j] = 
			path->sites[currentSite].players[j + 1];
            }
            path->sites[currentSite].players[path->sites[currentSite].limit 
		    - 1] = ' ';
        }
    }

    for (i = 0; i < path->sites[site].limit; i++) {
        if (path->sites[site].players[i] == ' ') { 
            path->sites[site].players[i] = id + '0';
            break;
        }
    }

    players[id].position = site;
    fprintf(stderr, 
	    "Player %d Money=%d V1=%d V2=%d Points=%d A=%d B=%d C=%d "
	    "D=%d E=%d\n", id, players[id].money, players[id].v1, 
	    players[id].v2, players[id].points, players[id].a, 
            players[id].b, players[id].c, players[id].d, players[id].e);
}

/*
 * Decide on a move based on the characteristics of this player
 * Return the site that the player has chosen to move to
 * */
int play_move(char** board, Path* path, Player* players, int id, int pCount) {
    int i, j, nextSite, currentSite = players[id].position;

    if (players[id].money != 0 && do_site(path, players, id) && 
	    !full_site(path, do_site(path, players, id))) {
        nextSite = do_site(path, players, id);
    } else if (mo_site(path, players, id) && !full_site(path, 
	    currentSite + 1)) {
        nextSite = currentSite + mo_site(path, players, id);
    } else {
        nextSite = v_site(path, players, id);
    }

    for (i = 0; i < path->sites[currentSite].limit; i++) {
        if (path->sites[currentSite].players[i] == id + '0') {
            for (j = i; j < path->sites[currentSite].limit; j++) {
                path->sites[currentSite].players[j] = 
		        path->sites[currentSite].players[j + 1];
            }
            path->sites[currentSite].players[path->sites[currentSite].limit 
	            - 1] = ' ';
        }
    }

    for (i = 0; i < path->sites[nextSite].limit; i++) {
        if (path->sites[nextSite].players[i] == ' ') {
            path->sites[nextSite].players[i] = id + '0';
            break;
        }
    }

    players[id].position = nextSite;
    return nextSite; 
}

/*
 * Check to see if there is a valid V site that the player can move to
 * Return the position of the V site on sucess or 0 if there is not a
 * valid V site
 * */
int v_site(Path* path, Player* players, int id) {
    int i;
    for (i = players[id].position + 1; i < path->pathSize; i++) {
        if (!strcmp(path->sites[i].type, "V1") || !strcmp(path->sites[i].type, 
		"V2") || !strcmp(path->sites[i].type, "::")) {
            if (!full_site(path, i)) {
                return i;
            }
        }
    }

    return 0;
}

/*
 * Check to see if there is a valid barrier site for the player to move to
 * Return the position of the closest barrier site
 * */
int barrier_site(Path* path, Player* players, int id) {
    int i, count = 1, site;
    for (i = players[id].position + 1; i < path->pathSize; i++) {
        if (!strcmp(path->sites[i].type, "::")) {
            break;
        }
        count++;
    }

    site = players[id].position + count;
    return site;
}

/*
 * Check to see if there is a valid Mo site for the player to move to
 * Return 1 if the next site is a valid Mo site or 0 if it is not
 * */
int mo_site(Path* path, Player* players, int id) {
    if (!strcmp(path->sites[players[id].position + 1].type, "Mo")) {
        return 1;
    }

    return 0;
}

/*
 * Check to see if there is a valid Do site for the player to move to
 * Return the position of the valid Do site or 0 if there is not one
 * */
int do_site(Path* path, Player* players, int id) {
    int i;
    for (i = players[id].position + 1; i < path->pathSize; i++) {
        if (!strcmp(path->sites[i].type, "Do")) {
            return i;
        }
    }

    return 0;
}

/*
 * Check to see if the specified site is full
 * Return true if it is full (limit is reached) and false otherwise
 * */
bool full_site(Path* path, int site) {
    int i;
    for (i = 0; i < path->sites[site].limit; i++) {
        if (path->sites[site].players[i] == ' ') {
            return false;
        }
    }

    return true;
}

/*
 * Print the final scores of the players to STDERR at the completion
 * of the game
 * */
void print_scores(Player* players, int pCount) {
    int i;

    fprintf(stderr, "Scores: ");
    for (i = 0; i < pCount; i++) {
        int cardScore = card_score(players, i);
        players[i].points += (players[i].v1 + players[i].v2 + cardScore);
        if (i == pCount - 1) {
            fprintf(stderr, "%d\n", players[i].points);
        } else {
            fprintf(stderr, "%d,", players[i].points);
        }
    }
}

/*
 * Calculates the score a player receives from the cards that they have
 * Return the score that they receive
 * */
int card_score(Player* players, int id) {
    int i, j, score = 0, a = players[id].a, b = players[id].b, 
	    c = players[id].c, d = players[id].d, e = players[id].e;
    char cards[5] = {a, b, c, d, e};
    for (i = 0; i < 5; i++) {
        for (j = i + 1; j < 5; j++) {
            if (cards[j] > cards[i]) {
                int tmp = cards[i];
                cards[i] = cards[j];
                cards[j] = tmp;
            }
        }
    }

    while (cards[0] && cards[1] && cards[2] && cards[3] && cards[4]) {
        cards[0]--;
        cards[1]--;
        cards[2]--;
        cards[3]--;
        cards[4]--;
        score += 10;
    }

    while (cards[0] && cards[1] && cards[2] && cards[3]) {
        cards[0]--;
        cards[1]--;
        cards[2]--;
        cards[3]--;
        score += 7;
    }

    while (cards[0] && cards[1] && cards[2]) {
        cards[0]--;
        cards[1]--;
        cards[2]--;
        score += 5;
    }

    while (cards[0] && cards[1]) {
        cards[0]--;
        cards[1]--;
        score += 3;
    }

    while (cards[0]) {
        cards[0]--;
        score += 1;
    }

    return score;
}

