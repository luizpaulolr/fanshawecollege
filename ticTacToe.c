/* This program emulates a basic Tic Tac Toe game for two players.
The program ends with a winner or in a DRAW GAME.
The players must input in each turn the row and column location where they want to put the symbols X or O.
The first column and row is 0 and it goest up to 2 (0, 1 or 2 for each input).*/

#include <stdio.h>

#define TRUE 1
#define FALSE 0
#define NUMPLAYERS 2
#define NUMROWS 3
#define NUMCOLUMNS 3
#define BOARDROWS 5
#define BOARDLINES 5

char board[3][3];

void printBoard(void)
{
	// Put the cursos on initial position "Cleaning all the previously printed characters. 
	printf("\033[2J");
	printf("BOARD STATUS\n");
	// Brute force matrix printing
	/*printf("%c # %c # %c\n# # # # #\n%c # %c # %c\n# # # # #\n%c # %c # %c\n",	board[0][0], board[0][1], board[0][2],
											board[1][0], board[1][1], board[1][2],
											board[2][0], board[2][1], board[2][2]);*/
	// The matrix is 3x3 and the Board is 5x5. So two counters is needed to keep track of the matrix
	int lineCount = 0; 
	int rowCount = 0;
	for (int i = 0; i < BOARDROWS; i++)
	{
		
		if (i % 2 == 0)
		{
			for (int j = 0; j < BOARDLINES; j++)
			{

				if (j % 2 == 0)
				{

					printf("%c", board[lineCount][rowCount]);
					rowCount++; // It increments when a line characters is printed
				}
				else
				{
					printf("|");
				}
			}
			rowCount = 0;
			lineCount++; // It increments only when a line with characters is printed
			printf("\n");
		}
		else
		{
			printf("-----\n");
		}
	}
	//lineCount = 0; // No need for it since the function is exiting and all local variables will be destroyed.
} // eo function printBoard ::

int main(void)
{
	int row = 0, column = 0;
	char player[NUMPLAYERS] = { 2, 1 };
	char turn = 1;

	// Initialize the board with BLANK SPACES 
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

		while(TRUE) // Prompt the user for a valid answer
		{
			printf("Player %i enter the row location: ", player[turn % NUMPLAYERS]);
			scanf_s("%i", &row);

			printf("Player %i enter the column location: ", player[turn % NUMPLAYERS]);
			scanf_s("%i", &column);

			if (row > 2 || row < 0 || column > 2 || column < 0 || board[row][column] != ' ')
			{
				printf("Invalid input! Try again!\n");
			}
			else
			{
				break; // exit the loop if a valid input was inserted
			}
		}

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
			/* There is 8 winning conditions that must be checked. It must check if a line is equal,
			and it cannot include a line of ' ' (blankspaces)*/
			if ((board[0][0] != ' ' && board[0][0] == board[0][1] && board[0][1] == board[0][2]) ||	//Condition 1
				(board[0][0] != ' ' && board[0][0] == board[1][1] && board[1][1] == board[2][2]) ||	//Condition 2
				(board[0][0] != ' ' && board[0][0] == board[1][0] && board[1][0] == board[2][0]) ||	//Condition 3
				(board[0][1] != ' ' && board[0][1] == board[1][1] && board[1][1] == board[2][1]) ||	//Condition 4
				(board[0][2] != ' ' && board[0][2] == board[1][2] && board[1][2] == board[2][2]) ||	//Condition 5
				(board[1][0] != ' ' && board[1][0] == board[1][1] && board[1][1] == board[1][2]) ||	//Condition 6
				(board[2][0] != ' ' && board[2][0] == board[2][1] && board[2][1] == board[2][2]) ||	//Condition 7
				(board[2][0] != ' ' && board[2][0] == board[1][1] && board[1][1] == board[0][2]) )	//Condition 8
			{
				printBoard();
				printf("\nPLAYER %i WON!\n", player[turn % 2]);
				break;
			} // eo if checking winning conditions
		} // eo if (turn >= 5)
		//Finishes the game if turn 9 arrives and wining criteria was not met
		if (turn == 9)
		{
			printBoard();
			printf("\nDRAW GAME!\n");
			break;
		} // eo if (turn == 9)
		//Increment the turn for the next cycle
		turn ++;
	} //eo while 
} // eo main
