/* 
 * widget.c
 * 
 * Module creating the widget hierarchy.
 * 
 * $Date: 1996/03/23 20:16:50 $
 * 
 * $Revision: 1.3 $
 * 
 * 
 */ 


#include "drum.h"
extern tPattern      Pattern[];
#include <stdio.h>
#include "xdrum.h"


void create_widget_hierarchy(Widget top_level, 
                             char *speed_buf, char *PatternBuf, 
                             char *song_name_buf, graph_ctx *g_ctx)
{
   Arg args[3];
   Pixmap play_btn_pixmap, record_btn_pixmap, stop_btn_pixmap;
   Pixmap zoom_in_btn_pixmap, zoom_out_btn_pixmap;
   Display *dpy;
   Widget w;
   int i,k,col=0,row=0;
   char name[5];
   
   dpy = XtDisplay(top_level);

   play_btn_pixmap = XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), 
   				play_bits, play_width, play_height);
   record_btn_pixmap = XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), 
   				record_bits, record_width, record_height);
   stop_btn_pixmap = XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), 
   				stop_bits, stop_width, stop_height);

   ww.main_form = MW ("main", formWidgetClass, top_level,
		      XtNorientation, XtorientVertical, NULL);

   
   ww.DrumForm = MW ( "DrumForm", formWidgetClass, ww.main_form,NULL);

/*   XtInsertEventHandler(ww.DrumForm, KeyPressMask ,False,
		     (XtEventHandler)KeyHandler,
		     NULL,XtListTail);
*/
   
   ww.menu_box = MW ("menu_box", boxWidgetClass, ww.DrumForm, 
      XtNborderWidth, 1, 
      XtNorientation, XtorientHorizontal,
      NULL); 					
   

   ww.file_menu_btn = MW ("file_menu_btn", menuButtonWidgetClass, ww.menu_box, 
      XtNlabel, "File", 
      XtNmenuName, "file_menu", NULL);


   ww.file_menu = XtCreatePopupShell ("file_menu", simpleMenuWidgetClass, 
      ww.file_menu_btn, NULL, 0);

   ww.load_btn = MW ("load_btn", smeBSBObjectClass, ww.file_menu, 
      XtNlabel, "Load...", NULL);
   ww.save_btn = MW ("save_btn", smeBSBObjectClass, ww.file_menu, 
      XtNlabel, "Save...", NULL);
   ww.nothing = MW ("nothing", smeLineObjectClass, ww.file_menu, NULL);
 
  ww.delay_btn = MW ("delay_btn", smeBSBObjectClass, ww.file_menu, 
      XtNlabel, "Delay", NULL);

  ww.nothing = MW ("nothing", smeLineObjectClass, ww.file_menu, NULL);
   ww.quit_menu_btn = MW ("quit_menu_btn", smeBSBObjectClass, ww.file_menu, 
      XtNlabel, "Quit", NULL);
   

   ww.BassRadioBox[0] = MW ("BassRadioBox", boxWidgetClass, ww.DrumForm, 
			    XtNfromHoriz, ww.menu_box,
			    XtNorientation, XtorientVertical, NULL);

   
   for (i=1;i<6;i++)  {
      
   ww.BassRadioBox[i] = MW ("BassRadioBox", boxWidgetClass, ww.DrumForm, 
			 XtNfromHoriz, ww.BassRadioBox[i-1],
			 XtNorientation, XtorientVertical, NULL);

   
   }

   if (Pattern[0].DrumPattern[0].Drum==NULL)  {
      fprintf(stderr,"Somethig is WRONG!! With your drum def file!");
   }
	       
   ww.BassRadioButton[0][0] = MW ("BassRadioButton", toggleWidgetClass, ww.BassRadioBox[0], 
				  XtNstate, False, 
				  XtNwidth, 60,
				  XtNradioData, "Bass0", 
				  XtNlabel, Pattern[0].DrumPattern[0].Drum->Name, NULL);
   
   XtAddCallback (ww.BassRadioButton[0][0], XtNcallback, ChangeDrum,(XtPointer) 0);

   col=Pattern[0].DrumPattern[0].Drum->col;
   row=0;
   
   for (k=1;k<MAX_DRUMS;k++)  {
       
      if (Pattern[0].DrumPattern[k].Drum!=NULL)  {
	 if (Pattern[0].DrumPattern[k].Drum->col>col) {
	    row=0;
	    col=Pattern[0].DrumPattern[k].Drum->col;
	    ww.BassRadioButton[col-1][0] = MW ("BassRadioButton", toggleWidgetClass, ww.BassRadioBox[col-1], 
					     XtNstate, False, 
					     XtNwidth, 60,
					     XtNradioData, "Bass1", 
					     XtNradioGroup, ww.BassRadioButton[0][0],    
					     XtNlabel, Pattern[0].DrumPattern[k].Drum->Name, NULL);

	    XtAddCallback (ww.BassRadioButton[col-1][0], XtNcallback, ChangeDrum,(XtPointer) k);
	    row=1;
	 } else  {

	 col=Pattern[0].DrumPattern[k].Drum->col;
	    
	 ww.BassRadioButton[col-1][row] = MW ("BassRadioButton2", toggleWidgetClass, ww.BassRadioBox[col-1], 
				     XtNstate, False, 
				     XtNwidth, 60,
				     XtNradioData, "Bass2", 
				     XtNradioGroup, ww.BassRadioButton[0][0], 
				     XtNlabel,  Pattern[0].DrumPattern[k].Drum->Name, NULL);

	  XtAddCallback (ww.BassRadioButton[col-1][row], XtNcallback, ChangeDrum,(XtPointer) k);
	  row++;
	 }
      }
   }
   
   
   
   ww.PatternForm = MW ( "PatternForm", formWidgetClass, ww.main_form,
			XtNfromVert, ww.DrumForm,
			NULL);
   
   ww.PatternRadioBox = MW ("PatternRadioBox", boxWidgetClass, ww.PatternForm,
			  XtNorientation, XtorientHorizontal, NULL);

   ww.PatternRadioButton[0] = MW ("PatternRadioButton1", toggleWidgetClass, ww.PatternRadioBox, 
      XtNstate, False, 
      XtNwidth, 15,
      XtNradioData, "1", 
      XtNlabel, "0", NULL);

   
   for (i=1;i<16;i++) {
      sprintf(name,"%d",i%4);
      ww.PatternRadioButton[i] = MW ("PatternRadioButton1", toggleWidgetClass, ww.PatternRadioBox, 
				     XtNstate, False, 
				     XtNwidth, 15,
				     XtNradioData, "2", 
				     XtNlabel, name, NULL);
   }

   ww.PatternLabel = MW ("PatternLabel", commandWidgetClass, ww.PatternRadioBox, 
			XtNfromHoriz,ww.PatternRadioButton[15], 
			XtNhorizDistance, 5, 
		        XtNlabel, "Patt:",
			XtNborderWidth, 0, NULL);
   
   ww.PatternName = MW ("PatternName", asciiTextWidgetClass, ww.PatternRadioBox, 
		       XtNfromHoriz,ww.PatternLabel, 
		       XtNstring, PatternBuf, 
		       XtNuseStringInPlace, True, 
		       XtNlength, 10, 
		       XtNwidth, 20, 
		       XtNeditType, XawtextRead, NULL);

   
   ww.TempoLabel = MW ("PatternLabel", labelWidgetClass, ww.PatternForm, 
		       XtNfromHoriz,ww.PatternRadioBox, 
		       XtNhorizDistance, 5, 
		       XtNlabel, "Tempo:",
		       XtNborderWidth, 0, NULL);
   
   ww.TempoName = MW ("TempoName", asciiTextWidgetClass, ww.PatternForm, 
		      XtNfromHoriz,ww.TempoLabel, 
		      XtNstring, speed_buf, 
		      XtNuseStringInPlace, True, 
		      XtNlength, 10, 
		      XtNwidth, 60, 
		      XtNeditType, XawtextEdit, NULL);   
   

   ww.ControlForm = MW ( "ControlForm", formWidgetClass, ww.main_form,
			XtNfromVert, ww.PatternForm,
			NULL);
   
   
   ww.stop_btn = MW ("stop_btn", commandWidgetClass, ww.ControlForm, 
      XtNwidth, 60, 
      XtNbitmap, stop_btn_pixmap, 
      XtNsensitive, False, 
      NULL);

   ww.play_btn = MW ("play_btn", commandWidgetClass, ww.ControlForm, 
      XtNfromHoriz, ww.stop_btn,
      XtNwidth, 60,
      XtNbitmap, play_btn_pixmap, 
      NULL);

   ww.song_stop_btn = MW ("record_btn", commandWidgetClass, ww.ControlForm, 
      XtNfromHoriz, ww.play_btn, 
      XtNwidth, 60,
      XtNhorizDistance, 20, 
      XtNbitmap, stop_btn_pixmap,
      XtNsensitive, False, 
      NULL);

   ww.song_play_btn = MW ("song_play_btn", commandWidgetClass, ww.ControlForm, 
      XtNfromHoriz, ww.song_stop_btn, 
      XtNwidth, 60,
      XtNbitmap, play_btn_pixmap, 
      NULL);

   ww.title_label = MW ("title_label", labelWidgetClass, ww.ControlForm, 
      XtNfromHoriz,ww.song_play_btn, 
      XtNhorizDistance, 5, 
      XtNlabel, "Song: ",
      XtNborderWidth, 0, NULL);

   ww.Song = MW ("Song", asciiTextWidgetClass, ww.ControlForm, 
      XtNfromHoriz,ww.title_label, 
      XtNstring, song_name_buf, 
      XtNuseStringInPlace, True, 
      XtNlength, 400, 
      XtNwidth, 130,
      XtNheight, 36,
      XtNscrollVertical,XawtextScrollAlways,
      XtNeditType,XawtextEdit, NULL);
   
   
/*   Maybe one day I will try to get this to work 
   ww.Scrollbar = MW ("scrollbar", scrollbarWidgetClass, ww.Song, 
      XtNorientation,XtorientHorizontal, 
      XtNlength, 400, 
      XtNthickness, 16,
      XtNshown,0.1,
      NULL);
   Probably not...
 * */
   
   
   XtOverrideTranslations(ww.DrumForm, XtParseTranslationTable
			  ("Shift<Key>: ShiftKeyPress()\n\
                           <Key>: KeyPress()"));

   XtOverrideTranslations(ww.PatternForm, XtParseTranslationTable
			  ("Shift<Key>: ShiftKeyPress() \n\
                            <Key>: KeyPress()"));

   for (i=0;i<16;i++) {
      XtAddCallback (ww.PatternRadioButton[i], XtNcallback, SetBeat,(XtPointer) i);
   }
   
   XtAddCallback (ww.quit_menu_btn, XtNcallback, quit_question, 0);
   XtAddCallback (ww.stop_btn, XtNcallback, stop_playing_pattern, 0);
   XtAddCallback (ww.play_btn, XtNcallback, play_pattern, 0);
   XtAddCallback (ww.song_play_btn, XtNcallback, play_song, 0);
   XtAddCallback (ww.song_stop_btn, XtNcallback, stop_playing_song, 0);


   XtAddCallback (ww.load_btn, XtNcallback, open_dialog, 0);
   XtAddCallback (ww.save_btn, XtNcallback, open_dialog, 0);

   XtAddCallback (ww.delay_btn, XtNcallback, delay_dialog, 0);
   
   XtAddCallback (ww.PatternLabel, XtNcallback, pattern_dialog, 0);

}


/* Wouldn't it be nice with colors in this program. 
 */
void create_GC(graph_ctx *g_ctx)
{
   XGCValues xgcv;


   /* GC for border lines */
   xgcv.foreground = define_color(ww.main_form, "red");
   xgcv.background = define_color(ww.main_form, "black");
   xgcv.line_width = 2;
   xgcv.line_style = LineOnOffDash;


}

int define_color(Widget w, char *color)
{
   XColor exact_color, screen_color;
   Screen *screen;
   Display *dpy;

   dpy = XtDisplay(w);
   screen = XDefaultScreenOfDisplay(dpy);

   if ((XDefaultVisualOfScreen(screen))->class == TrueColor 
        ||  (XDefaultVisualOfScreen(screen))->class == PseudoColor
        ||  (XDefaultVisualOfScreen(screen))->class == DirectColor
        ||  (XDefaultVisualOfScreen(screen))->class == StaticColor)

      if (XAllocNamedColor(dpy, XDefaultColormapOfScreen(screen),
          color, &screen_color, &exact_color))
         return screen_color.pixel; 
      else
         fprintf(stderr, "Color not allocated!\n");
   else 
      if (!strcmp(color, "black")) 
         return XBlackPixelOfScreen(screen);
      else
         return XWhitePixelOfScreen(screen);
}

