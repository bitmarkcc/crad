all:
	gcc -g -I./ -I../radicle -I/usr/local/include -o crad main.c command.c commands/auth.c commands/init.c commands/clone.c commands/validate.c -L../radicle -L/usr/local/lib64 -lradicle -lssh -lgit2 -ljson-c
clean:
	rm -f *~ crad commands/*~
