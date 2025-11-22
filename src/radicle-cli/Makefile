all:
	gcc -g -I./ -I../radicle -I/usr/local/include -o rad main.c command.c commands/auth.c commands/init.c -L../radicle -L/usr/local/lib64 -lradicle -lssh -lgit2 -ljson-c
clean:
	rm -f *~ rad commands/*~
