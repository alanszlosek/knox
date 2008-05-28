CC=gcc
COFLAGS=
CWFLAGS=
CIFLAGS=
CMFLAGS=
CFLAGS= $(CWFLAGS) $(COFLAGS) $(CIFLAGS) $(CMFLAGS)
commandLexer: commandLexer.c
	$(CC) $(CFLAGS) -ocommandLexer commandLexer.c

commandLexer.c: commandLexer.l
	flex -ocommandLexer.c commandLexer.l
