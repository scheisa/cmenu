#include <io.h>
#include <locale.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <windows.h>
#include <windowsx.h>
#include <wingdi.h>
#include <winuser.h>

#include "config.h"
#include "utils.c"

// TODO's:
// move some functions to utils.c
// move ui functions to a separate file
// in match function change handling lowercase case
// crashing after deleting charters in input_field
// crashing after typing something that matches nothing for too long
// case insensitivity

int screen_width, screen_height, win_height;

// items
typedef struct {
    wchar_t *str;
    int substr_index;
} item;
item *matched_items = NULL;
unsigned int matched_items_amount;
item *items = NULL;
unsigned int items_amount = 0;

HWND *items_hwnd = NULL;
const unsigned int CHUNK = 512;
unsigned int items_capacity = CHUNK;

unsigned int selected_index = 0;

// colors
int accent_color, bg_color, fg_color_norm, fg_color_sel;

// prompt
wchar_t **prompt = &prompt_text;
// size
struct tagSIZE prompt_size, letter_size;

int case_sensitive = 1;
int is_at_the_bottom = 0;

// api stuff
HWND hwnd, input_field, prompt_c;
HFONT font;
HDC hdc;
const char class_name[] = "cmenu";

int
hex_to_rgb(const char *hex)
{
    unsigned int r, g, b;
    sscanf(hex, "#%02X%02X%02X", &r, &g, &b);
    return RGB(r, g, b);
}

int
compare_substr_index(const void *i1, const void *i2)
{
    return (((item *)i1)->substr_index) - (((item *)i2)->substr_index);
}

void
draw_matched_items(void)
{
    for (unsigned int i = 0; i < matched_items_amount; i++)
        SetWindowTextW(items_hwnd[i], matched_items[i].str);
    for (unsigned int i = matched_items_amount; i < lines; i++)
        SetWindowTextW(items_hwnd[i], L"");
}

void
draw_blank_items(void)
{
    for (unsigned int i = 0; i < lines; i++) {
        SetWindowTextW(items_hwnd[i], L"");
    }
}

void
draw_def_items(void)
{
    for (unsigned int i = 0; i < lines; i++) {
        SetWindowTextW(items_hwnd[i], items[i].str);
    }
}

int
match(wchar_t *input)
{
    int input_len = GetWindowTextLengthA(input_field);
    char buffer[input_len];
    GetWindowTextA(input_field, buffer, input_len + 1);

    matched_items_amount = 0;
    for (unsigned int i = 0; i < items_amount; i++) {
        // TODO: some fix for case insensitivity option (items are displayed in
        // lowercase after applying of this)
        wchar_t *substr = (case_sensitive)
                              ? wcsstr(items[i].str, input)
                              : wcsstr(wcslwr(items[i].str), wcslwr(input));

        if (substr != NULL) {
            int substr_index = substr - items[i].str;

            matched_items[matched_items_amount].substr_index = substr_index;
            matched_items[matched_items_amount].str = items[i].str;

            matched_items_amount++;
        }
    }

    if (matched_items_amount == 0)
        return matched_items_amount;

    qsort(matched_items, matched_items_amount, sizeof(item),
          compare_substr_index);

    // debugging
    // for (unsigned int i = 0; i < matched_items_amount; i++) {
    //     printf("%s\n", matched_items[i].str);
    // }

    return matched_items_amount;
}

void
delete_last_char(void)
{
    int input_len = GetWindowTextLengthA(input_field);
    if (input_len > 0)
        SendMessageA(input_field, EM_SETSEL, (WPARAM)input_len - 1,
                     (LPARAM)input_len);
}

void
delete_last_word(void)
{
    int input_len = GetWindowTextLengthA(input_field);
    char buffer[input_len];

    GetWindowTextA(input_field, buffer, input_len + 1);
    if (strcmp(buffer, "") == 0)
        return;

    int first_space_index = -1;

    if (buffer[input_len - 1] == ' ')
        input_len -= 2;
    for (int i = input_len - 1; i >= 0; i--) {
        if (buffer[i] == ' ') {
            first_space_index = i;
            break;
        }
    }

    if (first_space_index != -1) {
        SendMessageA(input_field, EM_SETSEL, first_space_index, strlen(buffer));
        // Delete the selected text
        SendMessageA(input_field, WM_CLEAR, 0, 0);
    } else {
        SetWindowTextA(input_field, "");
    }
}

void
free_items(void)
{
    for (unsigned int i = 0; i < items_amount; i++) {
        free(items[i].str);
    }
    free(items);

    free(items_hwnd);

    free(matched_items);
}

void
quit(void)
{
    PostQuitMessage(0);
    free_items();
}

void
return_selection(void)
{
    wchar_t *selected_str;
    if (matched_items_amount > 0)
        selected_str = matched_items[selected_index].str;
    else
        selected_str = items[selected_index].str;

    fprintf(stdout, "%ls\n", selected_str);
    quit();
}

void
move_selection_down(void)
{
    item *items_to_display;
    unsigned int amount_of_items;

    size_t input_len = GetWindowTextLengthA(input_field);
    if (input_len == 0) {
        items_to_display = items;
        amount_of_items = items_amount;
    } else if (input_len > 0 && matched_items_amount > 0) {
        items_to_display = matched_items;
        amount_of_items = matched_items_amount;
    } else if (input_len > 0 && matched_items_amount == 0)
        return;

    if (selected_index < amount_of_items - 1) {
        selected_index++;
        for (unsigned int i = 0; i < lines; i++) {
            if ((selected_index + i) < amount_of_items) {
                SetWindowTextW(items_hwnd[i],
                               items_to_display[(selected_index + i)].str);
            } else {
                SetWindowTextW(items_hwnd[i], L"");
            }
        }
    }
}

void
move_selection_up(void)
{
    item *items_to_display;
    unsigned int amount_of_items;

    size_t input_len = GetWindowTextLengthA(input_field);
    if (input_len == 0) {
        items_to_display = items;
        amount_of_items = items_amount;
    } else if (input_len > 0 && matched_items_amount > 0) {
        items_to_display = matched_items;
        amount_of_items = matched_items_amount;
    } else if (input_len > 0 && matched_items_amount == 0)
        return;

    if (selected_index > 0) {
        selected_index--;
        for (unsigned int i = 0; i < lines; i++) {
            if ((selected_index + i) < amount_of_items) {
                SetWindowTextW(items_hwnd[i],
                               items_to_display[(selected_index + i)].str);
            } else {
                SetWindowTextW(items_hwnd[i], L"");
            }
        }
    }
}

void
handle_keypress(WPARAM key)
{
    switch (key) {
    case ESC: // closing the programm
        quit();
        break;
    case ENTR: // return selected option in stdin
        return_selection();
        break;
    }
    // execute this only if CTRL is pressed
    if ((GetKeyState(CTRL) & 0x8000) != 0) {
        switch (key) {
        case N: // moving selection down
            move_selection_down();
            break;
        case P: // moving selection up
            move_selection_up();
            break;
        case U: // clearing the input field
            SetWindowTextA(input_field, "");
            break;
        case W: // deletes last word from input field
            delete_last_word();
            break;
        case H: // deletes last char from input field
            delete_last_char();
            break;
        case LEFT_BRACKET: // closing the programm
            quit();
            break;
        }
    }
}

LRESULT CALLBACK
input_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_KEYDOWN:
        handle_keypress(w_param);
        break;

    // exit ihf focus was killed
    case WM_KILLFOCUS:
        quit();
    }

    // Call the original edit control procedure
    return CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd,
                          msg, w_param, l_param);
}

// font thing
int CALLBACK
set_font(HWND child, LPARAM font)
{
    SendMessage(child, WM_SETFONT, font, 1);
    return 1;
}

void
place_controls(HWND hwnd)
{
    // creating a font
    font = CreateFontA(font_size, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                       DEFAULT_QUALITY, VARIABLE_PITCH, "Arial");

    // getting size of string of text
    hdc = GetDC(hwnd);
    SelectObject(hdc, font);
    GetTextExtentPoint32A(hdc, "a", 1, &letter_size);

    GetTextExtentPoint32W(hdc, *prompt, wcslen(*prompt), &prompt_size);
    prompt_c = CreateWindowW(L"Static", *prompt, WS_CHILD | WS_VISIBLE, 0, 0,
                             prompt_size.cx, letter_size.cy, hwnd, (HMENU)1,
                             NULL, NULL);

    items_hwnd = malloc(sizeof(HWND) * lines);
    if (items_hwnd == NULL)
        die("ERROR", "unable to allocate memory", 1);

    // creating static controls
    for (unsigned int i = 0; i < lines; i++) {
        items_hwnd[i] = CreateWindowW(
            L"Static", items[i].str, WS_CHILD | WS_VISIBLE, prompt_size.cx + 0,
            letter_size.cy + letter_size.cy * i, screen_width, letter_size.cy,
            hwnd, (HMENU)1, NULL, NULL);
    }

    input_field =
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE, prompt_size.cx + 0,
                      0, screen_width, letter_size.cy, hwnd, NULL, NULL, NULL);

    // setting font for all child windows
    EnumChildWindows(hwnd, (WNDENUMPROC)set_font, (LPARAM)font);

    // setting subclass for input field
    SetWindowLongPtr(input_field, GWLP_USERDATA,
                     (LONG_PTR)GetWindowLongPtr(input_field, GWLP_WNDPROC));
    SetWindowLongPtr(input_field, GWLP_WNDPROC, (LONG_PTR)input_proc);

    // setting foucs on input field
    SetFocus(input_field);
}

LRESULT
edit_color(WPARAM w_param)
{
    SetBkMode((HDC)w_param, TRANSPARENT);
    SetTextColor((HDC)w_param, fg_color_norm);
    SetBkColor((HDC)w_param, bg_color);
    return (LRESULT)CreateSolidBrush(bg_color);
}

LRESULT
static_color(WPARAM w_param, LPARAM l_param)
{
    int color, fg_color;

    if ((HWND)l_param == items_hwnd[0] || (HWND)l_param == prompt_c) {
        fg_color = fg_color_sel;
        color = accent_color;
    } else {
        fg_color = fg_color_norm;
        color = bg_color;
    }
    SetTextColor((HDC)w_param, fg_color);
    SetBkColor((HDC)w_param, color);

    return (LRESULT)CreateSolidBrush(color);
}

// the window procedure
LRESULT
win_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    // debugging kekwait
    // printf("MESSAGE -> \"%d\"\n", msg);

    switch (msg) {
    case WM_CREATE:
        place_controls(hwnd);
        break;

    case WM_CTLCOLORSTATIC:
        return static_color(w_param, l_param);
        break;

    case WM_CTLCOLOREDIT:
        return edit_color(w_param);
        break;

    case WM_COMMAND:
        if (HIWORD(w_param) == EN_CHANGE) {
            size_t input_len = GetWindowTextLengthA(input_field);

            if (input_len > 0) {
                wchar_t *buffer = malloc(input_len + 1);
                GetWindowTextW(input_field, buffer, input_len + 1);

                if (match(buffer) < 0)
                    printf("%ls\n", buffer);

                // if nothing was matched
                if (matched_items_amount == 0) {
                    draw_blank_items();
                    selected_index = 0;
                }
                if (matched_items_amount > 0) {
                    draw_matched_items();
                }

            } else {
                draw_def_items();
                selected_index = 0;
            }
        }
        break;

    case WM_CLOSE: /* FALLTROUGH */
    case WM_DESTROY:
        quit();
        break;
    }

    return DefWindowProc(hwnd, msg, w_param, l_param);
}

void
draw_window(void)
{
    WNDCLASSA wcl = {0};
    MSG msg;

    // registering the window class
    wcl.lpfnWndProc = win_proc;
    wcl.lpszClassName = class_name;
    wcl.hbrBackground = (HBRUSH)CreateSolidBrush(bg_color);

    if (!RegisterClassA(&wcl)) {
        die("ERROR", "can't register a class", 1);
    }

    // creating the window
    hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"cmenu", L"", WS_OVERLAPPEDWINDOW,
                           0, 0, 0, 0, NULL, NULL, NULL, NULL);
    if (hwnd == NULL) {
        die("ERROR", "unable to create a window", 1);
    }

    // height of window
    win_height = letter_size.cy + (lines * letter_size.cy);
    // resizing window
    if (is_at_the_bottom)
        MoveWindow(hwnd, 0, screen_height - win_height, screen_width, win_height, 1);
    else
        MoveWindow(hwnd, 0, 0, screen_width, win_height, 1);

    // disabling beeping
    SystemParametersInfo(SPI_SETBEEP, 0, NULL, SPIF_SENDCHANGE);

    // removing borders
    SetWindowLongPtr(hwnd, GWL_STYLE,
                     GetWindowLongPtr(hwnd, GWL_STYLE) &
                         ~(WS_SIZEBOX | WS_DLGFRAME));
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    ShowWindow(hwnd, SW_SHOWNORMAL);

    // main loop
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        // debugging
        // printf("wParam -> \"%llu\"\n", msg.wParam);
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

void
read_stdin(void)
{
    const size_t max_buffer_len = 256;
    wchar_t buffer[max_buffer_len];

    // if nothing was piped
    if (_isatty(_fileno(stdin))) {
        free_items();
        usage_short();
    }

    while (fgetws(buffer, max_buffer_len, stdin) != NULL) {
        int buffer_len = wcslen(buffer);
        if (buffer[buffer_len - 1] == '\n')
            buffer[buffer_len - 1] = '\0';

        if (items_amount == items_capacity) {
            items_capacity += CHUNK;
            items = realloc(items, items_capacity * sizeof(item));
        }

        items[items_amount].str = malloc(sizeof(char *) * buffer_len);
        wcscpy(items[items_amount].str, buffer);
        items_amount++;
    }

    matched_items = malloc((sizeof(item) * items_amount));
    if (matched_items == NULL)
        die("ERROR", "unable to allocate memory", 0);
}

int
wmain(int argc, wchar_t *argv[])
{
    setlocale(LC_ALL, "en_US.UTF-8");
    system("chcp 65001 > NUL");

    // colors
    bg_color = hex_to_rgb(SchemeNorm);
    accent_color = hex_to_rgb(SchemeSel);
    fg_color_norm = hex_to_rgb(FontNorm);
    fg_color_sel = hex_to_rgb(FontSel);

    screen_width = GetSystemMetrics(SM_CXSCREEN);
    screen_height = GetSystemMetrics(SM_CYSCREEN);

    items = malloc(sizeof(item) * CHUNK);
    if (items == NULL)
        die("ERROR", "unable to allocate memory", 0);

    // parsing flags
    for (int i = 1; i < argc; i++) {
        if (!wcscmp(L"-v", argv[i]))
            die("VERSION", VERSION, 0);
        if (!wcscmp(L"-i", argv[i])) /* case_sensitive = 0; */
            die("INFO",
                "case insensitivity is not working as it should for now", 0);
        if (!wcscmp(L"-b", argv[i]))
            is_at_the_bottom = 1;
        else if (i + 1 == argc)
            usage_short();
        // flags with parametrs
        else if (!wcscmp(argv[i], L"-p")) {
            prompt = &argv[++i];
        } else if (!wcscmp(argv[i], L"-l")) {
            // if cannot be converted to integer use standart value from config
            lines = convert_arg_to_int(argv[++i], lines);
        } else if (!wcscmp(argv[i], L"-f")) {
            // if cannot be converted to integer use standart value from config
            font_size = convert_arg_to_int(argv[++i], font_size);
        } else if (!wcscmp(argv[i], L"-o")) {
            for (int j = i + 1; j < argc; j++) {
                if (items_amount == items_capacity) {
                    items_capacity += CHUNK;
                    items = realloc(items, items_capacity * sizeof(item));
                }

                items[items_amount].str =
                    malloc(sizeof(wchar_t *) * wcslen(argv[j]));
                wcscpy(items[items_amount].str, argv[j]);
                items_amount++;
            }

            break;
        } else
            usage_short();
    }

    if (items_amount == 0)
        read_stdin();

    // amount of shown lines can't be higher then amount of items
    lines = (lines > items_amount) ? items_amount : lines;

    draw_window();

    return 0;
}
