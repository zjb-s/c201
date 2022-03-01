#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#include "c201.h"

void draw_table() {
    int px = 15; int py = 8; int next_step_to_print = 0;
    move(py,px);
    for(int i=0; i<playlist.len;i++) { // for each phrase in the playlist
        Phrase * this_phrase = &phrases[playlist.list[i]];
        mvprintw(8+i, 1, "%d", playlist.list[i]);

        // draw phrase divider
        move(py,px-6);
        for (int j = 1; j < COLS-1-px; j++) {
            printw("'");
        }
        attron(A_REVERSE);
        mvprintw(py,px-6,"      ");
        mvprintw(py,px-6,"|%d.", playlist.list[i]);
        attroff(A_REVERSE);
        py++;

        //draw table content
        for (int k = 0; k < this_phrase->len; k++) {
            if (pos == next_step_to_print) {
                attron(A_REVERSE);
                mvprintw(py,px+45," %d ", count);
                attroff(A_REVERSE);
            }
            if (y == next_step_to_print) {
                mvprintw(py, px, "-----------------------------------");
            }
            Step * this_step = get_step(next_step_to_print);
            mvprintw(py, px-6,      "|%d.", k);
            mvprintw(py, px+0,      "%d", this_step->cva);
            mvprintw(py, px+7,      "%d", this_step->cvb);
            mvprintw(py, px+14,     "%d", this_step->dur);
            mvprintw(py, px+21,     "%d", this_step->gate);
            mvprintw(py, px+28,     "%d", this_step->prob);
            mvprintw(py, px+35,     "%d", this_step->every);
            mvprintw(py, px+42,     "%d", this_step->id);
            py++; next_step_to_print++;
        }
    }
}

void draw_context() {
    int bottom = getmaxy(stdscr)-2;
    mvprintw(5, 15, "cv.a   cv.b   dur    gate   prob   ev    id");
    mvprintw(bottom, 2, "phrase view"); //todo implement movement view & arrangement view
    mvprintw(bottom, 30, "clipboard step: [%d, %d, %d, %d]", clipboard.cva, clipboard.cvb, clipboard.dur, clipboard.gate);
    mvprintw(bottom -1, 2, "seqlen: %d", get_seqlen());
}


void redraw() {
    clear();
    attroff(A_REVERSE);
    mvprintw(1,1,"y: %d, pos: %d, phrase 0 len: %d", y, pos, phrases[playlist.list[0]].len);
    box(stdscr, ACS_VLINE, ACS_HLINE);
    draw_context();
    draw_table();
    refresh();
    dirty = false;
}

void add_step() {
    Phrase * p = get_phrase_from_step_index(y);
    int pn = which_phrase(y);
    p->len = clamp(p->len+1, 0, 128);
    for (int i = p->len-2; i >= y; i--) {
        *get_step_in_phrase(i+1, pn) = *get_step_in_phrase(i, pn);
    }
    step_init(& p->steps[y]);
}

void remove_step() {
    Phrase * p = get_phrase_from_step_index(y);
    int pn = which_phrase(y);
    p->len = clamp(p->len-1, 0, 128);
    for (int i = y; i < p->len; i++) {
        *get_step_in_phrase(i, pn) = *get_step_in_phrase(i+1, pn);
    }
    y = clamp(y-1, 0, get_seqlen());
}

void * keyboard_input(void * arg) {
    while (ch != '0') {
        cursor_phrase = get_phrase_from_step_index(y);
        cursor_step = get_step(y);
        cursor_playlist_position = get_playlist_position(y);

        switch (ch) {
            case KEY_LEFT:
                x = clamp(x-1, 0, MAX_X);
                break;
            case KEY_DOWN:
                y = clamp(y+1, 0, get_seqlen()-1);
                break;
            case KEY_UP:
                y = clamp(y-1, 0, get_seqlen()-1);
                break;
            case KEY_RIGHT:
                x = clamp(x+1, 0, MAX_X);
                break;
            case '=':
                add_step();
                break;
            case '-':
                remove_step();
                break;
            case ']':
                playlist.list[cursor_playlist_position] = clamp(playlist.list[cursor_playlist_position]+1, 0, 128);
                break;
            case '[':
                playlist.list[cursor_playlist_position] = clamp(playlist.list[cursor_playlist_position]-1, 0, 128);
                break;
            case 'x':
                clipboard = *cursor_step;
                remove_step();
                break;
            case 'c':
                clipboard = *cursor_step;
                break;
            case 'v':
                add_step();
                set_step(y+1, &clipboard);
                break;
            case 'q':
                cursor_step->cva = clamp(cursor_step->cva+1, 0, 128);
                break;
            case 'w':
                cursor_step->cvb = clamp(cursor_step->cvb+1, 0, 128);
                break;
            case 'e':
                cursor_step->dur = clamp(cursor_step->dur+1, 0, 128);
                break;
            case 'a':
                cursor_step->cva = clamp(cursor_step->cva-1, 0, 128);
                break;
            case 's':
                cursor_step->cvb = clamp(cursor_step->cvb-1, 0, 128);
                break;
            case 'd':
                cursor_step->dur = clamp(cursor_step->dur-1, 0, 128);
                break;
            case 'Q':
                cursor_step->gate = clamp(cursor_step->gate+1, 0, 128);
                break;
            case 'W':
                cursor_step->prob = clamp(cursor_step->prob+1, 0, 100);
                break;
            case 'E':
                cursor_step->every = clamp(cursor_step->every+1, 1, 128);
                break;
            case 'A':
                cursor_step->gate = clamp(cursor_step->gate-1, 0, 128);
                break;
            case 'S':
                cursor_step->prob = clamp(cursor_step->prob-1, 0, 100);
                break;
            case 'D':
                cursor_step->every = clamp(cursor_step->every-1, 1, 128);
                break;
        }

        ch = getch();
        dirty = true;
    }
    return 0;
}

void note() {
    char * command;
    asprintf(&command, "python3 pmod.py note %d %d", get_step(pos)->cva, get_step(pos)->cvb);
    system(command);
    free(command);
    // todo implement midi out
    // todo implement gates
    // todo implement repeats
}

void advance() {
    int old_pos = pos;
    bool any_steps_on;
    for (int i=0;i<get_seqlen();i++) { if (get_step(i)->dur) { any_steps_on = true; } }
    if (any_steps_on) {
        if (count < get_step(pos)->dur - 1) { 
            count++;
        } else {
            // advance!
            count = 0;
            for (int i=1;i<get_seqlen();i++) {
                if (get_step((old_pos+i) % get_seqlen())->dur) {
                    pos = (old_pos+i) % get_seqlen();
                    //note();
                    break;
                }
            }
        }
    }
}

void clock_step() {
    dirty = true;
    advance();
}

void * fast_tick(void * arg) {
    while (ch != '0') {
        usleep(1000); t++;
        if (t % 128 == 0) {
            clock_step();
        }
        if (dirty) { redraw(); }
    }
    return 0;
}

int main() {
    init_curses();
    playlist_init(&playlist);
    playlist.list[1] = 1;
    phrase_init(&phrases[0]);
    phrase_init(&phrases[1]);
    clipboard = phrases[playlist.list[0]].steps[0];
    cursor_step = get_step(0);

    pthread_create(&tid[0], NULL, fast_tick, (void *) &tid[0]);
    pthread_create(&tid[1], NULL, keyboard_input, (void *) &tid[1]);
    pthread_join(tid[1], NULL);
    endwin();
    return 0;
}
