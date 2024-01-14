//#ifndef UTILS_H
//#define UTILS_H

// keycodes
#define ESC 27
#define ENTR 13
#define CTRL 17
#define U 85
#define W 87
#define P 80
#define N 78
#define LEFT_BRACKET 219
#define H 72

#define VERSION "0.0.0"

#include <stdio.h>
#include <stdlib.h>

void
die(char *prefix, char *msg, int exit_code) {
    fprintf(stderr, "[%s]: %s\n", prefix, msg);
    exit(exit_code);
}

int
convert_arg_to_int(char *arg, int def_arg)
{
    int converted_arg = atoi(arg);
    if (converted_arg <= 0)
        return def_arg;
    else
        return converted_arg;
}

//#endif // UTILS_H
