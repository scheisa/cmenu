build:
	gcc -o cmenu main.c -lgdi32 -Wextra -Wall -pedantic

run:
	gcc -o cmenu main.c -lgdi32 -Wextra -Wall -pedantic
	./cmenu

build_o:
	gcc -o cmenu main.c -lgdi32 -O2
