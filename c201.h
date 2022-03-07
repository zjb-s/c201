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

int get_seqlen() {
    int step_tally = 0;
    for (int i=0;i<playlist.len;i++){
        step_tally += phrases[playlist.list[i]].len;
    }
    seqlen = step_tally;
    return seqlen;
}

Step * get_step(int n) {
    int pidx = 0;

    while(n >= 0 && n >= phrases[playlist.list[pidx]].len)
        n -= phrases[playlist.list[ pidx++ ]].len;

    return &phrases[playlist.list[pidx]].steps[n];
}

void set_step(int n, Step * s) {
    *get_step(n) = *s;
}

void cursor_resolve() { // clamp values, set up pointers, etc
    cursor.pos_in_phrase =      clamp(cursor.pos_in_phrase, 0, cursor.phrase_pointer->len-1);
    cursor.pos_in_playlist =    clamp(cursor.pos_in_playlist, 0, playlist.len-1);
    cursor.pos_in_sequence =    clamp(cursor.pos_in_sequence, 0, get_seqlen()-1);

    cursor.phrase_pointer = &   phrases[playlist.list[cursor.pos_in_playlist]];
    cursor.step_pointer = &     cursor.phrase_pointer->steps[cursor.pos_in_phrase];
}

void cursor_move(int delta) {
    if (delta > 0 && cursor.pos_in_sequence < get_seqlen()-1) {
        cursor.pos_in_sequence++;
        cursor.pos_in_phrase++;
        if (cursor.pos_in_phrase >= cursor.phrase_pointer->len) { // is there room to advance within current phrase?
            cursor.pos_in_playlist++;
            cursor.pos_in_phrase = 0;
        }
    } else if (delta < 0 && cursor.pos_in_sequence >= 0) {
        cursor.pos_in_sequence--;
        if (cursor.pos_in_phrase > 0) {
            cursor.pos_in_phrase--;
        } else {
            cursor.pos_in_playlist--;
            if (cursor.pos_in_playlist >= 0) {
                cursor.pos_in_phrase = phrases[playlist.list[cursor.pos_in_playlist]].len-1;
            }
        }
    }
    cursor_resolve();
}

void change_phrase(int delta) {
    int ph = cursor.pos_in_playlist;
    cursor.pos_in_phrase = 0;
    cursor.pos_in_sequence = 0;
    cursor.pos_in_playlist = 0;
    cursor_resolve();
    playlist.list[ph] = clamp(playlist.list[ph] + delta, 0, 127);
    while (cursor.pos_in_playlist < ph) {
        cursor_move(1);
    }
}

void arc_delta(const monome_event_t *e) {
    int d = e->encoder.delta; 
    int n = e->encoder.number;
    int change_to_send = 0;
    delta_counters[n] += d;
    if (delta_counters[n] > ARC_SENSITIVITY) {
        delta_counters[n] = 0;
        change_to_send = 1;
    } else if (delta_counters[n] < 0) {
        delta_counters[n] = ARC_SENSITIVITY;
        change_to_send = -1;
    }
    switch (n) { // which ring?
        case 0:
            cursor.step_pointer->cva = clamp(cursor.step_pointer->cva + (change_to_send / 1), 0, 127); 
            break;
        case 1:
            cursor.step_pointer->cvb = clamp(cursor.step_pointer->cvb + (change_to_send / 1), 0, 127); 
            break;
        case 2:
            cursor.step_pointer->dur = clamp(cursor.step_pointer->dur + (change_to_send / 1), 0, 127); 
            break;
        case 3:
            cursor_move(change_to_send);
            break;
    }
    screen.arc = true;
	cursor.step_pointer->dirty = true;
}

void open_arc() {
    if ((arc = monome_open(ARC_PATH_1))) {
        monome_register_handler(arc, MONOME_ENCODER_DELTA, arc_delta, NULL);
        arc_connected = true;
    } else if ((arc = monome_open(ARC_PATH_2))) { 
        monome_register_handler(arc, MONOME_ENCODER_DELTA, arc_delta, NULL);
        arc_connected = true;
    } else {
        arc_connected = false;
    }
}