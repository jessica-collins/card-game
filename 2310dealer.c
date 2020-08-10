#include "dealer.h"
#include "common.h"

/*
 * Game struct used to clean up when SIGHUB is caught
 **/
Game* sigHandler;

int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: 2310dealer deck path p1 {p2}\n");
        exit(1);
    }    

    char* buffer1 = read_deckfile(argv[1]);
    char* deck = check_deckfile(buffer1);
    char* buffer2 = read_pathfile(argv[2]);

    Game* game = (Game*)malloc(sizeof(Game));
    Site* sites = create_sites(game, buffer2, argc);
    initialise_game(game, deck, sites, argc);

    initialise_players(game, argv, buffer2);
    sigHandler = game;   
    struct sigaction sighup;
    memset(&sighup, 0, sizeof(sighup));
    sighup.sa_handler = sighup_handler;
    sigaction(SIGHUP, &sighup, NULL);
    signal(SIGPIPE, SIG_IGN);

    char** board = initialise_board(game);
    play_game(board, game);

    return 0;
}

/*
 * Handles SIGHUP when it is caught
 * */
void sighup_handler(int signalNumber) {
    sigHandler->sighup = true;
    shut_down_players(sigHandler);
    exit(1);
}

/*
 * Shuts down all player processes when SIGHUP has been caught
 * */
void shut_down_players(Game* game) {
    int i;
   
    for (i = 0; i < game->numPlayers; i++) {
        int status;
        waitpid(game->players[i].pid, &status, WNOHANG);
        
        if (!WIFEXITED(status)) {
            kill(game->players[i].pid, SIGKILL);
        }
    }
}

/*
 * Parse the deckfile
 * Return the deckfile as a string on success or exit if there was an 
 * issue with the deckfile
 * */
char* read_deckfile(char* fileName) {
    FILE* in = fopen(fileName, "r");
    if (!in) {
        fprintf(stderr, "Error reading deck\n");
        exit(2);
    }

    fseek(in, 0, SEEK_END);
    long length = ftell(in);
    fseek(in, 0, SEEK_SET);

    char* buffer = (char*)malloc(sizeof(char) * length + 1);
    long offset = 0;    
    if (buffer == NULL) {
        fprintf(stderr, "Error reading deck\n");
        exit(2);
    }

    while (!feof(in) && offset < length) {
        offset += fread(buffer + offset, sizeof(char), length - offset, in);
    }
    buffer[offset - 1] = '\0';
    fclose(in);

    return buffer;
}

/*
 * Parse the pathfile
 * Return the contents of the pathfile as a string on success or exit if there 
 * was an issue with the pathfile
 * */
char* read_pathfile(char* fileName) {
    FILE* in = fopen(fileName, "r");

    if (!in) {
        fprintf(stderr, "Error reading path\n");
        exit(3);
    }

    fseek(in, 0, SEEK_END);
    long length = ftell(in);
    fseek(in, 0, SEEK_SET);

    char* buffer = (char*)malloc(sizeof(char) * length + 1);
    long offset = 0;
    if (buffer == NULL) {
        fprintf(stderr, "Error reading path\n");
        exit(3);
    }

    while (!feof(in) && offset < length) {
        offset += fread(buffer + offset, sizeof(char), length - offset, in);
    }
    buffer[offset] = '\0';
    fclose(in);

    return buffer;
}

/*
 * Ensure that the contents of the deckfile are valid
 * Return the deck as a string on success or exit if the cards in the deck 
 * are invalid
 * */
char* check_deckfile(char* buffer) {
    int deckSize, i;
    if (sscanf(buffer, "%d", &deckSize) != 1) {
        fprintf(stderr, "Error reading deck\n");
        exit(2);
    }
 
    char* deck = (char*)malloc(sizeof(char) * deckSize);
    strncpy(deck, buffer + 1, deckSize);
    for (i = 0; i < deckSize; i++) {
        if (deck[i] != 'A' && deck[i] != 'B' && deck[i] != 'C' && 
		deck[i] != 'D' && deck[i] != 'E') {
            fprintf(stderr, "Error reading deck\n");
            exit(2);
        }
    }

    return deck;
}

/*
 * Ensure that the contents of the pathfile are valid and create the sites 
 * in the path
 * Return an array of the sites on success or exit if the sites are invalid
 * */
Site* create_sites(Game* game, char* buffer, int argc) {
    int pathSize, i = 0, j = 0, k, count = 1;
    char dummy;
    if (sscanf(buffer, "%d%c", &pathSize, &dummy) != 2 || dummy != ';') {
        fprintf(stderr, "Error reading path\n");
        exit(3);
    }

    if (pathSize > 9) {
        count += 2;
    } else {
        count++;
    }

    Site* sites = (Site*)malloc(sizeof(Site) * pathSize);
    for (i = count; i < pathSize * SITE_SIZE + count; i += SITE_SIZE) {
        Site site;
        char* siteType = (char*)malloc(sizeof(char) * TYPE_SIZE);
        strncpy(siteType, buffer + i, TYPE_SIZE);
        site.type = siteType;
        site.type[TYPE_SIZE] = '\0';
        if (buffer[i + TYPE_SIZE] == '-' || (buffer[i + TYPE_SIZE] - '0') > 
		(argc - PROGRAM_ARGS)) {
            site.limit = argc - PROGRAM_ARGS;
        } else {
            site.limit = buffer[i + TYPE_SIZE] - '0';
        }
        site.players = (char*)malloc(sizeof(char) * site.limit);
        for (k = 0; k < site.limit; k++) {
            site.players[k] = ' ';
        }
        sites[j] = site;
        j++;
    }

    if (!valid_path(game, sites, pathSize, argc)) {
        fprintf(stderr, "Error reading path\n");
        exit(3);
    }

    game->pathSize = pathSize;
    return sites;
}

/*
 * Check to see if the path is valid
 * Returns true if the path is valid and false if it is invalid
 * */
bool valid_path(Game* game, Site* sites, int pathSize, int argc) {
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
	    || sites[0].limit != argc - PROGRAM_ARGS || 
	    sites[pathSize - 1].limit != argc - PROGRAM_ARGS) {
        return false;
    }

    return true;
}

/*
 * Initialise the structure members of the game
 * */
void initialise_game(Game* game, char* deck, Site* sites, int argc) {
    game->sites = sites;
    game->deck = deck;
    game->numPlayers = argc - PROGRAM_ARGS;
    game->players = (Player*)malloc(sizeof(Player) * game->numPlayers);
    game->sighup = false;
}

/*
 * Send a message to a player to prompt them for a move, let them know 
 * when a move has occurred or when the game is over
 * */
void send_message(DealerMessage message, FILE* stream, int id, int site, 
	int points, int money, int card) {
    switch (message) {
        case YT:
            fprintf(stream, "YT\n");
            break;
        case EARLY:
            fprintf(stream, "EARLY\n");
            break;
        case DONE:
            fprintf(stream, "DONE\n");
            break;
        case HAP:
            fprintf(stream, "HAP%d,%d,%d,%d,%d\n", id, site, points, 
		    money, card);
            break;
    }

    fflush(stream);
}

/*
 * Initialise the values for each member of the player struct
 * */
void assign_player_values(Player* player) {
    player->position = 0;
    player->money = 7;
    player->v1 = 0;
    player->v2 = 0;        
    player->points = 0;
    player->a = 0;
    player->b = 0;
    player->c = 0;
    player->d = 0;
    player->e = 0;
}

/* 
 * Create the child process and initialise the structure members of the players
 * Exit if there was an issue starting a child process
 * */
void initialise_players(Game* game, char** argv, char* path) {
    int i;
    for (i = 0; i < game->numPlayers; i++) {
        int playerIn[2];
        int playerOut[2];
        if (pipe(playerIn) < 0 || pipe(playerOut) < 0) {
            fprintf(stderr, "Error starting process\n");
            exit(4);
        }

        Player player;
        player.id = i;
        assign_player_values(&player);
        player.pid = fork();
        if (player.pid < 0) {
            fprintf(stderr, "Error starting process\n");
            exit(4);
        } else if (player.pid == 0) {
            //child
            close(playerIn[STDOUT]);
            dup2(playerIn[STDIN], STDIN);
            close(playerOut[STDIN]);
            dup2(playerOut[STDOUT], STDOUT);
            dup2(open("/dev/null", O_WRONLY), STDERR);
            char numPlayers[3], id[3];
            sprintf(numPlayers, "%d", game->numPlayers);
            sprintf(id, "%d", i);
            execlp(argv[i + PROGRAM_ARGS], argv[i + PROGRAM_ARGS], 
		    numPlayers, id, NULL);
        } else {
            //parent
            close(playerIn[STDIN]);
            close(playerOut[STDOUT]);

            player.in = fdopen(playerIn[STDOUT], "w");
            player.out = fdopen(playerOut[STDIN], "r");

            if (!player.in || !player.out) {
                fprintf(stderr, "Error starting process\n");
                exit(4);
            }

            if (fgetc(player.out) != '^') {
                fprintf(stderr, "Error starting process\n");
                exit(4);
            }
            send_path(game, player.in);
        }
        game->players[i] = player;
    }
}

/*
 * Send the path to a player
 * */
void send_path(Game* game, FILE* stream) {
    int i;
    char* buffer = (char*)malloc(sizeof(char) * game->pathSize * 
	    SITE_SIZE + 3);
    sprintf(buffer, "%d;", game->pathSize);

    for (i = 0; i < game->pathSize; i++) {
        sprintf(buffer + strlen(buffer), "%c%c%d", game->sites[i].type[0], 
		game->sites[i].type[1], game->sites[i].limit);
    }
    buffer[game->pathSize * SITE_SIZE + 3] = '\0';
    fprintf(stream, "%s\n", buffer);

    fflush(stream);
}

/*
 * Start the game
 * Find the player that is furthest behind and prompt them for a move
 * Alert all players when a move has occurred and print the board
 * Print the scores when the game is over and alert players
 * */
void play_game(char** board, Game* game) {
    int i;

    display_board(board, game);

    while (!game_over(game)) {
        // Find player who is furthest behind
        int last = game->players[0].position, pID = 0;
        for (i = 1; i < game->numPlayers; i++) {
            if (game->players[i].position < last) {
                last = game->players[i].position;
                pID = i;
            }
        }

        for (i = game->sites[last].limit - 1; i >= 0; i--) {
            if (game->sites[last].players[i] == pID + '0') {
                break;
            }
            if (game->sites[last].players[i] != ' ' && 
		    game->sites[last].players[i] - '0' != pID) {
                pID = game->sites[last].players[i] - '0';
                break;
            }
        }

        send_message(YT, game->players[pID].in, pID, 0, 0, 0, 0);
        int site = receive_message(game, pID);
        int* move = handle_move(game, site, pID);
        printf("Player %d Money=%d V1=%d V2=%d Points=%d A=%d B=%d C=%d "
		"D=%d E=%d\n", pID, game->players[pID].money, 
		game->players[pID].v1, game->players[pID].v2, 
		game->players[pID].points, game->players[pID].a, 
		game->players[pID].b, game->players[pID].c, 
		game->players[pID].d, game->players[pID].e);
        update_board(board, game);
        for (i = 0; i < game->numPlayers; i++) {
            send_message(HAP, game->players[i].in, pID, site, move[0], 
		    move[1], move[2]);
        }
    }

    print_scores(game);

    for (i = 0; i < game->numPlayers; i++) {
        send_message(DONE, game->players[i].in, 0, 0, 0, 0, 0);
    }
}

/*
 * Receive a message from a player
 * On success, return the site that the player has chosen to move to
 * */
int receive_message(Game* game, int id) {
    long long n = game->pathSize;
    int c, i = 0, site, count = 0;
    while (n != 0) {
        n /= 10;
        ++count;
    }

    char* buffer = (char*)malloc(sizeof(char) * (count + 3));
    char dummy1, dummy2;

    while (c = fgetc(game->players[id].out), c != '\n' && c != EOF) {
        buffer[i] = c;
        i++;
    }
    buffer[count + 2] = '\0';

    sscanf(buffer, "%c%c%d", &dummy1, &dummy2, &site);

/*
    if (sscanf(buffer, "%c%c%d", &dummy1, &dummy2, &site) != 3) {
        fprintf(stderr, "Communications error\n");
        exit(5);
    }
*/
    return site;
}

/*
 * Carry out a move when a player has chosen their next site
 * Update the structure members of the players depending on the site they 
 * move to
 * Return an array of the values for members to be changed for that player
 * */
int* handle_move(Game* game, int site, int id) {
    int* move = (int*)malloc(sizeof(int) * 3);
    if (!strcmp(game->sites[site].type, "Mo")) {
        move[0] = 0;
        move[1] = 3;
        move[2] = 0;
        game->players[id].money += 3;
    } else if (!strcmp(game->sites[site].type, "V1")) {
        move[0] = 0;
        move[1] = 0;
        move[2] = 0;
        game->players[id].v1++;
    } else if (!strcmp(game->sites[site].type, "V2")) {
        move[0] = 0;
        move[1] = 0;
        move[2] = 0;
        game->players[id].v2++;
    } else if (!strcmp(game->sites[site].type, "Do")) {
        move[0] = game->players[id].money / 2;
        move[1] = -game->players[id].money;
        move[2] = 0;
        game->players[id].points += game->players[id].money / 2;
        game->players[id].money = 0;
    } else if (!strcmp(game->sites[site].type, "Ri")) {
        move[0] = 0;
        move[1] = 0;
        if (game->deck[0] == 'A') {
            move[2] = 1;
            game->players[id].a++;
        } else if (game->deck[0] == 'B') {
            move[2] = 2;
            game->players[id].b++;
        } else if (game->deck[0] == 'C') {
            move[2] = 3;
            game->players[id].c++;
        } else if (game->deck[0] == 'D') {
            move[2] = 4;
            game->players[id].d++;
        } else {
            move[2] = 5;
            game->players[id].e++;
        }
        shift_deck(game);
    } else {
        move[0] = 0;
        move[1] = 0;
        move[2] = 0;
    }
    shift_site_players(game, id, site);
    return move;
}

/*
 * Remove the player with the specified ID from one site and add them
 * to the site that they would like to move to
 * */
void shift_site_players(Game* game, int id, int nextSite) {
    int i, j, currentSite = game->players[id].position;

    for (i = 0; i < game->sites[currentSite].limit; i++) {
        if (game->sites[currentSite].players[i] == id + '0') {
            for (j = i; j < game->sites[currentSite].limit; j++) {
                game->sites[currentSite].players[j] =
                        game->sites[currentSite].players[j + 1];
            }
            game->sites[currentSite].players[game->sites[currentSite].limit
                    - 1] = ' ';
        }
    }

    for (i = 0; i < game->sites[nextSite].limit; i++) {
        if (game->sites[nextSite].players[i] == ' ') {
            game->sites[nextSite].players[i] = id + '0';
            break;
        }
    }

    game->players[id].position = nextSite;
}

/*
 * Shift the deck once a card has been drawn
 * */
void shift_deck(Game* game) {
    int i, deckSize = (sizeof(game->deck) / sizeof(char)) - 1;
    char first = game->deck[0];

    for (i = 0; i < deckSize; i++) {
        game->deck[i] = game->deck[i + 1];
    }
    game->deck[deckSize - 1] = first;
}

/*
 * Initialise the board and positions for the game
 * */
char** initialise_board(Game* game) {
    int r, c;
    char** board = (char**)malloc(sizeof(char*) * game->numPlayers);

    for (r = 0; r < game->numPlayers; r++) {
        board[r] = (char*)malloc(sizeof(char) * (game->pathSize * 
	        SITE_SIZE + 1));
        for (c = 0; c < game->pathSize * SITE_SIZE + 1; c++) {
            if (c == game->pathSize * SITE_SIZE) {
                board[r][c] = '\n';
            } else {
                board[r][c] = ' ';
            }
        }
    }

    char player = (game->numPlayers - 1) + '0';
    for (r = 0; r < game->numPlayers; r++) {
        game->sites[0].players[r] = player;
        board[r][0] = player;
        player--;
    }

    return board;
}

/*
 * Once a move has been made, update the positions of the players 
 * on the board
 * */
void update_board(char** board, Game* game) {
    int i = 0, r, c, j;

    for (r = 0; r < game->numPlayers; r++) {
        for (c = 0; c < game->pathSize * SITE_SIZE + 1; c++) {
            if (c == game->pathSize * SITE_SIZE) {
                board[r][c] = '\n';
            } else {
                board[r][c] = ' ';
            }
        }
    }

    for (i = 0; i < game->pathSize; i++) {
        if (game->sites[i].players[0] != ' ') {
            for (j = 0; j < game->sites[i].limit; j++) {
                board[j][i * SITE_SIZE] = game->sites[i].players[j];
            }
        }
    }

    display_board(board, game);
}

/*
 * Print the board and the path
 * */
void display_board(char** board, Game* game) {
    int i, r, c, count = 0;

    for (r = 0; r < game->numPlayers; r++) {
        for (c = 0; c < game->pathSize * SITE_SIZE; c++) {
            if (board[r][c] != ' ' && board[r][c] != '\n') {
                count++;
                break;
            }
        }
    }

    for (i = 0; i < game->pathSize; i++) {
        if (i == game->pathSize - 1) {
            printf("%s \n", game->sites[i].type);
        } else {
            printf("%s ", game->sites[i].type);
        }
    }

    for (r = 0; r < count; r++) {
        for (c = 0; c < game->pathSize * SITE_SIZE + 1; c++) {
            printf("%c", board[r][c]);
        }
    }
}

/*
 * Check to see if the game is over
 * Return true if all players are at the last site or false if the game 
 * is still running
 * */   
bool game_over(Game* game) {
    int i;

    for (i = 0; i < game->numPlayers; i++) {
        if (game->players[i].position != game->pathSize - 1) {
            return false;
        }
    }

    return true;
}

/*
 * Calculate and print the scores of for each player at the end of the game
 * */
void print_scores(Game* game) {
    int i;

    printf("Scores: ");
    for (i = 0; i < game->numPlayers; i++) {
        int cardScore = card_score(game, i);
        game->players[i].points += (game->players[i].v1 + game->players[i].v2 
		+ cardScore);
        if (i == game->numPlayers - 1) {
            printf("%d\n", game->players[i].points);
        } else {
            printf("%d,", game->players[i].points);
        }
    }
}

/*
 * Calculate the points that a player gains from the cards that they have
 * Return the points that they gain
 * */
int card_score(Game* game, int id) {
    int i, j, score = 0, a = game->players[id].a, b = game->players[id].b, 
	    c = game->players[id].c, d = game->players[id].d, 
	    e = game->players[id].e;
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
