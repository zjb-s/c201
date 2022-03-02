#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#define MAX_X 1

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
int cursor_playlist_position = 0;

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
void cursor_init(Cursor * c) {
    c->pos_in_sequence = 0;
    c-> pos_in_playlist = 0;
    c->pos_in_phrase = 0;
}
//playlist constructor
void playlist_init(Playlist * p) {
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
Cursor cursor;
Playlist playlist;
Phrase phrases[128];
Phrase * cursor_phrase;
Phrase * playhead_phrase;
Step * cursor_step;
Step * playhead_step;
Step * active_step;
Step clipboard;

void init_phrase_library() {
    for (int i=0;i<127;i++) {
        phrase_init(& phrases[i]);
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

Step * get_step_in_phrase(int step_number, int phrase) {
    return & phrases[playlist.list[phrase]].steps[step_number];
}

void set_step(int n, Step * s) {
    *get_step(n) = *s;
}

Phrase * get_phrase_from_step_index (int n) {
    Phrase * rval = & phrases[playlist.list[0]];
    int step_tally = 0;
    for (int i=0;i<playlist.len;i++) { 
        step_tally += phrases[playlist.list[i]].len; 
        if (n <= step_tally) {
            rval = & phrases[playlist.list[i]];
        }
    }
    return rval;
}

// return the phrase containing this step
int which_phrase (int n) {
    int rval = playlist.list[0];
    int step_tally = 0;
    for (int i=0;i<playlist.len;i++) { 
        step_tally += phrases[playlist.list[i]].len; 
        if (n <= step_tally) {
            rval = playlist.list[i];
        }
    }
    return rval;
}

// same as which_phrase, but return the playlist index
int get_playlist_position (int n) {
    int step_tally = 0;
    for (int i=0;i<playlist.len;i++) { 
        step_tally += phrases[playlist.list[i]].len; 
        if (n <= step_tally) {
            return i;
        }
    }
    return 0;
}

void move_cursor(int delta) {
    if (delta > 0) { // move down
        if (cursor.pos_in_sequence >= get_seqlen()-1) {
            cursor.pos_in_sequence = get_seqlen()-1;
            cursor.pos_in_playlist = playlist.len-1;
            return; // set position to end
        }
        cursor.pos_in_sequence++;

        if (cursor.pos_in_phrase + delta < cursor.phrase_pointer->len) {
            cursor.pos_in_phrase++;
        } else {
            cursor.pos_in_playlist++;
            cursor.pos_in_phrase = 0;
            cursor.phrase_pointer = & phrases[playlist.list[cursor.pos_in_playlist]];
            cursor.step_pointer = & cursor.phrase_pointer->steps[0];
        }

    } else { // move up
        if (cursor.pos_in_sequence <= 0) {
            cursor.pos_in_sequence = 0;
            cursor.pos_in_playlist = 0;
            return; // set position to start
        }
        cursor.pos_in_sequence--;

        if (cursor.pos_in_phrase + delta > 0) {
            cursor.pos_in_phrase--;
        } else {
            cursor.pos_in_playlist--;
            cursor.phrase_pointer = & phrases[playlist.list[cursor.pos_in_playlist]];
            cursor.pos_in_phrase = cursor.phrase_pointer->len;
            cursor.step_pointer = & cursor.phrase_pointer->steps[cursor.pos_in_phrase];
        }
    }
}