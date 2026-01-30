all:
	gcc -g -I./ -I../radicle -I/usr/local/include -o cradicle-node main.c command.c -L../radicle -L/usr/local/lib64 -lradicle -lssh -lgit2 -ljson-c -lsqlite3
clean:
	rm -f *~ cradicle-node
