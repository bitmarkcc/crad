all:
	gcc -g -I./ -I../radicle -I/usr/local/include -o rad main.c command.c commands/auth.c -L../radicle -L/usr/local/lib64 -lradicle -lssh
clean:
	rm -f *~
	rm -f rad
	rm -f commands/*~

