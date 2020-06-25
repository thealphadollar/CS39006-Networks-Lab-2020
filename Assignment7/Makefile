user: librsocket.a
	gcc user1.c -L. -l rsocket -o user1
	gcc user2.c -L. -l rsocket -o user2
librsocket.a: rsocket.o
	ar rcs librsocket.a rsocket.o
rsocket.o: rsocket.c
	gcc -o rsocket.o -c rsocket.c
clean:
	rm *.a *.o client serv