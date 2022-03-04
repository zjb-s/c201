#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <monome.h> 

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
            if (cursor.pos_in_sequence == next_step_to_print) {
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

    if (DEBUG) {
        attron(A_REVERSE);
        mvprintw(1,1,"DEBUG MODE");
        mvprintw(5,80,"DEBUG PANEL");
        attroff(A_REVERSE);
        mvprintw(6,80,"pos_in_sequence: %d", cursor.pos_in_sequence);
        mvprintw(7,80,"pos_in_playlist: %d", cursor.pos_in_playlist);
        mvprintw(8,80,"pos_in_phrase: %d", cursor.pos_in_phrase);
        mvprintw(9,80,"cursor step cva: %d", cursor.step_pointer->cva);
        mvprintw(10,80,"cursor step id: %d", cursor.step_pointer->id);
        mvprintw(11,80,"delta counter: %d", delta_counter);
    }
}

void arc_redraw() {
    monome_led_ring_all(arc, 2, 15);
}

void redraw() {
    clear();
    attroff(A_REVERSE);
    //mvprintw(1,1,"cursor: %d, pos: %d, phrase 0 len: %d", cursor.pos_in_sequence, pos, phrases[playlist.list[0]].len);
    box(stdscr, ACS_VLINE, ACS_HLINE);
    draw_table();
    draw_context();
    arc_redraw();
    refresh();
    dirty = false;
}

void add_step() {
    cursor.phrase_pointer->len = clamp(cursor.phrase_pointer->len+1, 0, 128);
    for (int i = cursor.phrase_pointer->len-2; i >= cursor.pos_in_phrase; i--) {
        cursor.phrase_pointer->steps[cursor.pos_in_phrase + 1] = * cursor.step_pointer;
    }
    step_init(cursor.step_pointer);
}

void remove_step() {
    cursor.phrase_pointer->len = clamp(cursor.phrase_pointer->len-1, 0, 128);
    cursor_move(-1);
    for (int i = cursor.pos_in_sequence; i < cursor.phrase_pointer->len; i++) {
        *get_step_in_phrase(i, cursor.pos_in_playlist) = *get_step_in_phrase(i+1, cursor.pos_in_playlist);
    }
}

void * keyboard_input(void * arg) {
    while (ch != '0') {
        switch (ch) {
            case KEY_LEFT:
                break;
            case KEY_DOWN:
                // y = clamp(y+1, 0, get_seqlen()-1);
                cursor_move(1);
                break;
            case KEY_UP:
                cursor_move(-1);
                break;
            case KEY_RIGHT:
                //x = clamp(x+1, 0, MAX_X);
                break;
            case '=':
                add_step();
                break;
            case '-':
                remove_step();
                break;
            case ']':
                playlist.list[cursor.pos_in_playlist] = clamp(playlist.list[cursor.pos_in_playlist]+1, 0, 128);
                cursor.phrase_pointer = & phrases[cursor.pos_in_playlist];
                break;
            case '[':
                playlist.list[cursor.pos_in_playlist] = clamp(playlist.list[cursor.pos_in_playlist]-1, 0, 128);
                cursor.phrase_pointer = & phrases[cursor.pos_in_playlist];
                break;
            case 'x':
                clipboard = * cursor.step_pointer;
                remove_step();
                break;
            case 'c':
                clipboard = * cursor.step_pointer;
                break;
            case 'v':
                add_step();
                set_step(y+1, &clipboard);
                break;
            case 'q':
                cursor.step_pointer->cva = clamp(cursor.step_pointer->cva+1, 0, 128);
                break;
            case 'w':
                cursor.step_pointer->cvb = clamp(cursor.step_pointer->cvb+1, 0, 128);
                break;
            case 'e':
                cursor.step_pointer->dur = clamp(cursor.step_pointer->dur+1, 0, 128);
                break;
            case 'a':
                cursor.step_pointer->cva = clamp(cursor.step_pointer->cva-1, 0, 128);
                break;
            case 's':
                cursor.step_pointer->cvb = clamp(cursor.step_pointer->cvb-1, 0, 128);
                break;
            case 'd':
                cursor.step_pointer->dur = clamp(cursor.step_pointer->dur-1, 0, 128);
                break;
            case 'Q':
                cursor.step_pointer->gate = clamp(cursor.step_pointer->gate+1, 0, 128);
                break;
            case 'W':
                cursor.step_pointer->prob = clamp(cursor.step_pointer->prob+1, 0, 100);
                break;
            case 'E':
                cursor.step_pointer->every = clamp(cursor.step_pointer->every+1, 1, 128);
                break;
            case 'A':
                cursor.step_pointer->gate = clamp(cursor.step_pointer->gate-1, 0, 128);
                break;
            case 'S':
                cursor.step_pointer->prob = clamp(cursor.step_pointer->prob-1, 0, 100);
                break;
            case 'D':
                cursor.step_pointer->every = clamp(cursor.step_pointer->every-1, 1, 128);
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

int main(int argc, char **argv) {
    arc = monome_open(DEFAULT_MONOME_DEVICE);
    void * bb = NULL;
    monome_register_handler(arc, MONOME_ENCODER_DELTA, delta, bb);
    init_curses();
    playlist_init(&playlist);
    phrase_init(&phrases[0]);
    cursor_init(& cursor);
    cursor.step_pointer = & phrases[0].steps[0];
    cursor.phrase_pointer = & phrases[0];
    clipboard = phrases[playlist.list[0]].steps[0];

    pthread_create(&tid[0], NULL, fast_tick, (void *) &tid[0]);
    pthread_create(&tid[1], NULL, keyboard_input, (void *) &tid[1]);
    pthread_join(tid[1], NULL);
    endwin();
    return 0;
}
