#include "tesi.h"
// tesInterpreter
/*
Interprets TESI escape sequences.
See terminfo definition
*/

#define DEBUG 1
#define DEBUG_SEQ_ACTIONS 1

/*
handleInput
	- reads data from file descriptor into buffer
	-

interpretSequence
	- once sequence has been completely read from buffer, it's passed here
	- it is interpreted and makes calls to appropriate callbacks
*/

int tesi_handleInput(struct tesiObject *to) {
	char input[128];
	char *pointer, c;
	int lengthRead;
	int i, j, sequenceLength;
	FILE *f;
	struct pollfd fds[1];

       	// use sequenceLength as a local cache for faster ops?
	// avoid premature optimization ... wait until find bottlenecks
	
	// use poll for it's speed, allows us to call this function at regular intervals without first checking for input
	/*
	fds[0].fd = to->fd_activity;
	fds[0].events = POLLIN | POLLPRI;
	poll(fds, 1, 0);
	if((fds[0].revents & (POLLIN | POLLPRI)) == 0) {
		return 0;
	}
	*/

	lengthRead = read(to->ptyMaster, input, 128);
#ifdef DEBUG
	fprintf(stderr, "TESI got %d characters from the pipe\n", lengthRead);
#endif

	f = fopen("output", "a+");
	fwrite(input, lengthRead, 1, f);
	fclose(f);

	pointer = input;
	for(i = 0; i < lengthRead; i++, pointer++) {
		c = *pointer;
		if(c == 0) { // skip NULL for unicode?
#ifdef DEBUG
			fprintf(stderr, "Skipped a NULL character\n");
#endif
			continue;
		}

		if((c >= 1 && c <= 31) || c == 127) {
#ifdef DEBUG_SEQ_ACTIONS
			fprintf(stderr, "Control character: %i\n", (int)c);
#endif
			// this should trigger partialSequence = 1
			tesi_handleControlCharacter(to, c);
			continue;
		}

		if(to->partialSequence > 0) {
			// where do we set partialSequence to 1? in handleControlCharacter
			// keep track of the sequence type
			to->sequence[ to->sequenceLength++ ] = c;
			to->sequence[ to->sequenceLength ] = 0;
			if(
				(c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z')
			) // done with sequence
				to->partialSequence = 0;

#ifdef DEBUG
			// check to->sequenceLength ... it can't be more than 39
			if(to->sequenceLength > 39)
				fprintf(stderr, "Sequence length exceeded: %i\n", to->sequenceLength);
#endif

			if(to->partialSequence == 0) {
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "Sequence: %s\n", to->sequence);
#endif
				tesi_interpretSequence(to);
				to->sequenceLength = 0;
				to->parametersLength = 0;
			}
		} else { // standard text, not part of any escape sequence
			//if(to->partialSequence == 0) {
				// newlines aren't in visible range, so they shouldn't be a problem
				if(to->insertMode == 0 && to->callback_printCharacter) {
					to->callback_printCharacter(to->pointer, c, to->x, to->y);
					to->x++;
					tesi_limitCursor(to);
				}
				// there is currently no way to enter insert mode
				if(to->insertMode == 1 && to->callback_insertCharacter) {
					to->callback_insertCharacter(to->pointer, c, to->x, to->y);
				}
			//}
		}
	}
	return 1;
}

/*
 * tesi_handleControlCharacter
 * Handles carriage return, newline (line feed), backspace, tab and others
 * Also starts a new escape sequence when ASCII 27 is read
 * */
int tesi_handleControlCharacter(struct tesiObject *to, char c) {
	int i, j;
	// entire switch from Rote
	switch (c) {
		case '\x1B': // begin escape sequence (aborting previous one if any)
			to->partialSequence = 1;
			// possibly flush buffer
			break;

		case '\r': // carriage return ('M' - '@'). Move cursor to first column.
#ifdef DEBUG_SEQ_ACTIONS
			fprintf(stderr, "Carriage return. Move to x=0\n");
#endif
			to->x = 0;
			if(to->callback_moveCursor)
				to->callback_moveCursor(to->pointer, to->x, to->y);
			break;

		case '\n':  // line feed ('J' - '@'). Move cursor down line and to first column.
#ifdef DEBUG_SEQ_ACTIONS
			fprintf(stderr, "Newline. x=0 y++\n");
#endif
			to->y++;
			//if(to->insertMode == 0 && to->linefeedMode == 1)
				//to->x = 0;
			tesi_limitCursor(to);
			if(to->callback_moveCursor)
				to->callback_moveCursor(to->pointer, to->x, to->y);
			break;

		case '\t': // ht - horizontal tab, ('I' - '@')
#ifdef DEBUG_SEQ_ACTIONS
			fprintf(stderr, "Tab. Currently at %d,%d (x,y)\n", to->x, to->y);
#endif
			j = 8 - (to->x % 8);		
			if(j == 0)
				j = 8;
			// loop is necessary so add spaces along the way
			for(i = 0; i < j; i++, to->x++) {
				tesi_limitCursor(to);
				if(to->callback_moveCursor)
					to->callback_moveCursor(to->pointer, to->x, to->y);
				if(to->callback_printCharacter)
					to->callback_printCharacter(to->pointer, ' ', to->x, to->y);
			}
#ifdef DEBUG_SEQ_ACTIONS
			fprintf(stderr, "End of Tab processing. Now at ...\n");
#endif
	 		break;

		case '\a': // bell ('G' - '@')
	 		// do nothing for now... maybe a visual bell would be nice?
#ifdef DEBUG_SEQ_ACTIONS
			fprintf(stderr, "If callback_bell is set, bong!\n");
#endif
			if(to->callback_bell)
				to->callback_bell(to->pointer);
			break;

	 	case 8: // backspace cub1 cursor back 1 ('H' - '@')
	 		// what do i do about wrapping back up to previous line?
	 		// where should that be handled
	 		// just move cursor, don't print space

			// THIS IS NOT BACKSPACE, JUST LEFT ARROW
#ifdef DEBUG_SEQ_ACTIONS
			fprintf(stderr, "Ascii 8, left arrow\n");
#endif
			if (to->x > 0) {
				to->x--;
				
				if(to->callback_moveCursor)
					to->callback_moveCursor(to->pointer, to->x, to->y);
			}
			/*
			if(to->callback_eraseCharacter)
				to->callback_eraseCharacter(to->pointer, to->x, to->y);
			*/
			break;

		default:
#ifdef DEBUG_SEQ_ACTIONS
			fprintf(stderr, "Unrecognized control char: %d (^%c)\n", c, c + '@');
#endif
			return false;
			break;
	}
	return true;
}

/*
 * This skips the [ present in most sequences
 */
void tesi_interpretSequence(struct tesiObject *to) {
	char *p = to->sequence;
	char *q; // another pointer
	char c;
	char *secondChar = to->sequence; // used to test for ? after [
	char operation = to->sequence[to->sequenceLength - 1];
	int i,j;

#ifdef DEBUG
	fprintf(stderr, "Operation: %c\n", operation);
#endif

	// preliminary reset of parameters
	for(i=0; i<6; i++)
		to->parameters[i] = 0;
	to->parametersLength = 0;

	if(*secondChar == '?')
		p++;

	// parse numeric parameters
	q = p;
	c = *p;
	while ((c >= '0' && c <= '9') || c == ';') {
		if (c == ';') {
			//if (to->parametersLength >= MAX_CSI_ES_PARAMS)
				//return;
			to->parametersLength++;
		} else {
			j = to->parameters[ to->parametersLength ];
			j = j * 10;
			j += c - '0';
			to->parameters[ to->parametersLength ] = j;
		}
		p++;
		c = *p;
	}
	if(p != q)
		to->parametersLength++;

	if( (operation >= 'A' && operation <= 'Z') || (operation >= 'a' && operation <= 'z')) {
		int j;
		switch(operation) {
			case 'R': // defaults
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "Reset to default state\n");
#endif
				to->parameters[0] = 0;
				tesi_processAttributes(to);
				to->parameters[0] = 1;
				tesi_processAttributes(to);
				// scroll regions, colors, etc.
				break;
			case 'C': // clear screen
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "Clear screen\n");
#endif

				for(i = 0; i < to->height; i++) {
					if(to->callback_moveCursor)
						to->callback_moveCursor(to->pointer, 0, i);
					if(to->callback_eraseLine)
						to->callback_eraseLine(to->pointer, i);
				}
				to->x = to->y = 0;
				if(to->callback_moveCursor)
						to->callback_moveCursor(to->pointer, to->x, to->y);
				break;

			// LINE-RELATED
			case 'c': // clear line (from current x to end?)
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "If callback_eraseLine is set, clear to EOL\n");
#endif
				if(to->callback_eraseLine)
					to->callback_eraseLine(to->pointer, to->y);
				break;
			case 'I': // insert line
				// should only be done from x = 0
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "If callback_insertLine is set, insert line below current\n");
#endif
				if(to->callback_insertLine)
					to->callback_insertLine(to->pointer, to->y);
				break;
			case 'i': // delete line
				// should only be done from x = 0
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "If callback_insertLine is set, insert line below current\n");
#endif
/*
				if(to->callback_insertLine)
					to->callback_insertLine(to->pointer, to->y);
*/
				break;

			// ATTRIBUTES AND MODES
			case 'a': // change output attributes
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "Attribute change\n");
#endif
				tesi_processAttributes(to);
				break;
/*
			case 'I': // enter/exit insert mode
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "Enter/exit insert mode (does nothing)\n");
#endif
				break;
*/

			// CURSOR RELATED
			case 'M': // cup. move to col, row, cursor to home
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "Move cursor (x,y): %d %d\n", to->x, to->y);
#endif
				// parameters should be 1 more than the real value
				if(to->parametersLength == 0)
					to->x = to->y = 0;
				else {
					to->x = to->parameters[1];
					to->y = to->parameters[0];
				}

				// limit cursor to boundaries
				tesi_limitCursor(to);
				break;

			// SCROLLING RELATED
			// hmm, top does change the scroll region
			case 'S': // change scrolling region
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "Change scrolling region from line %d to %d\n", i, j);
#endif
				if(to->parametersLength == 2) {
					i = to->parameters[0];
					j = to->parameters[1];
					to->scrollBegin = i;
					to->scrollEnd = j;
					if(to->callback_scrollRegion)
						to->callback_scrollRegion(to->pointer, i,j);
				} else {
					//0, 0
				}
				break;
			case 'D': // scroll down
				// this should only be called when cursor is at bottom left
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "If callback_scrollDown is set, scroll down\n");
#endif
				if(to->callback_scrollDown)
					to->callback_scrollDown(to->pointer);
				break;
			case 'U': // scroll up
				// this should only be called when the cursort is at the top left
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "If callback_scrollUp is set, scroll down\n");
#endif
				if(to->callback_scrollUp)
					to->callback_scrollUp(to->pointer);
				// cursor shouldn't change positions after a scroll
				// but this means the next output line (like a new prompt invoked after Enter on last line)
				// will be indented
				break;

			// INPUT RELATED
			// normally you can use arrow keys to get past where you've typed ...
			// how do we prevent that?
			case 'l': // left arrow
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "Left arrow\n");
#endif
				if(to->x > 0) {
				       to->x--;
					tesi_limitCursor(to);
				}
				break;
			case 'r': // right arrow
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "Right arrow\n");
#endif
				if(to->x < to->width) {
					to->x++;
					tesi_limitCursor(to);
				}
				break;
			case 'u': // up arrow
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "Up arrow\n");
#endif
				if(to->y > 0) {
					to->y--;
					tesi_limitCursor(to);
				}
				break;
			case 'd': // down arrow
#ifdef DEBUG_SEQ_ACTIONS
				fprintf(stderr, "Down arrow\n");
#endif
				if(to->y < to->height) {
					to->y++;
					tesi_limitCursor(to);
				}
				break;
		}
	}
}


void tesi_processAttributes(struct tesiObject *to) {
	// 0short bold, 1underline, 2blink, 3reverse, 4foreground, 5background, 6charset, 7i;

	//standout, underline, reverse, blink, dim, bold, foreground, background
	// no need for invisible or dim, right?
	switch(to->parameters[0]) {
		case 0: // all off
			to->attributes[0] = to->attributes[1] = to->attributes[2] = to->attributes[3] = to->attributes[4] = to->attributes[5] = to->attributes[6] = to->attributes[7] = 0;
			to->attributes[4] = 7;
			to->attributes[5] = 0;
			//bold = underline = blink = reverse = foreground = background = charset = 0;
			break;
		case 1: // standout
			// what is our standout mode? fg=black, bg=white
			to->attributes[4] = 0;
			to->attributes[5] = 7;
			break;
		case 2: // underline
			to->attributes[1] = to->parameters[1];
			break;
		case 3: // reverse
			to->attributes[2] = 1;
			break;
		case 4: // blink
			to->attributes[3] = 1;
			break;
		case 5: // bold
			to->attributes[4] = 1;
			break;
		case 6: // foreground color
			to->attributes[4] = to->parameters[1];
			// setf
			// black blue green cyan red magenta yellow white
			// setaf
			// black, red green yellow blue magenta cyan white
			break;
		case 7: // background color
			to->attributes[5] = to->parameters[1];
			break;
	}

	if(to->callback_attributes)
		to->callback_attributes(to->pointer, to->attributes[0], to->attributes[1], to->attributes[2], to->attributes[3], to->attributes[4], to->attributes[5], 0);
}
/*
void tesi_bufferPush(struct tesiObject *to, char c) {
	if(to->outputBufferLength == TESI_OUTPUT_BUFFER_LENGTH) {
		// PRINT
	}
	to->outputBuffer[ to->outputBufferLength++ ] = c;
	//to->sequence[ to->sequenceLength ] = 0; // don't use null as terminator
}
*/

void tesi_limitCursor(struct tesiObject *to) {
	// create some local variables for speed
	int width = to->width;
	int height = to->height;
	int x = to->x;
	int y = to->y;

	if(x < 0)
		to->x = 0;
	if(y < 0)
		to->y = 0;

	if(x >= to->width) {
#ifdef DEBUG
		fprintf(stderr, "Cursor was out of bounds in X direction: %d\n", x);
#endif
		to->x = 0;
		to->y++;
	}

	// when we don't auto scroll, top doesn't work
	// but we get an extra line
	if(to->y >= to->height) {
#ifdef DEBUG
		fprintf(stderr, "Cursor was out of bounds (height %d) in Y direction: %d\n", to->height, y);
#endif
		to->y = to->height - 1;
		// maybe we shouldn't scroll up until we get a newline while at the last line
		// and maybe top thinks we have more lines than we have
		/*
		if(to->callback_scrollUp)
			to->callback_scrollUp(to->pointer);
		*/
	}
}


struct tesiObject* newTesiObject(char *command, int width, int height) {
	struct tesiObject *to;
	struct winsize ws;
	char message[256];
	char *ptySlave;
	
	to = malloc(sizeof(struct tesiObject));
	if(to == NULL)
		return NULL;

	to->partialSequence = 0;
	// we need to ensure that we don't blow this length
	to->sequence = malloc(sizeof(char) * 40); // escape sequence buffer
	to->sequence[0] = 0;
	to->sequenceLength = 0;
	//to->outputBuffer[0] = 0;
	//to->outputBufferLength = 0;
	to->parametersLength = 0;

	to->x = to->y = to->x2 = to->y2 = 0; // cursor x,y and window width,height
	to->width = width;
	to->height = height;
	to->scrollBegin = 0;
	to->scrollEnd = height - 1;
	to->insertMode = 0;
	
	to->attributes[0] = to->attributes[1] = to->attributes[2] = to->attributes[3] = to->attributes[4] = to->attributes[5] = to->attributes[6] = to->attributes[7] = 0;
	to->attributes[4] = 0;
	to->attributes[5] = 7;

	to->callback_printCharacter = NULL;
	to->callback_printString = NULL;
	to->callback_insertCharacter = NULL;
	to->callback_eraseLine = NULL;
	to->callback_eraseCharacter = NULL;
	to->callback_moveCursor = NULL;
	to->callback_attributes = NULL;
	to->callback_scrollUp = NULL;
	to->callback_scrollDown = NULL;
	to->callback_bell = NULL;
	to->callback_invertColors = NULL;

	//to->callback_processReturned = NULL;

	to->command[0] = to->command[1] = to->command[2] = NULL;
	to->pid = 0;
	to->ptyMaster = posix_openpt(O_RDWR|O_NOCTTY);
	
	if(to->ptyMaster == -1 || grantpt(to->ptyMaster) == -1 || unlockpt(to->ptyMaster) == -1) {
		printf("Failed to open communication pipe\n");
	}

	to->fd_activity = to->ptyMaster; // descriptor to check whether the process has sent output
	to->fd_input = to->ptyMaster; // descriptor for sending input to child process

#ifdef DEBUG
		fprintf(stderr, "Before fork. process id: %i parent process id: %i session id: %i process group id: %i\n", (int)getpid(), (int)getppid(), getsid(0), (int)getpgid(0));
#ifdef DEBUG
		fprintf(stderr, "session id: %i process group id: %i\n", getsid(0), (int)getpgid(0));
#endif
#endif

	to->pid = fork();
	//setpgid(to->pid, to->pid); // set new process group id for easy killing of shell children
	if(to->pid == 0) { // inside child
		// hmm, where do we tell the forked process to use the new pty? i can't remember

		setsid(); // become session leader so bash job control will work

		// before reset file descriptors, print process and group ids?
#ifdef DEBUG
		fprintf(stderr, "After fork. process id: %i parent process id: %i session id: %i process group id: %i\n", (int)getpid(), (int)getppid(), getsid(0), (int)getpgid(0));
#endif
		ptySlave = ptsname(to->ptyMaster);
		to->ptySlave = open(ptySlave, O_RDWR);
		
		// need ot turn off echoing in the child
		// dup fds to stdin and stdout, or something like, then exec /bin/bash
		dup2(to->ptySlave, fileno(stdin));
		dup2(to->ptySlave, fileno(stdout));
		dup2(to->ptySlave, fileno(stderr));

		//clearenv(); // don't clear because we want to keep all other variables
		setenv("TERM","tesi",1); // vt102
		sprintf(message, "%d", width);
		setenv("COLUMNS", message, 1);
		sprintf(message, "%d", height);
		setenv("LINES", message, 1);

		//fflush(stdout); // flush output now because it will be cleared upon execv

		if(execl(command, command, NULL) == -1)
		//if(execl("/bin/bash", "/bin/bash", NULL) == -1)
		//if(execl("/bin/cat", "/bin/cat", NULL) == -1)
			exit(EXIT_FAILURE); // exit and become zombie until parent cares....
	}
	return to;
}
/*
 * Why does this take a void pointer?
 * So that you don't have to cast before you pass the parameter
 * */
void deleteTesiObject(void *p) {
	struct tesiObject *to = (struct tesiObject*) p;

	// kill if process is still running
	//kill(-(getpgid(to->pid)), SIGTERM); // kill all with this process group id
	kill(to->pid, SIGTERM); // probably don't need this line
	waitpid(to->pid);

	close(to->ptyMaster);

	free(to->sequence);
	free(to->command[0]);
	if(to->command[1])
		free(to->command[1]);
	free(to);
}
