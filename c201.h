#pragma once

#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <monome.h>

#define MAX_X 1
#define ARC_PATH_1 "/dev/ttyUSB0"
#define ARC_PATH_2 "/dev/tty.usbserial-m1100276"
#define KNOBS 4
#define ARC_SENSITIVITY 24

// global variables
int every_counter = 0;
int step_id_counter = 1;
int phrase_id_counter = 0;
int ch = ' ';
int y = 0;
int x = 0;
int t = 0;
int seqlen = 0;
int count = 0;
int pos = 0;
int delta_counters[] = {0,0,0,0};
bool debug = true;
bool arc_connected = false;

// print checkers
typedef struct {
	bool arc;
	bool screen;
	bool playlist;
	bool count;
	bool table;
	bool cursor;
} Screen;

void init_screen(Screen * s) {
	s->arc = true;
	s->screen = true;
	s->playlist = true;
	s->count = true;
	s->table = true;
	s->cursor = true;
}

// step class
typedef struct {
	bool dirty;
    int id;
    int cva;
    int cvb;
    int dur;
    bool on;
    int prob;
    int every;
} Step;

// step constructor                                 ---
void init_step(Step * s) {
	s->dirty = true;
    step_id_counter++;
    s->id = step_id_counter;
    s->cva = 12;
    s->cvb = 12;
    s->dur = 1;
    s->on = true;
    s->prob = 100;
    s->every = 1;
}

// phrase class
typedef struct {
    int id;
    int len;
    int pos;
    Step steps[128];
} Phrase;

// phrase constructor
void init_phrase(Phrase * p) {
    p->id = phrase_id_counter++;
    p->len = 1;
    p->pos = 0;
    init_step(&p->steps[0]);
}

// playlist class
typedef struct {
    int list[128];
    int start;
    int end;
    int len;
} Playlist;

// cursor class
typedef struct {
    int pos_in_sequence;
    int pos_in_playlist;
    int pos_in_phrase;
    Step * step_pointer;
    Phrase * phrase_pointer;
} Cursor;

// cursor constructor
void init_cursor(Cursor * c) {
    c->pos_in_sequence = 0;
    c-> pos_in_playlist = 0;
    c->pos_in_phrase = 0;
}
//playlist constructor
void init_playlist(Playlist * p) {
    for(int i=0;i<128;i++) { p->list[i] = 0; }
    p->start = 0;
    p->end = 0;
    p->len = 4;
}

// utilities                                        ---
int clamp(int n, int min, int max) {
    if(n < min)
        return min;
    else if(n > max)
        return max;
    return n;
}

// initialize curses options                        ---
void init_curses() {
    initscr();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    noecho();
}

// more global variables
pthread_t tid[16];
Screen screen;
Cursor cursor;
Playlist playlist;
Phrase phrases[128];
Step * active_step;
Step clipboard;
monome_t * arc;

void init_phrase_library() {
    for (int i=0;i<127;i++) {
        init_phrase(& phrases[i]);
    }
}