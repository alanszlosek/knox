/*
version 0.4 - with tesInterpreter
*/

#include <signal.h>
#include <sys/select.h>
#include <iterm/core.h>
#include <iterm/unix/ttyio.h>
#include <sys/types.h>
#include <string.h>
#include <ncurses.h>

#define DEBUG 1
#include "vt.h"
#include "../../linkedList/list.h"
#include "../../divideRectangle/divideRectangle.h"

#ifdef ABCD
#include <mcheck.h>
#endif

sig_atomic_t previousSignal = 0;

void signalHandler(int signalNumber) {
	previousSignal = signalNumber;
}

// pass number of terminals on commandline
int main(int argc, char *argv[]) {
	struct virtualTerminal *vtPointer;

	// nCurses related variables
	WINDOW *ncursesScreen;
	int screenWidth, screenHeight, vtColumns, vtRows; // to hold screen's height and width

	// Miscellaneous variables
	unsigned int input;
	char ch;
	int in;
	char inputString[33];
	int i, j, k, done, fdMax, fdCurrent, inputSize;
	int *vtCoordinates, *vtCoordPointer;
	fd_set fileDescriptors;
	struct timespec timeout;
	struct sigaction sa;
	linkedList *llTerminals;
	linkedListNode *llNode, *llCurrent;

	//mtrace();
	// Set up signal handling
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &signalHandler;
	sigaction(SIGTERM, &sa, NULL);

	// Initializations
	llTerminals = listNew();
	done = fdMax = 0;
	timeout.tv_sec = 0;
	timeout.tv_nsec = 500000000; // 10E-9

	// Initialize terminal
	ncursesScreen = initscr();
	if(ncursesScreen == NULL) {
		perror("Error initializing nCurses\n");
		exit(1);
	}

	if(has_colors()) {
		start_color();
		init_pair(1, COLOR_WHITE, COLOR_BLACK);
		init_pair(2, COLOR_BLACK, COLOR_WHITE);
		init_pair(3, COLOR_RED, COLOR_BLACK);
		init_pair(4, COLOR_BLUE, COLOR_BLACK);
		init_pair(5, COLOR_WHITE, COLOR_BLACK);
	}

	//keypad(ncursesScreen, true); // cause nCurses to package key combos into special, single values
	//raw();
	noecho(); // don't echo input
	refresh(); // clear the main window

	// Get main window dimensions. This will be used when creating additional virtual terminals
	getmaxyx(ncursesScreen, screenHeight, screenWidth);
#ifdef DEBUG
	fprintf(stderr, "Height %d Width %d\n", screenHeight, screenWidth);
#endif
	//vtRows = (vtRows / 2); // - (vtRows % 4);
	//vtCols = (vtCols / 2); // - (vtCols % 4);

	//fprintf(stderr, "Rows: %d Cols: %d\n", vtRows, vtCols);

	// set j equal to number of terminals
	if(argc > 1)
		j = atoi(argv[1]);
	else
		j = 1;

	vtPointer = vtCreate(screenWidth, 2, 0, screenHeight - 2, 0); // pass width, height, x, y, number
	listAppendCargo(llTerminals, vtPointer);

	screenHeight -= 2;

	vtCoordPointer = vtCoordinates = divideRectangle_favorHeight(screenWidth, screenHeight, j, 0);
	for(i = 0; i < j; i++) {
		vtPointer = vtCreate(*(vtCoordPointer + 2), *(vtCoordPointer + 3), *vtCoordPointer, *(vtCoordPointer + 1), i + 1); // pass width, height, x, y, number
		if(vtPointer == NULL)
			break;
		listAppendCargo(llTerminals, vtPointer);
		vtCoordPointer += 4;
	}

	llCurrent = llNode = listFirst(llTerminals);
	vtPointer = (struct virtualTerminal*) nodeCargo(llNode);
	fdCurrent = vtPointer->fd;
	fprintf(stderr, "fd %d\n",fdCurrent);
	k = 0;

	while(!done) {
		FD_ZERO(&fileDescriptors);
		FD_SET(0, &fileDescriptors);
		fdMax = 0;
		llNode = listFirst(llTerminals);
		while(llNode != NULL) {
			vtPointer = (struct virtualTerminal*) nodeCargo(llNode);
			if(vtPointer->fd > 0 && vtPointer->state != VT_PAUSED) { // fd will be -1 if shell program has exited
				j = vtPointer->fd;
				if(j > fdMax)
					fdMax = j;
				FD_SET(j, &fileDescriptors);
			}
			llNode = listNext(llNode);
		}

		pselect(fdMax + 1, &fileDescriptors, NULL, NULL, &timeout, NULL);

		// process keyboard input, if any
		if(FD_ISSET(0, &fileDescriptors)) {
			//inputSize = read(0, &ch, 1);
			in = getch();
			switch(in) { //inputString[i]) {
				case '`': // tab
					vtPointer = (struct virtualTerminal*) nodeCargo(llCurrent);
					if(vtPointer->wBorder) {
						wattron(vtPointer->wBorder, COLOR_PAIR(1));
						box(vtPointer->wBorder, ACS_VLINE, ACS_HLINE);
						mvwprintw(vtPointer->wBorder, 0, 0, "%i ", vtPointer->id);
						wnoutrefresh(vtPointer->wBorder);
					}

					llCurrent = listNext(llCurrent);
					if(llCurrent == NULL)
						llCurrent = listFirst(llTerminals);
					vtPointer = (struct virtualTerminal*) nodeCargo(llCurrent);
					if(vtPointer->wBorder) {
						wattron(vtPointer->wBorder, COLOR_PAIR(3));
						box(vtPointer->wBorder, ACS_VLINE, ACS_HLINE);
						mvwprintw(vtPointer->wBorder, 0, 0, "%i ", vtPointer->id);
						wattroff(vtPointer->wBorder, COLOR_PAIR(3));
						wnoutrefresh(vtPointer->wBorder);
					}
					doupdate();

					//vtPointer = (struct virtualTerminal*) to->pointer;
					//wcolor_set(vtPointer->window, 1, NULL);
					k++;
					if(k == llTerminals->length)
						k = 0;
					fdCurrent = vtPointer->fd;
					break;

				case '~':
					done = 1;
					break;
				/*
				case KEY_ENTER:
					ch = '\n';
					write(fdCurrent, &ch, 1);
					break;
				*/

				default:
					ch = (char)in;

					write(fdCurrent, &ch, 1);
fprintf(stderr, "wrote: %c\n", ch);
					break;
			}
		}

		j = 0; // temporarily keep track of whether any windows were updated
		llNode = listFirst(llTerminals);
		while(llNode != NULL) {
			vtPointer = (struct virtualTerminal*) nodeCargo(llNode);
			if(vtPointer->fd > 0 && FD_ISSET(vtPointer->fd, &fileDescriptors) && vtPointer->state != VT_PAUSED) { // fd will be -1 if shell has closed
				//tesi_handleInput(to); // handle escape sequences
				VTCore_dispatch(vtPointer->core);

				//if(vtPointer->wBorder)
					//wnoutrefresh(vtPointer->wBorder); // updates backbuffered nCurses window
				wnoutrefresh(vtPointer->window); // updates backbuffered nCurses window
				j = 1; // specifies whether we should flush nCurses buffered windows
			}
			llNode = listNext(llNode);
		}
		if(j)
			doupdate(); // updates were made, wnoutrefresh() was called, now flush updates

		switch(previousSignal) {
			case SIGTERM:
				fprintf(stderr, "Caught SIGTERM\n");
				done = 1;
				break;
			default:
				break;
		}
	}

	//strcpy(inputString, "\nexit\n");
	/*
	llNode = listFirst(llTerminals);
	while(llNode != NULL) {
		vtPointer = (struct virtualTerminal*) nodeCargo(llNode);

		kill(vtPointer->pid, 9);
		//write(to->pipeToChild[1], &inputString, strlen(inputString)); // send exit to bash
		waitpid(vtPointer->pid, NULL, 0); // bash should have quit by the time it gets here

		vtDestroy(vtPointer);

		llNode = listNext(llNode);
	}
	* */

	free(vtCoordinates);
	listDelete(llTerminals, vtDestroy);

	endwin();

	return 0;
}
