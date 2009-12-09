#include "config.h"
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#ifdef USE_SLANG
#include <slang.h>
#endif
#ifdef USE_NCURSES
#include <ncurses.h>
#endif
#include "tesi_ncurses/vt.h"
//#include "iterm_ncurses/vt.h"

#define DEBUG 1

#ifdef USE_NCURSES
WINDOW *ncursesScreen;
#endif

int keepRunning;

int processInput(char *command, int terminalIndex, int screenHeight, int screenWidth) {
	int *vtCoordinates, *vtCoordPointer;
	int numberTerminals;
	int i, j, k;
	int targets[10], targetCounter;
	struct virtualTerminal *vt;
	char *pRest, *pRest2, *pToken, *pToken2, *pToken3;
	char *delimSpace, *delimComma, *delimHyphen;
	
	delimSpace = delimComma = delimHyphen = NULL;


	if(isdigit(*command)) { // command starts with digit 
		// build up array of target terminals
		//for(i = 0; i < 10; i++)
		//	targets[i] = -1;
		targetCounter = 0;

		i = -1;
		// set NULL at the first space or colon
		pToken = strtok_r(command, " ", &delimSpace); // pToken points to first string up to the space
		pToken2 = strtok_r(pToken, ",", &delimComma); // pToken2 points to first string up to the comma
		do {
			i = j = 0;			
			pToken3 = strtok_r(pToken2, "-", &delimHyphen);
			do {
				// if start is null, set start
				// if start isn't null, set end
				if(i == 0) {
					i = strtol(pToken3, NULL, 10); // get target terminal
				} else {
					j = strtol(pToken3, NULL, 10); // get target terminal
				}
			} while((pToken3 = strtok_r(NULL, "-", &delimHyphen)));
			
			if(i > 0) {
				if(j > 0) {
					for(; i <= j; i++) {
						targets[targetCounter++] = i -1;
					}
				} else {
					targets[targetCounter++] = i -1;
				}
			}
			
		} while((pToken2 = strtok_r(NULL, ",", &delimComma)));

		pToken = strtok_r(NULL, " ", &delimSpace);
		if(pToken == NULL) { // switch to term
			i = targets[0];
			if(vtGet(i)) {
				vtHighlight(i);
				return i;
			}
		}
		
		// restore space
		if(strlen(command) > strlen(pToken)) {
			*(pToken + strlen(pToken)) = ' ';
		}
		
		strcat(pToken, "\n");
		
		
		/*
		// move the following three to functions
		if(*pRest == ' ') { // terminal # followed by space then commands to send to it ... send with newline
			strcat(pRest, "\n"); // send newline? ... this needs to be optional
		}
		if(*pRest == ':') { // terminal # followed by colon then commands to send to it ... send without newline
		}
		pRest++;
		*/
		
		for(j = 0; j < targetCounter; j++) {
			i = targets[j];
			//fprintf(stderr, "#%d\n", i);
			//continue;
			if(i < 0 || i > 10) // make sure we have a valid number
				continue;
			// rest of command should be a terminal-specific command
			vtSend(i, pToken); // move past space

			/*
			if(*pRest == '.') {
				pRest++;
				if(strcmp(pRest, "close") == 0 || strcmp(pRest, "destroy") == 0) { // destroy terminal
				}
				if(strcmp(pRest, "hide") == 0) {
				}
			}
			*/

			/*
			if(*pRest == '-') { // range of terminal numbers
				pRest++;
				j = strtol(pRest, &pRest, 10); // get last target terminal
				if(*pRest == ' ') { // terminal # followed by space then commands to send to it ... send with newline
					pRest++; // move past space
					strcat(pRest, "\n"); // send newline? ... this needs to be optional
					for(i; i <= j; i++) {
						vtSend(i-1, pRest);
					}
				}
				if(*pRest == ':') { // terminal # followed by colon then commands to send to it ... send without newline
					pRest++; // move past colon 
					vtSend(i, pRest);
					for(; i <= j; i++) {
						vtSend(i-1, pRest);
					}
				}
			}
			if(*pRest == ',') { // list of terminal numbers
			}
			*/
		}

	} else { // command doesn't start with digit

		pToken = strtok(command, " ");
		if(pToken) {
			if(strcmp(pToken, "quit") == 0) {
				keepRunning = 0;
			}
			if(strcmp(pToken, "create") == 0) {
				pToken = strtok(NULL, " ");
				if(pToken && isdigit(*pToken)) { // probably have a number
					numberTerminals = strtol(pToken, NULL, 10); // get target terminal
				} else // no number, just create 1
					numberTerminals = 1;
				j = 0;
				for(i = 0; i < 10; i++) {
					if(vtGet(i) != NULL)
						j++;
				}

#ifdef DEBUG
				fprintf(stderr, "%d existing terminals, creating %d\n", j, numberTerminals);
#endif

				// we're about to resize terminals, clear the background
				wclear(ncursesScreen);
				wnoutrefresh(ncursesScreen);
				// move terms around if not all positions are filled but last one is
				// array of terminals needs to reflect the order they are on the screen
				vtCoordPointer = vtCoordinates = divideRectangle_favorHeight(screenWidth, screenHeight - 2, numberTerminals, 1);
				for(i = 0; i < 10; i++) {
					vt = vtGet(i);
					if(vt) {
						vtMove(i, *vtCoordPointer, *(vtCoordPointer + 1), *(vtCoordPointer + 2), *(vtCoordPointer + 3)); 
					} else // found a spot to put our new terminal(s)
						break;

					vtCoordPointer += 4;
				}
				for(; i < numberTerminals; i++) {
					vtCreate(*(vtCoordPointer + 2), *(vtCoordPointer + 3), *vtCoordPointer, *(vtCoordPointer + 1), i);
					vtCoordPointer += 4;
				}
				free(vtCoordinates);
				// draw separation lines
			}
		}
	}
	return terminalIndex;
}

int main() {
	int i, j, k, screenHeight, screenWidth;
	int input;
	char ch;
	char *boundString;
	int ch2;
	char inputBuffer[40];
	int terminalIndex, fdMax;
	fd_set fileDescriptors;
	struct timespec timeout;
	//struct tesiObject *to;
	struct virtualTerminal *vt;

	for(i = 0; i < 10; i++)
		virtualTerminals[i] = NULL;

	fdMax = 0;
	timeout.tv_sec = 0;
	timeout.tv_nsec = 50000000; // 10E-9

#ifdef USE_NCURSES
	ncursesScreen = initscr();
	if(ncursesScreen == NULL) {
		perror("Error initializing nCurses\n");
		exit(1);
	}

	if(has_colors()) {
		start_color();
#ifdef DEBUG
		fprintf(stderr, "max colors: %d\n", COLORS);
		fprintf(stderr, "max color pairs: %d\n", COLOR_PAIRS);
#endif

/*
		for(i = 0; i < COLORS; i++) {
			for(j = 0; j < COLORS; j++) {
				vt_colors[i][j] = k;
				init_pair(k, i, j);
				k++;
			}
		}
*/

		//init all color pairs
		/*
		 * black red green yellow blue magenta cyan white
		 * attributes 30-37
		 * in iterm, black starts at 1
		 * */
	}

	//nodelay(ncursesScreen, TRUE); // return immediately if no input is waiting
	raw();
	noecho(); // don't echo input
	//cbreak();
	//keypad(ncursesScreen, true); // cause nCurses to package key combos into special, single values
	refresh(); // clear the main window

	// Get main window dimensions. This will be used when creating additional virtual terminals
	getmaxyx(ncursesScreen, screenHeight, screenWidth);
#ifdef DEBUG
	fprintf(stderr, "Screen height, width: %i %i\n", screenHeight, screenWidth);
#endif
#endif


	inputBuffer[0] = 0;
	mvwaddstr(ncursesScreen, screenHeight - 1, 0, ":");

	mvwaddstr(ncursesScreen, 1, 0, "USING KNOX\n\nCommands\n\t\"create\" - creates a new Virtual Terminal\n\t\"create #\" - create # number of new Virtual Terminals\n\t\"NUMBER\" - sets focus to so numbered terminal\n\t\"NUMBER COMMAND\" - sends COMMAND to numbered terminal followed by newline");
	wmove(ncursesScreen, screenHeight - 1, 1);
	wnoutrefresh(ncursesScreen);

	terminalIndex = -1;
	keepRunning = 1;
	ch = '\0';
	j = 1;
	while(keepRunning) {
		FD_ZERO(&fileDescriptors);
		FD_SET(0, &fileDescriptors);
		fdMax = 0;
		// do i need to re-add the file descriptors every time?
		for(i = 0; i < 10; i++) {
			vt = vtGet(i);
			if(vt && vt->fdActivity != -1) {
				if(vt->fdActivity > fdMax)
					fdMax = vt->fdActivity;
				FD_SET(vt->fdActivity, &fileDescriptors);
			}
		}
		
		pselect(fdMax + 1, &fileDescriptors, NULL, NULL, &timeout, NULL);
		for(i = 0; i < 10; i++) {
			vt = vtGet(i);
			if(vt != NULL && vt->fdActivity != -1 && FD_ISSET(vt->fdActivity, &fileDescriptors)) {

#ifdef DEBUG
				fprintf(stderr, "VT %i has input\n", i);
#endif
				tesi_handleInput(vt->pointer);
#ifdef DEBUG
				fprintf(stderr, "Done with VT %i input\n", i);
#endif
				//VTCore_dispatch(vt->core);
#ifdef USE_NCURSES
				//vt = (struct virtualTerminal*) vtGet(i)->pointer;
				wnoutrefresh(vt->window);
#endif
				j++;
			}
		}

		if(j) {
			doupdate();
			j = 0;
		}

		// no keyboard input waiting
		if(FD_ISSET(0, &fileDescriptors)) {
			read(0, &ch, 1);
		} else {
			ch = '\0';
			continue;
		}

#ifdef USE_SLANG
			SLsmg_refresh();
		if(!SLang_input_pending(1)) // wait 1/10 of a second
			continue;
		ch = SLang_getkey();
#endif

		switch(ch) {
			case '`': // tilde pressed, cycle through terms?
				terminalIndex++;
				if(terminalIndex == 10 || vtGet(terminalIndex) == NULL) {
					terminalIndex = -1;
				}
				vtHighlight(terminalIndex);
				j++;
				break;
			case ERR: // no input
				break;
			default:
				if(terminalIndex > -1) { // send input to terminal
					//to = vtGet(terminalIndex);
#ifdef DEBUG
					fprintf(stderr, "Get terminal %i\n", terminalIndex);
#endif
					vt = vtGet(terminalIndex);
					if(vt) { // this should never be NULL, but check anyway
						write(vt->fdInput, &ch, 1);
					}

				} else { // build input buffer
#ifdef DEBUG
					fprintf(stderr, "Keypress: %d\n", ch);
#endif
					if(ch == 13) { // parse buffer when Enter is pressed, returns active terminal index
						//wclear(ncursesScreen);
						terminalIndex = processInput(inputBuffer, terminalIndex, screenHeight, screenWidth);
#ifdef DEBUG
						fprintf(stderr, "terminalIndex %d\n", terminalIndex);
#endif
						vtHighlight(terminalIndex);

#ifdef USE_NCURSES
						// clear command window
						mvwaddch(ncursesScreen, screenHeight - 1, 0, ':');
						wmove(ncursesScreen, screenHeight - 1, 1);
						wclrtoeol(ncursesScreen);
						wmove(ncursesScreen, screenHeight - 1, 1);
						wnoutrefresh(ncursesScreen);
						j++;
#endif
						inputBuffer[0] = 0;

					} else {
						i = strlen(inputBuffer);
						inputBuffer[ i ] = ch;
						inputBuffer[ i + 1 ] = 0;
#ifdef DEBUG
						fprintf(stderr, "inputBuffer %s\n", inputBuffer);
#endif
#ifdef USE_NCURSES
						mvwaddstr(ncursesScreen, screenHeight - 1, 1, inputBuffer);
						wnoutrefresh(ncursesScreen);
						j++;
#endif
#ifdef USE_SLANG
						SLsmg_gotorc(SLtt_Screen_Rows - 1, 0);
						SLsmg_write_string(inputBuffer);
						SLsmg_refresh();
#endif
					}
				}
				break;
		}
	}

	for(i = 0; i < 10; i++) {
		if(virtualTerminals[i] != NULL) {
			vtDestroy(i);
		}
	}

#ifdef USE_NCURSES
	endwin();
#endif
#ifdef USE_SLANG
	SLsmg_reset_smg();
	SLang_reset_tty();
#endif
	return 0;
}
