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
		if (screen.count || screen.screen) {
			mvprintw(8+i, 1, "   ");
			mvprintw(8+i, 1, "%d", playlist.list[i]);
			screen.count = false;
		} 

        // draw phrase divider
		if (screen.screen) {
			 move(py,px-6);
        	for (int j = 1; j < COLS-1-px; j++) {
        	    printw("'");
        	}
        	attron(A_REVERSE);
        	mvprintw(py,px-6,"      ");
        	mvprintw(py,px-6,"|%d.", playlist.list[i]);
        	attroff(A_REVERSE);
		}
        py++;

        //draw table content
        for (int k = 0; k < this_phrase->len; k++) {
			Step * this_step = get_step(next_step_to_print);
            if ((pos == next_step_to_print) && (screen.screen || screen.count)) { // count indicator / playhead
                attron(A_REVERSE);
				mvprintw(py,px+45," %d ", count);
                attroff(A_REVERSE);
				screen.count = false;
            }
            if (cursor.pos_in_sequence == next_step_to_print) {
                mvprintw(py, px, "-----------------------------------");
				screen.cursor = false;
            } 
            mvprintw(py, px-6,      "|%d.", k);
            mvprintw(py, px+0,      "%d", this_step->cva);
            mvprintw(py, px+7,      "%d", this_step->cvb);
            mvprintw(py, px+14,     "%d", this_step->dur);
            if (this_step->on) { 
                attron(A_REVERSE);
                mvprintw(py, px+21,     "   ");
                attroff(A_REVERSE);
            }
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
    }
}

void arc_redraw() {
    int i;
    for(i=0;i<4;i++) { monome_led_ring_all(arc, i, 0); } // all lights off
    monome_led_ring_range(arc, 0, 0, (cursor.step_pointer->cva / 2)-1, 15);
    monome_led_ring_range(arc, 1, 0, (cursor.step_pointer->cvb / 2)-1, 15);
    monome_led_ring_range(arc, 2, 0, (cursor.step_pointer->dur / 2)-1, 15);
    screen.arc= false;
	// split ring 3 into equal parts...
	for (int i=0; i<cursor.phrase_pointer->len;i++) {
		int led = (64 / cursor.phrase_pointer->len) * i;
		monome_led_ring_set(arc, 3, led, (i == cursor.pos_in_phrase ? 15 : 8));
		monome_led_ring_set(arc, 3, led-1, (i == cursor.pos_in_phrase ? 8 : 4));
		monome_led_ring_set(arc, 3, led+1, (i == cursor.pos_in_phrase ? 8 : 4));
	}	
}

void redraw() {
	if (screen.screen) { erase(); } 
    attroff(A_REVERSE);
    //mvprintw(1,1,"cursor: %d, pos: %d, phrase 0 len: %d", cursor.pos_in_sequence, pos, phrases[playlist.list[0]].len);
	draw_table();
    draw_context();
    if (screen.arc && arc_connected) {
        arc_redraw();
    }
    refresh();
    screen.screen = false;
}

void add_step() {
    Phrase * ph = cursor.phrase_pointer;
    ph->len = clamp(ph->len+1, 0, 127);
    for (int i = ph->len-1; i >= cursor.pos_in_phrase; i--) {
        ph->steps[i + 1] = ph->steps[i];
    }
    cursor_move(1);
    init_step(cursor.step_pointer);
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
}

void * keyboard_input() {
    while (ch != '0') {
        switch (ch) {
			case '~':
				debug = ! debug;
				break;
            case KEY_LEFT:
                break;
            case KEY_DOWN: 
                cursor_move(1);
                break;
            case KEY_UP:
                cursor_move(-1);
                break;
			case KEY_RIGHT:
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
                cursor.step_pointer->cva = clamp(cursor.step_pointer->cva+1, 0, 127);
                break;
            case 'w':
                cursor.step_pointer->cvb = clamp(cursor.step_pointer->cvb+1, 0, 127);
                break;
            case 'e':
                cursor.step_pointer->dur = clamp(cursor.step_pointer->dur+1, 0, 127);
                break;
            case 'a':
                cursor.step_pointer->cva = clamp(cursor.step_pointer->cva-1, 0, 127);
                break;
            case 's':
                cursor.step_pointer->cvb = clamp(cursor.step_pointer->cvb-1, 0, 127);
                break;
            case 'd':
                cursor.step_pointer->dur = clamp(cursor.step_pointer->dur-1, 0, 127);
                break;
            case 'Q':
                cursor.step_pointer->on = !cursor.step_pointer->on;
                break;
            case 'W':
                cursor.step_pointer->prob = clamp(cursor.step_pointer->prob+1, 0, 100);
                break;
            case 'E':
                cursor.step_pointer->every = clamp(cursor.step_pointer->every+1, 1, 127);
                break;
            case 'A':
                cursor.step_pointer->on = !cursor.step_pointer->on;
                break;
            case 'S':
                cursor.step_pointer->prob = clamp(cursor.step_pointer->prob-1, 0, 100);
                break;
            case 'D':
                cursor.step_pointer->every = clamp(cursor.step_pointer->every-1, 1, 127);
                break;
        }

        ch = getch();
        screen.screen = true;
    }
    return 0;
}

void note() {
    //char * command;
    //asprintf(&command, "python3 pmod.py note %d %d", get_step(pos)->cva, get_step(pos)->cvb);
    //system(command);
    //free(command);
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
					screen.count = true;
                    //note();
                    break;
                }
            }
        }
    }
}

void clock_step() {
    screen.screen = true;
	screen.count = true;
    advance();
}

void * fast_tick() {
    while (ch != '0') {
        usleep(1000); t++;
        if (t % 128 == 0) {
            clock_step();
        }
        if (screen.screen) { redraw(); }
	if(screen.arc && arc_connected) { arc_redraw(); }
    }
    return 0;
}

int main() {
    open_arc();
	init_screen(&screen);
    init_curses();
    init_playlist(&playlist);
    for (int i=0;i<128;i++) { init_phrase(&phrases[i]); }
    init_cursor(& cursor);
    cursor.step_pointer = & phrases[0].steps[0];
    cursor.phrase_pointer = & phrases[0];
    clipboard = phrases[playlist.list[0]].steps[0];

    pthread_create(&tid[0], NULL, fast_tick, (void *) &tid[0]);
    pthread_create(&tid[1], NULL, keyboard_input, (void *) &tid[1]);

    if (arc_connected) { monome_event_loop(arc); }

    pthread_join(tid[1], NULL);
    if (arc_connected) { monome_close(arc); }
    endwin();
    return 0;
}