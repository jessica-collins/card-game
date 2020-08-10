#ifndef DEALER_H
#define DEALER_H

#include "common.h"

void sighup_handler(int signalNumber);
void shut_down_players(Game* game);
char* read_deckfile(char* fileName);
char* read_pathfile(char* fileName);
char* check_deckfile(char* buffer);
Site* create_sites(Game* game, char* buffer, int argc);
void create_pipes(Game* game);
bool valid_path(Game* game, Site* sites, int pathSize, int argc);
void initialise_players(Game* game, char** argv, char* path);
void initialise_game(Game* game, char* deck, Site* sites, int argc);
void play_game(char** board, Game* game);
void send_message(DealerMessage message, FILE* stream, int id, 
	int site, int points, int money, int card);
void send_path(Game* game, FILE* stream);
int receive_message(Game* game, int id);
int* handle_move(Game* game, int site, int id);
char** initialise_board(Game* game);
void update_board(char** board, Game* game);
void display_board(char** board, Game* game);
void shift_deck(Game* game);
bool game_over(Game* game);
void print_scores(Game* game);
int card_score(Game* game, int id);
void shift_site_players(Game* game, int id, int nextSite);

#endif
