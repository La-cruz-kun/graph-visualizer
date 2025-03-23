main: main.c
	gcc main.c  -g -o main -lraylib -std=c99 -Wall -Wextra

test: test.c
	clang test.c -g -o main -lraylib -std=c99 -Wall -Wextra
