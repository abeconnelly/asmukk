CC=gcc
CPP=g++
CFLAGS=-I.

all: asm_ukk.o asm_ukk asm_ukk.a

asm_ukk.o: asm_ukk.c asm_ukk.h
	$(CC) -c -o $@ $< $(CFLAGS)

asm_ukk: asm_ukk.o asm_ukk_main.cpp
	$(CPP) -o $@ $^ $(CFLAGS)

asm_ukk.a: asm_ukk.o
	ar rcs $@ $^

clean:
	rm -f *.o *.a *~ asm_ukk
