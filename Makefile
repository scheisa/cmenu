build:
	gcc -o cmenu main.c -lgdi32 -Wextra -Wall -pedantic -municode

run:
	gcc -o cmenu main.c -lgdi32 -Wextra -Wall -pedantic
	./cmenu

build_o:
	gcc -o cmenu main.c -lgdi32 -O2
