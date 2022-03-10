#include "print.h"

void highlight(char *entry[], int *maxlen, int *pos, int *len)
{
    int i;
    for (i = 0; i < *maxlen; ++i) {
        if (i < *len) {
            printf("%s%c", bg_cyan, entry[*pos][i]);
        } else if (i == *maxlen - 1) {
            printf(" %s", bg_reset);
        } else {
            printf(" %s", bg_cyan);
        }
    }
}

void print_no_scroll(Window *w, char *entry[], int *y, int *pos, int *maxlen, int up)
{
    if (up) {
        gotoyx(*y + 1, w->x_beg);
        del_ncharc2right(*maxlen);
        printf("%s", entry[*pos + 1]);
        gotoyx(*y, w->x_beg);
    } else {
        gotoyx(*y - 1, w->x_beg);
        del_ncharc2right(*maxlen);
        printf("%s", entry[*pos  - 1]);
        gotoyx(*y, w->x_beg);
    }
}

/*
void print_scroll(Window *w, Scroll *s, char *entry[], int SIZE, int *pos, int *maxlen, int up)
{
    if (up) {
        --s->pos_upper_t;
        --s->pos_lower_t;
        ++s->n_lower_t;
        --s->n_upper_t;
    } else {
        ++s->n_upper_t;
        s->pos_upper_t = s->n_upper_t;
        --s->n_lower_t;
        ++s->pos_lower_t;
    }
    // reprint all 10 decalees de 1 a partir de pos
    int i;
    for (i = 0; i < SIZE; ++i) { // int j = pos;, ++j
        gotoyx(i + w->y_beg, w->x_beg);
        del_ncharc2right(*maxlen);
        printf("%s", entry[s->n_upper_t + i]); // j
    }
    gotoyx(*pos - s->pos_upper_t + w->y_beg, w->x_beg);
    //gotoyx(s.n_to_print + w.y_beg - 1, w.x_beg);
    del_ncharc2right(*maxlen);
}
*/

void print_debug_info(Window *w, Scroll *s, int *pos, int *sz, int *y)
{
    gotoyx(2, (w->x_size / 2) - 20);
    del_ncharc2right(42);
    printf("pos: %d, s.n_upper_t: %d, s.n_lower_t: %d",
            *pos, s->n_upper_t, s->n_lower_t);
    gotoyx(3, (w->x_size / 2) - 20);
    del_ncharc2right(56);
    printf("s.pos_upper_t: %d, s.pos_lower_t: %d, s.n_to_print: %d",
            s->pos_upper_t, s->pos_lower_t, s->n_to_print);

    gotoyx(w->y_beg, (w->x_size / 2) - 20);
    del_ncharc2right(44);
    printf("__________________ s.pos_upper_t: %d, y: %d", s->pos_upper_t, *y);

    gotoyx(w->y_beg + s->n_to_print - 1, (w->x_size / 2) - 20);
    del_ncharc2right(54);
    printf("__________________ s.pos_lower_t: %d, s.n_lower_t: %d",
            s->pos_lower_t, s->n_lower_t);

    gotoyx(w->y_size - 1, w->x_size - 15);
    printf("y: %d, x: %d", w->y_size, w->x_size);

    if (*pos > s->n_to_print - 1 && s->pos_lower_t < *sz) {
        gotoyx(1, 1);
        printf("pos: %d > s.n_to_print: %d", *pos, s->n_to_print);
        gotoyx(1, 150);
        printf("%d", s->n_to_print + w->y_beg - 1);
    }
}

Scroll set_scroll(int pos, int sz)
{
    Scroll s;
    s.array_size = sz;
    s.n_to_print = 10; // w.y_size;
    s.n_upper_t = pos;
    s.n_lower_t = sz - s.n_to_print;

    s.pos_upper_t = pos;
    s.pos_lower_t = s.n_to_print - 1;
    return s;
}

void move_up(Window *w, Scroll *s, char *entry[], int *pos, int *y, int *maxlen, int SIZE, int len)
{
    if (*pos > 0) {
        --(*pos);
        *y = *pos - s->pos_upper_t + w->y_beg;

        if (*pos >= s->pos_upper_t && *pos < s->pos_lower_t) { // NO SCROLL

            print_no_scroll(w, entry, y, pos, maxlen, 1);

        } else if (*pos <= s->pos_upper_t && s->pos_upper_t > 0) {

            print_scroll(w, s, entry, SIZE, pos, maxlen, 1);

        }
        len = strlen(entry[*pos]);
        highlight(entry, maxlen, pos, &len);
    }
}

void move_dn(Window *w, Scroll *s, char *entry[], int *pos, int *y, int *maxlen, int SIZE, int len, int *sz)
{
    if (*pos < *sz - 1) {
        ++(*pos);
        *y = *pos - s->pos_upper_t + w->y_beg;

        if (*pos > s->pos_upper_t && *pos <= s->pos_lower_t) { // NO SCROLL

            print_no_scroll(w, entry, y, pos, maxlen, 0);

        } else if (*pos - 1 >= s->pos_lower_t) {

            print_scroll(w, s, entry, SIZE, pos, maxlen, 0);

        }
        len = strlen(entry[*pos]);
        highlight(entry, maxlen, pos, &len);
    }
}
