all: pa4

pa4: first.c
	gcc -Wall -Werror -fsanitize=address -std=c11 first.c -lm -o first

clean:
	rm -f first

