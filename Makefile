.PHONY:clean

CC = g++
CCFLAGS = -std=c++11 -Wall
ASSEMBLER_SOURCE = assembler.cc
EXECUTOR_SOURCE = executor.cc

all: assembler executor

assembler: $(ASSEMBLER_SOURCE)
	$(CC) $(ASSEMBLER_SOURCE) -o assembler $(CCFLAGS)

executor: $(EXECUTOR_SOURCE)
	$(CC) $(EXECUTOR_SOURCE) -o executor $(CCFLAGS)

clean:
	rm -f assembler executor
