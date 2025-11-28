all:
	gcc -g -I./ -I/usr/local/include -c -fPIC *.c cob/*.c
	gcc -shared -o libradicle.so *.o

clean:
	rm -f *~ *.o *.so
