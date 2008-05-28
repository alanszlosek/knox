#include <stdlib.h>
#include <slang.h>
#include <pcre.h>
#include "vt.h"

void processInput(char *input) {
	int *vtCoordinates, *vtCoordPointer;
	int numberTerminals;
	int i;
	long int terminalNumber;
	struct tesiObject *to;
	char *pNumberEnd;

	/*
	pcre *reNumber;
	const char *error;
	int erroffset;
	int matches;
	int ovector[5];

	reNumber = pcre_compile(
		"^[0-9]+$",          // the pattern
		0,                // default options
		&error,           // for error message
		&erroffset,       // for error offset
		NULL);            // use default character tables

	*/

	numberTerminals = 0;
	for(i = 0; i < 10; i++) {
		if(vtGet(i) != NULL)
			numberTerminals++;
	}
	terminalNumber = -1;

	/*
	if(pcre_exec(
	reNumber, // result of pcre_compile()
	NULL, // we didn't study the pattern
	inputBuffer, // the subject string
	strlen(inputBuffer), // the length of the subject string
	0, // start at offset 0 in the subject
	0, // default options
	ovector, // vector of integers for substring information
	5) > 0) { // number of elements (NOT size in bytes)
	*/

	//terminalNumber = atoi(input);
	terminalNumber = strtol(input, &pNumberEnd, 10);

	if(terminalNumber > 0) {
		numberTerminals += terminalNumber;
		// move terms around if not all positions are filled but last one is
		// array of terminals needs to reflect the order they are on the screen
		vtCoordPointer = vtCoordinates = divideRectangle_favorHeight(SLtt_Screen_Cols, SLtt_Screen_Rows - 2, numberTerminals, 1);
		for(i = 0; i < 10; i++) {
			to = vtGet(i);
			if(to) {
				vtMove(i, *vtCoordPointer, *(vtCoordPointer + 1), *(vtCoordPointer + 2), *(vtCoordPointer + 3)); 
			} else // found a spot to put our new terminal(s)
				break;

			vtCoordPointer += 4;
		}
		for(i; i < numberTerminals; i++) {
			vtCreate(*(vtCoordPointer + 2), *(vtCoordPointer + 3), *vtCoordPointer, *(vtCoordPointer + 1), i);
			vtCoordPointer += 4;
		}
		free(vtCoordinates);
	}
}

int main() {
	int i, j;
	int keepRunning;
	int input;
	unsigned int ch;
	int ch2;
	char inputBuffer[40];
	int terminalIndex;
	struct tesiObject *to;

	for(i = 0; i < 10; i++)
		tesiObjects[i] = NULL;

	SLtt_get_terminfo();
	SLang_init_tty(-1, 0, 0);
	SLsmg_init_smg();
	SLsmg_refresh();

	inputBuffer[0] = 0;

	terminalIndex = -1;
	keepRunning = 1;
	while(keepRunning) {
		j = 0;
		for(i = 0; i < 10; i++) {
			if(tesiObjects[i] != NULL)
				j += tesi_handleInput(tesiObjects[i]);
		}
		if(j)
			SLsmg_refresh();

		if(!SLang_input_pending(1)) // wait 1/10 of a second
			continue;
		ch = SLang_getkey();

		switch(ch) {
			case '`': // tilde pressed, cycle through terms?
				terminalIndex++;
				if(terminalIndex == 10 || vtGet(terminalIndex) == NULL)
					terminalIndex = -1;
				// highlight selected terminal
				break;
			case 'Q':
				keepRunning = 0;
				break;
			default:
				if(terminalIndex > -1) { // send input to terminal
					to = vtGet(terminalIndex);
					if(to) // this should never be null, but check anyway
						write(to->fd_input, &ch, 1);

				} else { // build input buffer
#ifdef DEBUG
					fprintf(stderr, "Keypress: %d\n", ch);
#endif
					if(ch == 13) { // parse buffer when Enter is pressed
						// new terminal
						processInput(inputBuffer);
						inputBuffer[0] = 0;

					} else {
						i = strlen(inputBuffer);
						inputBuffer[ i ] = ch;
						inputBuffer[ i + 1 ] = 0;
						SLsmg_gotorc(SLtt_Screen_Rows - 1, 0);
						SLsmg_write_string(inputBuffer);
						SLsmg_refresh();
					}
				}
				break;
		}
	}

	for(i = 0; i < 10; i++) {
		if(tesiObjects[i] != NULL) {
			vtDestroy(i);
		}
	}

	SLsmg_reset_smg();
	SLang_reset_tty();
	return 0;
}
