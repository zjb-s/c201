#include <ncurses.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <monome.h> 

#include "c201.h"

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

void deselect_all() {
    for (int i=0;i<get_seqlen();i++) { 
        Step * s = get_step(i);
        s->selected = false;
    }
}

void cursor_move(int delta) {
    cursor.pos_in_sequence = clamp (cursor.pos_in_sequence + delta, 0, get_seqlen()-1);

    // handle cursor ints
    int tally = 0;
    for (int i = 0; i < playlist.len; i++) { 
        tally += phrases[playlist.list[i]].len;
        if (cursor.pos_in_sequence < tally) {
            cursor.pos_in_playlist =    i;
            cursor.pos_in_phrase =      (cursor.pos_in_sequence - tally) + phrases[playlist.list[i]].len;
            break;
        }
    }
    // handle cursor pointers
    cursor.phrase_pointer = &   phrases[playlist.list[cursor.pos_in_playlist]];
    cursor.step_pointer = &     cursor.phrase_pointer->steps[cursor.pos_in_phrase];

    // handle step selections
    deselect_all();
    cursor.step_pointer->selected = true;
    if (visual) {
        int a = cursor.pos_in_sequence > select_origin ? select_origin : cursor.pos_in_sequence;
        int b = cursor.pos_in_sequence < select_origin ? select_origin : cursor.pos_in_sequence;
        for (int i = a; i <= b; i++) {
            Step * s = get_step(i);
            s->selected = true;
        }
    }
}

void cursor_jump(int delta) {
    if (delta > 0) {
        if (cursor.pos_in_phrase >= cursor.phrase_pointer->len-1) {
            cursor_move(1);
        } else {
            while (cursor.pos_in_phrase < cursor.phrase_pointer->len-1) {
                cursor_move(1);
            }
        }
    } else if (delta < 0) {
        if (cursor.pos_in_phrase <= 0) {
            cursor_move(-1);
        } else {
            while (cursor.pos_in_phrase > 0) {
                cursor_move(-1);
            }
        }
    }
}

void change_phrase(int delta) {
    int ph = cursor.pos_in_playlist;
    cursor.pos_in_phrase = 0;
    cursor.pos_in_sequence = 0;
    cursor.pos_in_playlist = 0;
    playlist.list[ph] = clamp(playlist.list[ph] + delta, 0, 127);
    while (cursor.pos_in_playlist < ph) {
        cursor_move(1);
    }
    arc_dirty = true;
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
    arc_dirty = true; screen_dirty = true;
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

void draw_table() {
    int px = 15; int py = 8; int next_step_to_print = 0;
    move(py,px);
    for(int i=0; i<playlist.len;i++) { // for each phrase in the playlist
        Phrase * this_phrase = &phrases[playlist.list[i]];
        if (cursor.pos_in_playlist == i) { attron(A_REVERSE); }
        mvprintw(8+i, 1, "%d", playlist.list[i]);
        attroff(A_REVERSE);

        // draw phrase divider
            move(py,px-6);
        for (int j = 1; j < COLS-1-px; j++) {
            printw("'");
        }
        attron(A_REVERSE);
        mvprintw(py,px-6,"|%d.", playlist.list[i]);
        attroff(A_REVERSE);
        py++;

        //draw table content
        for (int k = 0; k < this_phrase->len; k++) {
			Step * this_step = get_step(next_step_to_print);
            if (pos == next_step_to_print) { // count indicator / playhead
                attron(A_REVERSE);
				mvprintw(py,px+45," %d ", count);
                attroff(A_REVERSE);
            }
            if (cursor.pos_in_sequence == next_step_to_print) {
                mvprintw(py, px, "-----------------------------------");
            } 
            mvprintw(py, px-6,      "|%d.", k);
            if (this_step->selected && visual) { attron(A_REVERSE); }
            mvprintw(py, px+0,      "%d", this_step->cva);
            mvprintw(py, px+7,      "%d", this_step->cvb);
            mvprintw(py, px+14,     "%d", this_step->dur);
            if (this_step->on) { mvprintw(py, px+21,     "ON"); }
            mvprintw(py, px+28,     "%d", this_step->prob);
            mvprintw(py, px+35,     "%d", this_step->every);
            mvprintw(py, px+42,     "%d", this_step->id);
            attroff(A_REVERSE);
			py++; next_step_to_print++;
        }
    }
}

void draw_context() {
    int bottom = getmaxy(stdscr)-2;
    mvprintw(5, 15, "cv.a   cv.b   dur    gate   prob   ev    id");
    mvprintw(bottom, 2, "phrase view"); //todo implement movement view & arrangement view
    mvprintw(bottom, 30, "clipboard step: [%d, %d, %d, %d]", clipboard.cva, clipboard.cvb, clipboard.dur, clipboard.on);
    mvprintw(bottom -1, 2, "seqlen: %d", get_seqlen());

    if (debug) {
        attron(A_REVERSE);
        mvprintw(1,1,"DEBUG MODE");
        mvprintw(5,80,"DEBUG PANEL");
        attroff(A_REVERSE);
        mvprintw(6,80,"pos_in_sequence: %d", cursor.pos_in_sequence);
        mvprintw(7,80,"pos_in_playlist: %d", cursor.pos_in_playlist);
        mvprintw(8,80,"pos_in_phrase: %d", cursor.pos_in_phrase);
        mvprintw(9,80,"cursor step cva: %d", cursor.step_pointer->cva);
        mvprintw(10,80,"cursor step id: %d", cursor.step_pointer->id);
        mvprintw(11,80,"visual? %d", visual);
    }
}

void arc_redraw() {
    int i;
    for(i=0;i<4;i++) { monome_led_ring_all(arc, i, 0); } // all lights off
    monome_led_ring_range(arc, 0, 0, (cursor.step_pointer->cva / 2)-1, 15);
    monome_led_ring_range(arc, 1, 0, (cursor.step_pointer->cvb / 2)-1, 15);
    monome_led_ring_range(arc, 2, 0, (cursor.step_pointer->dur / 2)-1, 15);
    arc_dirty= false;
	// split ring 3 into equal parts...
	for (int i=0; i<cursor.phrase_pointer->len;i++) {
		int led = (64 / cursor.phrase_pointer->len) * i;
		monome_led_ring_set(arc, 3, led, (i == cursor.pos_in_phrase ? 15 : 8));
		monome_led_ring_set(arc, 3, led-1, (i == cursor.pos_in_phrase ? 8 : 4));
		monome_led_ring_set(arc, 3, led+1, (i == cursor.pos_in_phrase ? 8 : 4));
	}	
}

void redraw() {
	if (screen_dirty) { erase(); } 
    attroff(A_REVERSE);
    //mvprintw(1,1,"cursor: %d, pos: %d, phrase 0 len: %d", cursor.pos_in_sequence, pos, phrases[playlist.list[0]].len);
	draw_table();
    draw_context();
    if (arc_dirty && arc_connected) {
        arc_redraw();
    }
    refresh();
    screen_dirty = false;
}

void add_step() {
    Phrase * ph = cursor.phrase_pointer;
    ph->len = clamp(ph->len+1, 0, 127);
    for (int i = ph->len-1; i >= cursor.pos_in_phrase; i--) {
        ph->steps[i + 1] = ph->steps[i];
    }
    cursor_move(1);
    step_id_counter++;
    cursor.step_pointer->id = step_id_counter;
    //init_step(cursor.step_pointer);
    arc_dirty = true;
}

void remove_step() {
    if (cursor.phrase_pointer->len != 1) { // don't remove the last step in phrase
        Phrase * ph = cursor.phrase_pointer;
        for (int i = cursor.pos_in_phrase; i < ph->len; i++) {
            ph->steps[i] = ph->steps[i+1];
        }
        ph->len = clamp(ph->len-1, 0, 127);
        cursor_move(-1);
    }
    arc_dirty = true;
}

void toggle_visual_mode() {
    visual = ! visual;
    if (visual ) {
        select_origin = cursor.pos_in_sequence;
    } else {
        deselect_all();
    }
    cursor.step_pointer->selected = true;
}

void change_step_attribute(int attr, int d) {
    Step * s;
    int steps_already_checked[128] = {0};
    int step_check_position = 0;
    for (int i=0;i<get_seqlen();i++) { 
        s = get_step(i);
        bool skip = false;
        for (int i=0;i<128;i++) { // if we've already modified this step, skip it! This prevents phrase multiples from messing up this function
            if (steps_already_checked[i] == s->id) {
                skip = true;
                break;
            }
        }
        if (s->selected && ! skip) {
            switch (attr) {
                case 0:
                    s->cva = clamp(s->cva+d,0,127);
                    break;
                case 1:
                    s->cvb = clamp(s->cvb+d,0,127);
                    break;
                case 2:
                    s->dur = clamp(s->dur+d,0,127);
                    break;
                case 3:
                    s->on = ! s->on;
                    break;
                case 4:
                    s->prob = clamp(s->prob+d,0,100);
                    break;
                case 5:
                    s->every = clamp(s->every+d,0,127);
                    break;
            }
            steps_already_checked[step_check_position] = s->id;
            step_check_position++;
        }
    }
}

void num_key(char c) {
    int k = c - '0';
    Step * s;
    if (k == 0) {
        s = cursor.step_pointer;
    } else {
        s = & cursor.phrase_pointer->steps[k-1];
    }
    s->on = ! s->on;
}

void * keyboard_input() {
    while (ch != EXIT_KEY) {
        if (is_member_of(ch, numbers, 10)) {
            num_key(ch);
        }
        switch (ch) {
			case '~':
				debug = ! debug;
				break;
            case KEY_LEFT:
                cursor_jump(-1);
                break;
            case KEY_DOWN: 
                cursor_move(1);
                break;
            case KEY_UP:
                cursor_move(-1);
                break;
			case KEY_RIGHT:
                cursor_jump(1);
 				break;
            case 'k':
                cursor_move(-1);
                break;
            case 'j':
                cursor_move(1);
                break;
            case ' ':
                toggle_visual_mode();
                break;
            case '=':
                add_step();
                break;
            case '-':
                remove_step();
                break;
            case ']':
                change_phrase(1);
                break;
            case '[':
                change_phrase(-1);
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
                change_step_attribute(0, 1);
                break;
            case 'w':
                change_step_attribute(1, 1);
                break;
            case 'e':
                change_step_attribute(2, 1);
                break;
            case 'a':
                change_step_attribute(0, -1);
                break;
            case 's':
                change_step_attribute(1, -1);
                break;
            case 'd':
                change_step_attribute(2, -1);
                break;
            case 'Q':
                change_step_attribute(3, 1);
                break;
            case 'W':
                change_step_attribute(4, 1);
                break;
            case 'E':
                change_step_attribute(5, 1);
                break;
            case 'A':
                change_step_attribute(3, -1);
                break;
            case 'S':
                change_step_attribute(4, -1);
                break;
            case 'D':
                change_step_attribute(5, -1);
                break;
        }

        ch = getch();
        screen_dirty = true;
    }
    return 0;
}

void note() {
    char * command;
    asprintf(&command, "echo 'ii.jf.play_note(%d, %d)' | websocat ws://localhost:6666", get_step(pos)->cva, get_step(pos)->cvb);
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
                    note();
                    break;
                }
            }
        }
    }
}

void clock_step() {
    screen_dirty = true;
    advance();
}

void * fast_tick() {
    while (ch != EXIT_KEY) {
        usleep(1000); t++;
        if (t % 128 == 0) {
            clock_step();
        }
        if (screen_dirty) { redraw(); }
	if(arc_dirty && arc_connected) { arc_redraw(); }
    }
    return 0;
}

int main() {
    open_arc();
    init_curses();
    init_playlist(&playlist);
    for (int i=0;i<128;i++) { init_phrase(&phrases[i]); }
    init_cursor(& cursor);
    cursor.step_pointer = & phrases[0].steps[0];
    cursor.phrase_pointer = & phrases[0];
    clipboard = phrases[playlist.list[0]].steps[0];
    visual = false;

    pthread_create(&tid[0], NULL, fast_tick, (void *) &tid[0]);
    pthread_create(&tid[1], NULL, keyboard_input, (void *) &tid[1]);

    if (arc_connected) { monome_event_loop(arc); }

    pthread_join(tid[1], NULL);
    if (arc_connected) { monome_close(arc); }
    endwin();
    return 0;
}