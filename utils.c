// #ifndef UTILS_H
// #define UTILS_H

#include <stdio.h>
#include <stdlib.h>

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

#define VERSION "0.0.1"

void
die(char *prefix, char *msg, int exit_code)
{
    fprintf(stderr, "[%s]: %s\n", prefix, msg);
    exit(exit_code);
}

void
usage_short(void)
{
    die("USAGE", "cmenu [-vi] [-p prompt] [-l lines] [-f font size]", 0);
}

int
convert_arg_to_int(wchar_t *arg, int def_arg)
{
    int converted_arg = _wtoi(arg);
    if (converted_arg <= 0)
        return def_arg;
    else
        return converted_arg;
}

// #endif // UTILS_H
