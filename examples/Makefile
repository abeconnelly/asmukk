CC=gcc

all: dp mktest

dp: dp.c
	$(CC) $< -o $@

mktest: mktest.c
	$(CC) $< -o $@

test:
	./check_ukk.sh

clean:
	rm dp mktest *~
