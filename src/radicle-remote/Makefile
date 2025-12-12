all:
	gcc -g -I./ -I../radicle -I/usr/local/include -o git-remote-rad main.c push.c list.c fetch.c -L../radicle -L/usr/local/lib64 -lradicle -lssh -lgit2 -ljson-c
clean:
	rm -f *~ git-remote-rad
