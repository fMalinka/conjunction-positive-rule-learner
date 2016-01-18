CC=g++-4.9
CFLAGS=-std=c++11 #-Wall
CLIBRARY=-lboost_regex

all:
	$(CC) $(CFLAGS) conjunction_learning.cpp -o conjunction_learning

run:
	$(CC) $(CFLAGS) conjunction_learning.cpp -o conjunction_learning
	./conjunction_learning
