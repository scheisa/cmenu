build:
	gcc -o cmenu main.c -lgdi32 -Wextra -Wall -pedantic -municode

build_o:
	gcc -o cmenu main.c -lgdi32 -municode -O2
