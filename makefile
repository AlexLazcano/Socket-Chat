CFLAGS = -pthread -o talk

default: talk 
	gcc -g -o talk talk.c list.c -pthread


talk: talk.o list.o
	gcc -g $(CFLAGS) talk.o list.o 

talk.o: talk.c 
	gcc -g -c talk.c

list.o: list.c
	gcc -g -c list.c


hello: client.c
	gcc -o client client.c





clean:
	-rm talk
	-rm *.o