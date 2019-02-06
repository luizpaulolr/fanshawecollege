/* This program emulates a basic Tic Tac Toe game for two players. 
The program ends with a winner or in a DRAW GAME.
The players must input in each turn the row and column location where they want to put the symbols X or O.
The first column and row is 0 and it goest up to 2 (0, 1 or 2 for each input).*/

#include <stdio.h>

char board[3][3];

void printBoard(void)
{
	printf("BOARD STATUS\n");
	printf("%c # %c # %c\n# # # # #\n%c # %c # %c\n# # # # #\n%c # %c # %c\n", 	board[0][0], board[0][1], board[0][2],
											board[1][0], board[1][1], board[1][2],
											board[2][0], board[2][1], board[2][2]);
} // eo function printBoard ::

#define TRUE 1
#define FALSE 0
#define NUMPLAYERS 2
#define NUMROWS 3
#define NUMCOLUMNS 3

int main(void)
{
	int row = 0, column = 0; // Invalid values to row and column
	char player[NUMPLAYERS] = { 2, 1 };
	char turn = 1;

	// Initialize the board with empty values
	for (int index = 0; index < NUMROWS; index ++)
	{
		for (int index1 = 0; index1 < NUMCOLUMNS; index1 ++)
		{
			board[index][index1] = ' ';
		}
	}
	while (TRUE)
	{
		// Print the board
		printBoard();
		// Print which turn it is
		printf("\nTurn %i inputs.\n", turn);
		do
		{
			printf("Player %i enter the row location: ", player[turn % NUMPLAYERS]);
			scanf_s("%i", &row);

			printf("Player %i enter the column location: ", player[turn % NUMPLAYERS]);
			scanf_s("%i", &column);

		} while (row > 2 || row < 0 || column > 2 || column < 0 || board[row][column] != ' '); // Validation criteria for the inputs, if invalid prompt again.

		//Place the correct Symbol according which the turn 
		if ((turn + 1) % 2 == 0)
		{
			board[row][column] = 'X';
		}
		else
		{
			board[row][column] = 'O';
		} //eo if placing the correct symbol

		//Checking for winning conditions up to turn 5
		if (turn >= 5)
		{
			if 	(	board[0][0] == board[0][1] && board[0][1] == board[0][2] ||
					board[0][0] == board[1][1] && board[1][1] == board[2][2] || 
					board[0][0] == board[1][0] && board[1][0] == board[2][0] || 
					board[0][1] == board[1][1] && board[1][1] == board[2][1] || 
					board[0][2] == board[1][2] && board[1][2] == board[2][2] || 
					board[1][0] == board[1][1] && board[1][1] == board[1][2] || 
					board[2][0] == board[2][1] && board[2][1] == board[2][2] ||
					board[2][0] == board[1][1] && board[1][1] == board[0][2] ) //eo winning conditions
			{
				printf("\nPLAYER %i WON!\n", player[turn % 2]);
				printBoard();
				break;
			} // eo if

		} // eo if (turn >= 5)
		//Finishes the game if turn 9 arrives and wining criteria was not met
		if (turn == 9)
		{
			printf("\nDRAW GAME!\n", player[turn % 2]);
			printBoard();
			break;
		} // eo if (turn == 9)
		//Increment the turn for the next cycle
		turn ++;
	} //eo while 
} // eo main
