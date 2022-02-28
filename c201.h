#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <Python/Python.h>
#define MAX_X 0

// global variables
bool dirty = true;
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


// step class
typedef struct {
    int id;
    int cva;
    int cvb;
    int dur;
    int gate;
    int prob;
    int every;
} Step;

// step constructor                                 ---
void step_init(Step * s) {
    step_id_counter++;
    s->id = step_id_counter;
    s->cva = 12;
    s->cvb = 12;
    s->dur = 1;
    s->gate = 1;
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
void phrase_init(Phrase * p) {
    p->id = phrase_id_counter++;
    p->len = 1;
    p->pos = 0;
    step_init(&p->steps[0]);
}

// playlist class
typedef struct {
    int list[128];
    int start;
    int end;
} Playlist;

//playlist constructor
void playlist_init(Playlist * p) {
    for(int i=0;i<128;i++) { p->list[i] = 0; }
    p->start = 0;
    p->end = 0;
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
Playlist playlist;
Phrase phrases[128];
Phrase * cursor_phrase;
Phrase * playhead_phrase;
Step * cursor_step;
Step * playhead_step;
Step * active_step;
Step clipboard;

int get_seqlen() {
    int step_tally = 0;
    for (int i=0;i<4;i++){
        step_tally += phrases[playlist.list[i]].len;
    }
    seqlen = step_tally;
    return seqlen;
}

Step * get_step(int n) {
    Step * rs;
    int step_tally = 0;
    for (int i=0;i<4;i++) { 
        step_tally += phrases[playlist.list[i]].len; 
        if (n <= step_tally) {
            rs = & phrases[ playlist.list[ i ] ].steps[ ( step_tally - phrases[ playlist.list[ i ] ].len ) + n];
        }
    }
    return rs;
}

Step * get_step_in_phrase(int step_number, int phrase) {
    return & phrases[playlist.list[phrase]].steps[step_number];
}

void set_step(int n, Step * s) {
    int step_tally = 0;
    // get total steps in all phrases in the playlist
    for (int i=0;i<4;i++) { 
        Phrase * p = & phrases[playlist.list[i]];
        if (n < p->len) {
            phrases[playlist.list[i - step_tally]].steps[n] = *s;
        }
        step_tally += p->len; 
    }
}

void set_step_in_phrase(int step_number, int phrase, Step * step_pointer) {
    Step * s = get_step_in_phrase(step_number, phrase);
    s = step_pointer;
}

Phrase * get_phrase_from_step_index (int n) {
    Phrase * rval = & phrases[playlist.list[0]];
    int step_tally = 0;
    for (int i=0;i<4;i++) { 
        step_tally += phrases[playlist.list[i]].len; 
        if (n <= step_tally) {
            rval = & phrases[playlist.list[i]];
        }
    }
    return rval;
}

int which_phrase (int n) {
    int rval = playlist.list[0];
    int step_tally = 0;
    for (int i=0;i<4;i++) { 
        step_tally += phrases[playlist.list[i]].len; 
        if (n <= step_tally) {
            rval = playlist.list[i];
        }
    }
    return rval;
}