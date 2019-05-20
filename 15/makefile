CC = g++

myServer:main.o http_conn.o threadpool.h  heap_timer.h http_conn.o
	$(CC) *.o -o myServer -lpthread

main.o: main.c http_conn.h threadpool.h heap_timer.h
	$(CC) -c main.o

http_conn.o: http_conn.h  heap_timer.h http_conn.cpp
	$(CC) -c  http_conn.cpp

heap_timer.o: heap_timer.h http_conn.h heap_timer.cpp
	$(CC) -c  heap_timer.cpp
