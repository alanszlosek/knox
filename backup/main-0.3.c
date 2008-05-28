/*
version 0.3
*/

#include <signal.h>
#include <sys/select.h>
#include <iterm/core.h>
#include <iterm/unix/ttyio.h>
#include <sys/types.h>


#include <ncurses.h>

#ifdef ABCD
#include <mcheck.h>
#endif

#include "vt.h"
#include "../linkedList/list.h"
#include "../rectangleArrangement/rectangleCoordinates.h"

sig_atomic_t previousSignal = 0;

void signalHandler(int signalNumber) {
	previousSignal = signalNumber;
}

// pass number of terminals on commandline
int main(int argc, char *argv[]) {
	// Virtual Terminal related
	struct virtualTerminal *vt;

	// nCurses related variables
	WINDOW *ncursesScreen;
	int screenWidth, screenHeight, vtColumns, vtRows; // to hold screen's height and width

	// Miscellaneous variables
	unsigned int input;
	char ch;
	char inputString[33];
	int i, j, k, done, fdMax, fdCurrent, inputSize;
	int *vtCoordinates, *vtPointer;
	fd_set fileDescriptors;
	struct timespec timeout;
	struct sigaction sa;
	linkedList *llTerminals;
	linkedListNode *llNode;

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
	}

	keypad(ncursesScreen, true); // cause nCurses to package key combos into special, single values
	noecho(); // don't echo input
	refresh(); // clear the main window

	// Get main window dimensions. This will be used when creating additional virtual terminals
	getmaxyx(ncursesScreen, screenHeight, screenWidth);
	//vtRows = (vtRows / 2); // - (vtRows % 4);
	//vtCols = (vtCols / 2); // - (vtCols % 4);

	//fprintf(stderr, "Rows: %d Cols: %d\n", vtRows, vtCols);

	// set j equal to number of terminals
	if(argc > 1)
		j = atoi(argv[1]);
	else
		j = 1;


	//vtPointer = vtCoordinates = rectangleCoordinates_favorWidth(screenWidth, screenHeight, j, 1);
	vtPointer = vtCoordinates = rectangleCoordinates_favorHeight(screenWidth, screenHeight, j, 0);
	for(i = 0; i < j; i++) {
		vt = vtCreate( *(vtPointer + 2), *(vtPointer + 3), *vtPointer, *(vtPointer + 1), i);
		listAppendCargo(llTerminals, vt);
		vtPointer += 4;
	}

	llNode = listFirst(llTerminals);
	vt = (struct virtualTerminal*) nodeCargo(llNode);
	fdCurrent = vt->fd;
	//fprintf(stderr, "fd %d\n",fdCurrent);
	k = 0;

	while(!done) {
		FD_ZERO(&fileDescriptors);
		FD_SET(0, &fileDescriptors);
		llNode = listFirst(llTerminals);
		fdMax = 0;
		while(llNode != NULL) {
			vt = (struct virtualTerminal*) nodeCargo(llNode);
			if(vt->fd > 0 && vt->state != VT_PAUSED) { // fd will be -1 if shell program has exited
				j = vt->fd;
				if(j > fdMax)
					fdMax = j;
				FD_SET(j, &fileDescriptors);
			}
			llNode = listNext(llNode);
		}

		pselect(fdMax + 1, &fileDescriptors, NULL, NULL, &timeout, NULL); 

		if(FD_ISSET(0, &fileDescriptors)) {
			inputSize = read(0, &ch, 1);
			//for(i = 0; i < inputSize; i++) {
				switch(ch) { //inputString[i]) {
			
					case '`': // tab
						wcolor_set(((struct virtualTerminal*)listNodeIndex(llTerminals ,k))->window, 1, NULL);
						k++;
						if(k == llTerminals->length)
							k = 0;
						fdCurrent = TtyTerminalIO_get_associated_fd( ((struct virtualTerminal*)listNodeIndex(llTerminals ,k))->terminalIO );
						break;

					case '~':
						done = 1;
						break;

					default:
						//ch = inputString[i];

						write(fdCurrent, &ch, 1);
						break;
				}
			//}
		}

		j = 0; // temporarily keep track of whether any windows were updated
		llNode = listFirst(llTerminals);
		while(llNode != NULL) {
			vt = (struct virtualTerminal*) nodeCargo(llNode);
			if(vt->fd > 0 && FD_ISSET(vt->fd, &fileDescriptors) && vt->state != VT_PAUSED) { // fd will be -1 if shell has closed
				VTCore_dispatch(vt->core);
			
				wnoutrefresh(vt->wBorder); // updates backbuffered nCurses window
				wnoutrefresh(vt->window); // updates backbuffered nCurses window
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

	llNode = listFirst(llTerminals);
	while(llNode != NULL) {
		vt = (struct virtualTerminal*) nodeCargo(llNode);
		//fprintf(stderr,"Terminal %d\n", vt->id);
		llNode = listNext(llNode);
	}

	free(vtCoordinates);
	listDelete(llTerminals, vtDestroy);
	endwin();

	return 0;
}
