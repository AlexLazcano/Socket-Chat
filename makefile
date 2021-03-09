all: talk.c 
	gcc -g -o talk talk.c list.c -pthread


o: talk.c
	gcc -c talk talk.c



clean:
	-rm talk
	-rm *.o