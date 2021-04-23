//This program was created by Reid Edward Erb, UCID 10089638
//for the course CPSC 359 at the University of Calgary in the 2021 winter semester
//CPSC 359 is instructed by Dr. Jalal Kawash. The lead TA for this assignment is Desmond Larsen-Rosner

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

struct shared
{
	char board[9];
	int turn;
	int placed; 	//Keeps track of how many tiles are full. When it equals 9, the board is full.
	int last;	//Keeps track of whether player 1 or player 2 last moved
	int wintrue;	//If somebody has won the game, wintrue will be set to 1
	int game_on;
};

struct shared game; //Creates a global structure that can be accessed by all threads

void initboard(struct shared * gamep)
{
	int i;
	for (i = 0; i < 9; i++)
	{
		gamep->board[i] = '#'; //We use the hashmark to represent an empty space on the board
	}
}

void displayboard(struct shared * gamep)
{
	int i, j, k;
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			k = (i * 3) + j; 
			//If (i, j) is the coordinate of an entry in the grid, then k will be its corresponding index
			printf("%c ", gamep->board[k]);
		}
		printf("\n");
	} 
}

int decideMove(struct shared *gamep)
{
	//This function will return the index where a character should be placed in game.board
	char ch = '@';
	int tile = -1;
	while (ch != '#')
	{
		tile = rand() % 9;	//By using the modulus operator, it's guaranteed that tile is in the range [0, 8]
		ch = gamep->board[tile]; //The loop will reiterate until it finds and empty tile
	}

	return tile;
}

int checkwin(struct shared *gamep, char check)
{
	int winstat = 0;	//winstat will be returned by the function. If it equals 0, there is no winner yet.

	//There are 8 possible ways to win a game of tic-tac-toe. First we'll check for horizontal matches.

	if (gamep->board[0] == check && gamep->board[1] == check && gamep->board[2] == check) {winstat = 1;}
	else if (gamep->board[3] == check && gamep->board[4] == check && gamep->board[5] == check) {winstat = 1;}
	else if (gamep->board[6] == check && gamep->board[7] == check && gamep->board[8] == check) {winstat = 1;}

	//Next we check for vertical matches
	
	else if (gamep->board[0] == check && gamep->board[3] == check && gamep->board[6] == check) {winstat = 1;}
	else if (gamep->board[1] == check && gamep->board[4] == check && gamep->board[7] == check) {winstat = 1;}
	else if (gamep->board[2] == check && gamep->board[5] == check && gamep->board[8] == check) {winstat = 1;}

	//Finally, we check for diagonal matches

	else if (gamep->board[0] == check && gamep->board[4] == check && gamep->board[8] == check) {winstat = 1;}
	else if (gamep->board[2] == check && gamep->board[4] == check && gamep->board[6] == check) {winstat = 1;}

	return winstat;
}

void * playGame(void * threadid)
{
	long tid = (long) threadid;
	
	struct shared * newp;
	newp = &game;
	int temp, tile;

	while(1)
	{
		while(newp->turn != tid);	//Wait until it's the thread's turn
		tile = decideMove(newp);
		if (tid == 1) {newp->board[tile] = 'X';}	//The thread id determines whether an X or O is placed
		else {newp->board[tile] = 'O';}

		newp->last = tid;
		temp = newp->placed;
		temp = temp + 1;
		newp->placed = temp;	//the value of placed is incremented each time a letter is placed in the grid

		if (tid == 1) {newp->wintrue = checkwin(newp, 'X');}
		else {newp->wintrue = checkwin(newp, 'O');}	//We check if the player thread has won.

		newp->turn = 0;	//Signal to the master thread that it can perform its instructions
	}	
}

void * masterThread(void * threadid)
{
	struct shared * gamep;
	gamep = &game;

	gamep->turn = 0;	//Initializes the "turn" value in the global structure to 0
	gamep->placed = 0;
	gamep->last = 2;
	gamep->wintrue = 0;
	
	initboard(gamep);	//Fills board with '#' characters, which represent empty spaces

	pthread_t xid = 1;
	pthread_t oid = 2;

	pthread_create(&xid, NULL, playGame, (void *) xid);
	pthread_create(&oid, NULL, playGame, (void *) oid);
	
	while(1)
	{
		while(gamep->turn != 0); //Wait until the turn in the game structure is set to 0
		displayboard(gamep);
		printf("\n");
		if(gamep->placed >= 9 || gamep->wintrue == 1)	//Check if the board is full or if there is a winner
		{
			if (gamep->wintrue == 1)
			{
				if (gamep->last == 1) {printf("X is the winner!\n");}
				else {printf("O is the winner!\n");}
			}
			else {printf("DRAW\n");}
			/*Unlike in real tic-tac-toe, it seems like draw games are relatively rare when the X's and O's
			are randomly placed.*/

			gamep->game_on = 0;	//Switch the game_on flag off to signal to the main function to cancel master
			pthread_cancel(xid);
			pthread_cancel(oid);
			break;	//End the loop in the master thread
		}
		else
		{
			if (gamep->last == 1) {gamep->turn = 2;}
			else {gamep->turn = 1;}	//The next player turn depends on who moved last
		}
	}
}

int main(int argc, char **argv)
{
	srand(time(0));

	struct shared * gamep;
	gamep = &game;
	
	pthread_t master = 777;	//We set the master id to 777 for luck :-)

	gamep->game_on = 1;	//Switch the game_on flag to true.
	pthread_create(&master, NULL, masterThread, (void *) master); //The master thread will create the other threads
	
	while (gamep->game_on == 1); //Wait until the game_on flag is switched off.
	pthread_cancel(master);

	return 0;
}
