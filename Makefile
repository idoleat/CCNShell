ccnshell: linenoise.h linenoise.c


ccnshell: linenoise.c ccnshell.c
	$(CC) -Wall -W -Os -g -o ccnshell linenoise.c ccnshell.c

clean:
	rm -f ccnshell
