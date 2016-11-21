CFLAGS= -g -Wall -Wextra -Werror -static-libgcc -static-libstdc++
WORK=main
CC=g++

all: $(WORK)

main: dtw.o FFTReal.o libmfccOptim.o WavToMfcc.o main.o
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $^

clean: 
	$(RM) *.o $(WORK)
