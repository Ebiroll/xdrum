/* 
 * xdrum.h
 * 
 * Include file for the X11 interface.
 * 
 * $Date: 1996/04/08 18:40:33 $
 * 
 * $Revision: 1.2 $
 * 
 * 
 */ 

#ifndef XDRUM_H
#define XDRUM_H


/* Include files required for all toolkit programs */
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/keysymdef.h>
#include <X11/Shell.h>
#include <X11/X.h>

/* Include files required for this program */

#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/Scrollbar.h>


/* bitmap includes */

#include "xbm/title.bit"
#include "xbm/stop.bit"
#include "xbm/play.bit"
#include "xbm/record.bit"

#include "play.h"

#define MW XtVaCreateManagedWidget
#define SPEED_LEN 6
#define LIMIT_LEN 6
#define NAME_LEN 31
#define GRAPH_HEIGHT 200
#define GRAPH_WIDTH 490

/* The widget hierarchy !! */

typedef struct {
   Widget main_form;
   Widget quit_btn;
   Widget play_btn, stop_btn, song_stop_btn, song_play_btn;
   Widget DrumForm;
   Widget PatternForm;
   Widget ControlForm;
   Widget BassRadioBox[20];
   Widget BassRadioButton[20][20];
   Widget PatternRadioBox;
   Widget PatternRadioButton[16];
   Widget limit;
   Widget menu_box, file_menu_btn, file_menu;
   Widget load_btn, save_btn, quit_menu_btn, nothing, delay_btn;
   Widget effects_menu_btn, effects_menu;
   Widget echo_btn;
   Widget title_label;
   Widget song_title;
   Widget Song;
   Widget Scrollbar;
   Widget PatternLabel,PatternName;
   Widget TempoLabel,TempoName;
} widget_structure;

extern widget_structure ww;

typedef struct {
   GC graph_GC, line_GC, delete_line_GC;
} graph_ctx;

/* Prototypes for all functions used */

/* Utility functions used by other modules */
char *PatternIndexToString(int PI);
int StringToPatternIndex(char *string);

/*  I think theese are local . */
void quit_question(Widget w, XtPointer client_data, XtPointer call_data);
void quit_application();
void stop_playing_pattern(Widget w, XtPointer client_cata, XtPointer call_data);
void stop_playing_song(Widget w, XtPointer client_cata, XtPointer call_data);
void play_pattern_ui(Widget w, XtPointer client_data, XtPointer call_data);
void play_song(Widget w, XtPointer client_data, XtPointer call_data);
void SetBeat(Widget w, XtPointer client_data, XtPointer call_data);
void ChangeDrum(Widget w, XtPointer client_data, XtPointer call_data);
void get_changeable_options();
void record_question(Widget w, XtPointer client_data, XtPointer call_data);
void record_sample();
void zoom_out(Widget w, XtPointer client_data, XtPointer call_data);
void zoom_in(Widget w, XtPointer client_data, XtPointer call_data);
void open_dialog(Widget w, XtPointer client_data, XtPointer call_data);
void calculate_echo(Widget w, XtPointer client_data, XtPointer call_data);
void save_file();
void load_file();
void update_display();
void set_limit_borders_action(Widget w, XEvent *event, String *params, 
                              Cardinal *num_params);
void calculate_limit_borders();
void draw_limit_borders(int x1, int x2);
void popup_file_dialog(Widget w, String str, String def_fn, void (*func)() );
void popup_centered(Widget w);
void popdown_file_dialog_action(Widget w, XEvent *event, String *params, 
                                Cardinal *num_params);
void popdown_file_dialog(Widget w, XtPointer client_data, XtPointer call_data);
void close_error(Widget w, XtPointer client_data, XtPointer call_data);
void discard_question(int flag);
void discard_answer(Widget w, XtPointer client_data, XtPointer call_data);
void create_GC(graph_ctx *g_ctx);
void create_widget_hierarchy(Widget top_level, 
                             char *speed_buf, char *limit_buf, 
                             char *song_name_buf, graph_ctx *g_ctx);
int define_color(Widget w, char *color);
void key_press_action(Widget w, XEvent *event, String *params, 
                                Cardinal *num_params);

void shift_key_press_action(Widget w, XEvent *event, String *params, 
                                Cardinal *num_params);

void pattern_dialog(Widget w, XtPointer client_data, XtPointer call_data);

void popup_pattern_dialog(Widget w, String str, String def_fn, void (*func)() );
void popup_pattern(Widget w);

void delay_dialog(Widget w, XtPointer client_data, XtPointer call_data);


#endif



