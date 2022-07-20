/*
 *  xdrum.c 
 *   
 * Module for creating the interface with all its callbacks, etc.
 * 
 * $Date: 1996/04/08 18:39:33 $
 * 
 * $Revision: 1.6 $
 * 
 *                  (C) Olof Astrand        
 * See README file for details.
 *
 * This code is based on the work made by:
 *         Andre Fuechsel
 * 
 */

/* #define DEBUG */

#ifdef DEBUG
#define debug(f,x)        fprintf(stderr,f,x);
#else
#define debug(f,x)        {  }
#endif

#define NAME_LENGTH 90

#include <stdio.h>


#include "drum.h"

// #include <assert.h>


/* Global variables */

/* This is tottaly messy.. It needs to be rewritten.
 * There is too many darn indexes...
 */
int DrumIndex=0;            /* Which drum are we manipulating */
extern int PatternIndex;  /* Which Pattern are we manipulating */
int SongPatternIndex=0;     /* Which Pattern is playing right now in the song*/
int M=0;                    /* What Measure are we playing right now */
int SongM=0;                /*  */ 
int buffIndex=0;            /* */ 
int SongPlay=0;            /* Boolean variable that determines 
			     if song or pattern is being played 
			     I dont think it is ever used...*/
                   /* Beats per minute. Set when play is pushed */
unsigned long UpdateIntervall;   /* Time between two PlayPattern calls */
unsigned long RealUpdateIntervall; /* Time to play one pattern */

 

extern tPattern      Pattern[];

pDrumPattern PattP;


char speed_buf[7];   /* #define this 7. */
char limit_buf[100];
char PatternBuf[3];
char SongBuf[SONG_BUF_LENGTH];
char song_name_buf[NAME_LENGTH];

   
char *edit_buf; 			/* edit buffer for samples */
int x1_old = 0, x2_old = 0;             /* coordiantes of old select window */

int visible_sample;			/* visible part of sample */
// DWORD start_visible;		/* pointer to first data displayed */
int pid=-1; 				/* pid of the play process */

int start_main(int argc, char **argv)
{
 
//   PatternIndex=StringToPatternIndex("A");  /* Which Pattern are we manipulating */
   
   /* Initialize the sound card */
   // MAIN(argc,argv);

   PattP=&Pattern[0].DrumPattern[0];
   /* create the widget hierarchy */

   
 return(1);
}

/*
 * IncreasePattern
 * 
 * I really dont think this function is necessary
 * 
 * Function to be called whenever an entire pattern is played.
 * The funtion increases M. And updates the SongPatternIndex if 
 * it is necessary.
 * 
 *  Not implemented
 */


/* 
 *  PatternIndexToString(int PI)
 * 
 *  Finds the string for a given patternindex
 * 
 */
char *PatternIndexToString(int PI)
     {
//	return(XKeysymToString(XKeycodeToKeysym(dpy,PI,0)));
		return("NISSE");

     }


/* 
 *  StringToPatternIndex(char *string)
 * 
 *  Finds the patternindex for a given string
 * 
 */
int StringToPatternIndex(char *string)
{
//	return(XKeysymToKeycode(dpy,XStringToKeysym(string)));
return(1);
}


/********************************
 * 
 * Callbacks and subroutines
 * 
 ********************************/


/*
 *  CopyBeat
 * 
 *  If shift and key is pressed then the pattern is overwritten.
 * 
 */
void CopyBeat(int FromPattern,int ToPattern) {
   int k,j;
   
   for (j=0;j<MAX_DRUMS;j++)  {
      
      for (k=0;k<16;k++) {
   
	 Pattern[ToPattern].DrumPattern[j].beat[k]=
	 Pattern[FromPattern].DrumPattern[j].beat[k];
	 
      }
   }
}

/*
 *  SetBeat
 * 
 *  Callback to be invoked whenever a beat has been pushed (toggled).
 *  The call_data contains the number of the beat (0-15) that has been pushed.
 */
void SetBeat(void  *client_data)
{
   int BeatNr=(int)client_data;

   if (Pattern[PatternIndex].DrumPattern[DrumIndex].beat[BeatNr]>0)  {
      Pattern[PatternIndex].DrumPattern[DrumIndex].beat[BeatNr]=0;
   } else  {
      Pattern[PatternIndex].DrumPattern[DrumIndex].beat[BeatNr]=1;
   }
   
}
/*
 *  SetBeats
 * 
 *  Sets the PatternRadioButtons (beats) to the beat of the drum of this pattern.
 * 
 */
void SetBeats(int DrumNr) {
   int k;

   
   for (k=0;k<16;k++) {
   
      if (Pattern[PatternIndex].DrumPattern[DrumNr].beat[k]>0)  {
//	 XtSetValues(ww.PatternRadioButton[k],set,1);
      } else  {
//	  XtSetValues(ww.PatternRadioButton[k],unset,1);
      }  
   }
}

/*
 * ChangeDrum
 * 
 * Callback to be called whenever a new drum has been pushed.
 * PattP will change to the new pattern of this drum.
 * SetBeats will be called to set the toggle buttons.
 */


void ChangeDrum(void *client_data)
{
   int DrumNr=(int)client_data;

   
   if (Pattern[PatternIndex].DrumPattern[DrumNr].Drum->Loaded!=1)  {
      debug("Load sample num %d\n",DrumNr);
//      load_sample(
//		  Pattern[PatternIndex].DrumPattern[DrumNr].Drum->SampleNum,
//		  Pattern[PatternIndex].DrumPattern[DrumNr].Drum->Filename,
//		  Pattern[PatternIndex].DrumPattern[DrumNr].Drum->pan,
//		  Pattern[PatternIndex].DrumPattern[DrumNr].Drum->Filetype);
		  
      Pattern[PatternIndex].DrumPattern[DrumNr].Drum->Loaded=1;
   }
   
   DrumIndex=DrumNr;
   PattP=&Pattern[PatternIndex].DrumPattern[DrumNr];
//   assert(PattP!=NULL);
   SetBeats(DrumNr);
   
}


#ifdef OLLEPOLLE
/*
 * stop_playing_pattern
 * Callback to be called whenever the user pushes the pattern stop button.
 * 
 */
void stop_playing_pattern(Widget w, XtPointer client_data, XtPointer call_data)
{
   XtRemoveTimeOut(Id);
   XtSetSensitive(ww.play_btn,True);
   XtSetSensitive(ww.stop_btn,False);
   XtSetSensitive(ww.song_stop_btn,False);
   XtSetSensitive(ww.song_play_btn,True);
   StopPlaying();
}

/*
 * stop_playing_song
 * Callback to be called whenever the user pushes the song stop button.
 * 
 */
void stop_playing_song(Widget w, XtPointer client_data, XtPointer call_data)
{
   XtRemoveTimeOut(Id);
   XtRemoveTimeOut(UpdateSongId);
   XtSetSensitive(ww.play_btn,True);
   XtSetSensitive(ww.stop_btn,False);
   XtSetSensitive(ww.song_stop_btn,False);
   XtSetSensitive(ww.song_play_btn,True);
   XtSetSensitive(ww.Song,True);
   StopPlaying();
}
#endif


#ifdef SONG
/*
 *  UpdateSong
 *  Intervall Timeout function used to update the song buffer pointer
 *  while the song is playing
 * 
 */
void UpdateSong(XtPointer client_data,XtIntervalId *Dummy)  
{
   struct timeval Time;
   struct timezone Tz;
   long newtime;
   char Test[2];
   int testindex;

   if (SongBuf[buffIndex]=='\n') {
      XtVaSetValues (ww.Song, XtNinsertPosition, buffIndex-1, NULL);
   } else if (SongBuf[buffIndex-1]=='\n') {
      XtVaSetValues (ww.Song, XtNdisplayPosition, buffIndex, NULL); 
      XtVaSetValues (ww.Song, XtNinsertPosition, buffIndex, NULL);
   } else  {
      if (buffIndex>=0)  {	 
	 XtVaSetValues (ww.Song, XtNinsertPosition, buffIndex, NULL);
      }
   }

   if (SongBuf[buffIndex]=='\0')  {
    /* Fake a stop song press */
   stop_playing_song((Widget) NULL, (XtPointer) NULL,(XtPointer) NULL);  

   } else  {
      
      if (gettimeofday(&Time,&Tz)<0)  {
	 fprintf (stderr,"Something went wrong with the time call..\n");
      }
      
      /* No thinking was done here just pure testing */
      newtime=(M)*RealUpdateIntervall-(unsigned long)((240+(1000*(Time.tv_sec-StartTime.tv_sec))+
					      ((Time.tv_usec-StartTime.tv_usec)/1000)));
      if (newtime<0) newtime=1;   

      UpdateSongId=XtAppAddTimeOut(app_context,newtime,(XtTimerCallbackProc)UpdateSong,NULL);

   }
}
#endif

#ifdef TIME
/*
 *  PlayMeasure
 *  Intervall Timeout function used to fill the /dev/sequencer with new
 *  data when the beat is almost done. (for GUS)
 */
void PlayMeasure(XtPointer client_data,XtIntervalId *Dummy)  
{
   struct timeval Time;
   struct timezone Tz;
   long newtime;
   
   PlayPattern(PatternIndex,M);

   if (gettimeofday(&Time,&Tz)<0)  {
      fprintf (stderr,"Something went wrong with the time call");
   }

   /* No thinking was done here just pure testing 
    * It might not work for a system slower than DX2 66MHz */
   
   newtime=(M)*UpdateIntervall-(unsigned long)((240+(1000*(Time.tv_sec-StartTime.tv_sec))+
					      ((Time.tv_usec-StartTime.tv_usec)/1000)));
   if (newtime<0) newtime=1;
   
   Id=XtAppAddTimeOut(app_context,newtime,(XtTimerCallbackProc)PlayMeasure,NULL);

}

#endif


#ifdef TIME
/*
 *  PlaySongMeasure
 *  Intervall Timeout function used to fill the /dev/sequencer with new
 *  data when the beat is almost done.
 */

void PlaySongMeasure(XtPointer client_data,XtIntervalId *Dummy)  
{
   struct timeval Time;
   struct timezone Tz;
   long newtime;
   char Test[2];
   
   if (SongM!=M)  {
      buffIndex++;
      if (SongBuf[buffIndex]=='\n') {
	 buffIndex++;
      }   
      Test[0]=SongBuf[buffIndex];
      Test[1]='\0';
      SongPatternIndex=XKeysymToKeycode(dpy,XStringToKeysym(Test));
      SongM=M;
   }
   
    PlayPattern(SongPatternIndex,SongM);

   if (gettimeofday(&Time,&Tz)<0)  {
      fprintf (stderr,"Something went wrong with the time call..\n");
   }
   
   /* No thinking was done here just pure testing */
   newtime=(M)*UpdateIntervall-(unsigned long)((240+(1000*(Time.tv_sec-StartTime.tv_sec))+
					      ((Time.tv_usec-StartTime.tv_usec)/1000)));
   if (newtime<0) newtime=1;
   
   Id=XtAppAddTimeOut(app_context,newtime,(XtTimerCallbackProc)PlaySongMeasure,NULL);

}


/*
 *  play_pattern
 *  Callback used to start the playing of one Pattern.
 */
void play_pattern(Widget w, XtPointer client_data, XtPointer call_data)
{
   struct timeval Time;
   struct timezone Tz;

   long newtime;

   SongPlay=0; 

//   sscanf(speed_buf,"%d",&BPM);
//   if (BPM>640)  {
//      BPM=640;
//   }

//   if (BPM<1)  {
//      BPM=1;
//   }
   starttimer();
   M=0;
   PlayPattern(PatternIndex,0);

   if (gettimeofday(&StartTime,&DummyTz)<0)  {
      fprintf (stderr,"Something went wrong with the time call");
   }

   UpdateIntervall=GetUpdateIntervall(PatternIndex);    

   if (gettimeofday(&Time,&Tz)<0)  {
      fprintf (stderr,"Something went wrong with the time call");
   }

   
   newtime=UpdateIntervall-(unsigned long)((240+(1000*(Time.tv_sec-StartTime.tv_sec))+
						((Time.tv_usec-StartTime.tv_usec)/1000)));

   if (newtime<0) newtime=1;

   Id=XtAppAddTimeOut(app_context,newtime,(XtTimerCallbackProc)PlayMeasure,NULL);

   XtSetSensitive(ww.stop_btn,True);
   XtSetSensitive(ww.play_btn,False);
   XtSetSensitive(ww.song_play_btn,False);
   XtSetSensitive(ww.song_stop_btn,False);

}

/*
 *  play_song
 *  Callback used to start the playing of the song.
 */
void play_song(Widget w, XtPointer client_data, XtPointer call_data)
{
   struct timeval Time;
   struct timezone Tz;
   long newtime;
   char Test[2];
   SongPlay=1;

   SongM=0;
   buffIndex=0;
   Test[0]=SongBuf[buffIndex];
   Test[1]='\0';
   SongPatternIndex=XKeysymToKeycode(dpy,XStringToKeysym(Test));   
//   sscanf(speed_buf,"%d",&BPM);
//   if (BPM>640)  {
//      BPM=640;
//   }

//   if (BPM<1)  {
//      BPM=1;
//   }
   
   starttimer();
   M=0;
   PlayPattern(SongPatternIndex,0);

   if (gettimeofday(&StartTime,&DummyTz)<0)  {
      fprintf (stderr,"Something went wrong with the time call..\n");
   }
  
   UpdateIntervall=GetUpdateIntervall(PatternIndex);    
   if (gettimeofday(&Time,&Tz)<0)  {
      fprintf (stderr,"Something went wrong with the time call..\n");
   }
   newtime=UpdateIntervall-(unsigned long)((240+(1000*(Time.tv_sec-StartTime.tv_sec))+
					    ((Time.tv_usec-StartTime.tv_usec)/1000)));

   if (newtime<0) newtime=1;

   Id=XtAppAddTimeOut(app_context,newtime,(XtTimerCallbackProc)PlaySongMeasure,NULL);

   
//   RealUpdateIntervall=(unsigned long)(1000*240/BPM);    
   
   newtime=RealUpdateIntervall-
   (unsigned long)(240+((1000*(Time.tv_sec-StartTime.tv_sec))+
		    ((Time.tv_usec-StartTime.tv_usec)/1000)));
   
   UpdateSongId=XtAppAddTimeOut(app_context,newtime,
		      (XtTimerCallbackProc)UpdateSong
		      ,NULL);

   XtSetSensitive(ww.song_stop_btn,True);
   XtSetSensitive(ww.song_play_btn,False);
   XtSetSensitive(ww.play_btn,False);
   XtSetSensitive(ww.stop_btn,False);
   XtSetSensitive(ww.Song,False);

}

void get_changeable_options()
{
   char *wid;

   options.speed = atoi(speed_buf); 
}

void record_question(Widget w, XtPointer client_data, XtPointer call_data)
{

}

void record_sample()
{
}

void update_display()
{

}

/*
 * open_dialog 
 * Callback to be invoked whenever the Files... button
 * is pushed.
 * 
 */
void open_dialog(Widget w, XtPointer client_data, XtPointer call_data)
{
   if (w == ww.load_btn) {    
      /*strcpy(song_name_buf, "");*/
      XtSetArg(args[0], XtNstring, song_name_buf);
      XFlush(dpy);
   
      popup_file_dialog (w, "Load file: ", song_name_buf, load_file);
   } else
     popup_file_dialog (w, "Save as: ", song_name_buf, save_file);
}

void load_file()
{
   char *pointer;
   fprintf(stderr,"Loading %s\n",song_name_buf);
   LoadFile(song_name_buf);
   SetBeats(DrumIndex);
   XtSetArg(args[0], XtNstring,SongBuf);
   XtSetValues(ww.Song, args, 1);

   XtSetArg(args[0], XtNstring,speed_buf);
   XtSetValues(ww.TempoName, args, 1);

   
/*  Loading directly into the string might cause problems??
 * XtSetArg(args[0], XtNstring, song_name_buf);
 *  XtSetValues(ww.Song, args, 1);
*/
   
}

void save_file()
{
   fprintf(stderr,"Saving %s\n",song_name_buf);
   SaveFile(song_name_buf);
}

void popup_file_dialog (Widget w, String str, String def_fn, void (*func)() )
/* Pops up a file dialog for widget w, label str, default filename def_fn */
/* calls func if the user clicks on OK */
{
   DialogInfo *file_info;
   Widget file_dialog;

   Widget shell = XtCreatePopupShell("file_dialog_shell", 
   				     transientShellWidgetClass, 
          		             top_level, NULL, 0);

   if (dialog_context == None) dialog_context = XUniqueContext();

   file_dialog = MW ("file_dialog", dialogWidgetClass, shell, 
      XtNlabel, str, 
      XtNvalue, def_fn, 
      NULL);
   
   XtOverrideTranslations(file_dialog,XtParseTranslationTable
			  ("<Key>Return: PopdownFileDialog(okay)"));
							      
   
   
   /* This should work!! */
   
   
   file_info = XtNew(DialogInfo);
   assert(func!=NULL);
   
   file_info->func = func;
   

   
   
   if (XSaveContext(dpy, (Window) file_dialog, 
      dialog_context, (caddr_t) file_info) != 0) {
      fprintf(stderr, "Error while trying to save context\n");
      XtDestroyWidget(shell);
      return;
   }

   XawDialogAddButton(file_dialog, "OK", popdown_file_dialog, (XtPointer) 1);
   XawDialogAddButton(file_dialog, "Cancel", popdown_file_dialog, NULL);


   
   
   popup_centered(shell);
}

void popup_centered(Widget w)
{
   int win_x, win_y; 
   int x, y;
   Dimension width, height, b_width;
   unsigned int mask;
   Window root, child;
   Cardinal num_args;

   XtRealizeWidget(w);

   XQueryPointer(dpy, XtWindow(w), 
    		 &root,  		/* root window under the pointer */
    		 &child, 		/* child window under the pointer */
    		 &x, &y, 		/* pointer coords relative to root */
	 	 &win_x, &win_y, 	/* pointer coords relative to window */
		 &mask);		/* state of modifier keys and buttons */
    		  
   num_args = 0;
   XtSetArg(args[num_args], XtNwidth, &width); num_args++;
   XtSetArg(args[num_args], XtNheight, &height); num_args++;
   XtSetArg(args[num_args], XtNborderWidth, &b_width); num_args++;
   XtGetValues(w, args, num_args);

   width += 2 * b_width;
   height += 2 * b_width;

   x -= ((int) width/2);
   y -= ( (Position) height/2 );

   num_args = 0;
   XtSetArg(args[num_args], XtNx, x); num_args++;
   XtSetArg(args[num_args], XtNy, y); num_args++;
   XtSetValues(w, args, num_args);
     
   XtPopup(w, XtGrabExclusive);
}

void popdown_file_dialog_action(Widget w, XEvent *event, String *params, 
                                Cardinal *num_params)
{
   char buf[80];
   int val;

   if (*num_params != 1) {
      sprintf(buf, "Action `%s' must have exactly one argument.", 
              "popdown_file_dialog");
      err(1, buf);
      return;
   }

   XmuCopyISOLatin1Lowered(buf, params[0]);

   if (!strcmp(buf, "cancel"))
      val = FALSE;
   else if (!strcmp(buf, "okay"))
      val = TRUE;
   else {
      sprintf(buf, "Action %s's argument must be either `cancel' or `okay'.",
              "popdown_file_dialog");
      err(1, buf);
      return;
   }

   popdown_file_dialog(w, (XtPointer) val, (XtPointer) NULL);
}

void popdown_file_dialog(Widget w, XtPointer client_data, XtPointer call_data)
{
   Widget dialog = XtParent(w);
   caddr_t file_info_ptr;
   DialogInfo * file_info;

   if (XFindContext(dpy, (Window) dialog, dialog_context,
                    &file_info_ptr) == XCNOENT) {
      fprintf(stderr, "Error while trying to find context\n");
   }

   (void) XDeleteContext(dpy, (Window)dialog, 
                         dialog_context);

   file_info = (DialogInfo *) file_info_ptr;


     if ( ((int) client_data) == 1 ) {
       String filename = XawDialogGetValueString(dialog);
       strncpy(song_name_buf, filename, sizeof(song_name_buf));
       song_name_buf[NAME_LENGTH-2]='\0';
   
      
      (*file_info->func)(); 		/* call handler */
   }   
     
     
     
   XtPopdown(XtParent(dialog));
   XtDestroyWidget(XtParent(dialog));
   XFlush(dpy);



  /* XtFree( (XtPointer) file_info); */	/* Free data. */
}

/* X11-error handler */
void err(int severity, char *text)
{
   Widget shell, dialog; 

   /* do something tricky to avoid changing the whole errorhandler */
   /* if pid=-1 or pid>0 we are the main process and are able to   */
   /* open an error dialog; if pid = 0, we aren't!                 */

   if (!pid) { 
      fprintf(stderr, "%s\n", text);
      return;
   }

   shell = XtCreatePopupShell("dialog_shell", 
   				     transientShellWidgetClass, 
          		             top_level, NULL, 0);

   dialog = MW ("warning", dialogWidgetClass, shell, 
      XtNlabel, text, 
      NULL);

   XawDialogAddButton(dialog, "OK", close_error, NULL);

   popup_centered(shell);
}

void close_error(Widget w, XtPointer client_data, XtPointer call_data)
{
   Widget dialog = XtParent(w);

   XtPopdown(XtParent(dialog));
   XtDestroyWidget(XtParent(dialog));
   XFlush(dpy);
}

void discard_question(int flag)
/* click on YES: execute flag dependend action */ 
{
   Widget dialog;
   DialogInfo *info;

   Widget shell = XtCreatePopupShell("dialog_shell", 
   				     transientShellWidgetClass, 
          		             top_level, NULL, 0);

   if (dialog_context == None) dialog_context = XUniqueContext();

   dialog = MW ("dialog", dialogWidgetClass, shell, 
      XtNlabel, "Discard modified file?", 
      NULL);

   info = XtNew(DialogInfo);
   info->flag = flag;

   if (XSaveContext(dpy, (Window) dialog,
      dialog_context, (caddr_t) info) != 0) {
      fprintf(stderr, "Error while trying to save context\n");
      XtDestroyWidget(shell);
      return;
   }

   XawDialogAddButton(dialog, "Yes", discard_answer, (XtPointer) 1);
   XawDialogAddButton(dialog, "No", discard_answer, NULL);

   popup_centered(shell);
}

void discard_answer(Widget w, XtPointer client_data, XtPointer call_data)
{
   Widget dialog = XtParent(w);
   caddr_t info_ptr;
   DialogInfo *info;

   if (XFindContext(dpy, (Window) dialog, dialog_context, &info_ptr) ==
       XCNOENT) {
      fprintf(stderr, "Error while trying to find context\n");
   }

   (void) XDeleteContext(dpy, (Window) dialog, dialog_context);

   info = (DialogInfo *) info_ptr;

   XtPopdown(XtParent(dialog));
   XtDestroyWidget(XtParent(dialog));
   XFlush(dpy);

   if ((int) client_data == 1) {
      /* XtFree(edit_buf);*/
      edit_buf=NULL;
      x1_old = x2_old = 0;
      strcpy(song_name_buf, "");
      XtSetArg(args[0], XtNstring, song_name_buf);
      XtSetValues(ww.song_title, args, 1);
      update_display();
      XFlush(dpy);
 
      switch (info->flag) {
         case DISC_CON_QUIT:
            quit_application();
            break;
         case DISC_CON_LOAD:
            popup_file_dialog (w, "Load file: ", song_name_buf, load_file);
            break;
         case DISC_CON_RECORD:
            record_sample();
            break;
         default:
           fprintf(stderr, "internal error: no valid continuation flag\n");
           exit(-1);
     }
   }
   XtFree( (XtPointer) info); 	/* Free data. */
}

/*
 *  key_press_action
 *  Takes care of key press whithin Drum and Beat area
 *  Changes the current pattern to the letter pressed.
 *  Pattp and PatternIndex indicates the patterns being pressed
 */ 
void key_press_action(Widget w, XEvent *event, String *params, 
                          Cardinal *num_params)
{
   Modifiers Modifier;
   KeySym Key;

   if (event->type == KeyPress)  {

      if (event->xkey.keycode<MAX_PATTERNS) {
	 
	 XtTranslateKey(event->xkey.display,event->xkey.keycode,
			ShiftMask,&Modifier,&Key);

	 if (strlen(XKeysymToString(Key))<2) 
	   {
	    PatternIndex=(int)event->xkey.keycode;
	    
	    PattP=&Pattern[PatternIndex].DrumPattern[DrumIndex];
	    SetBeats(DrumIndex);
	    sprintf(PatternBuf,"%s",PatternIndexToString(PatternIndex));
	    XtSetArg(args[0], XtNstring, PatternBuf);
	    XtSetValues(ww.PatternName, args, 1);
	 }
      }
   }
}


/*
 *  shift_key_press_action
 *  Takes care of shift key press whithin Drum and Beat area.
 *  Copies the current pattern into the new pattern
 *  corresponding to the letter pressed.
 *  Pattp and PatternIndex indicates the patterns that is selected.
 */ 

void shift_key_press_action(Widget w, XEvent *event, String *params, 
                          Cardinal *num_params)
{
   Modifiers Modifier;
   KeySym Key;
   
   if (event->type == KeyPress)  {
	    
      XtTranslateKey(event->xkey.display,event->xkey.keycode,
		     ShiftMask,&Modifier,&Key);
  
      if (event->xkey.keycode<MAX_PATTERNS) {

	 if (strlen(XKeysymToString(Key))<3) {

	    CopyBeat(PatternIndex,(int)event->xkey.keycode);
	    
	    PatternIndex=(int)event->xkey.keycode;
	    PattP=&Pattern[PatternIndex].DrumPattern[DrumIndex];
	    sprintf(PatternBuf,"%s",PatternIndexToString(PatternIndex));
	    XtSetArg(args[0], XtNstring, PatternBuf);
	    XtSetValues(ww.PatternName, args, 1);
	    SetBeats(DrumIndex);
	    
	    XtTranslateKey(event->xkey.display,event->xkey.keycode,
			   ShiftMask,&Modifier,&Key);  
	 }	 	 
      }
   }
}

/*
 *  ToggleBeat
 * 
 *  Callback to be invoked whenever a popup beat has been pushed (toggled).
 *  The call_data contains an (int) pointer to the beat that has been pushed.
 */
void ToggleBeat(Widget w, XtPointer client_data, XtPointer call_data)
{
   
   int *Beat=(int *)client_data;

   if (*Beat>0)  {
      *Beat=0;
   } else  {
      *Beat=1;
   }
   
}


/*
 * set_active_pattern_pattern
 * 
 * 
 * 
 */
void set_active_pattern(Widget w, XtPointer client_data, XtPointer call_data)
{
   
   PatternIndex=(int)client_data;
   PattP=&Pattern[PatternIndex].DrumPattern[DrumIndex];
   SetBeats(DrumIndex);
   sprintf(PatternBuf,"%s",PatternIndexToString(PatternIndex));
   XtSetArg(args[0], XtNstring, PatternBuf);
   XtSetValues(ww.PatternName, args, 1);
   
   
}



/*
 * popdown_pattern_dialog
 * 
 * 
 * 
 */
void popdown_pattern_dialog(Widget w, XtPointer client_data, XtPointer call_data)
{
   Widget dialog = XtParent(w);
   caddr_t file_info_ptr;
     
     
   XtPopdown(XtParent(dialog));
   XtDestroyWidget(XtParent(dialog));
   XFlush(dpy);


  /* XtFree( (XtPointer) file_info); */	/* Free data. */
}
/******************************/
/*
 * pattern_dialog 
 * Callback to be invoked whenever the pattern button
 * is pushed.
 * 
 */
void pattern_dialog(Widget w, XtPointer client_data, XtPointer call_data)
{
   if (w == ww.PatternLabel) {    
     popup_pattern_dialog (w, "Pattern: ", song_name_buf, save_file);

   }
}


void popup_pattern_dialog (Widget w, String str, 
			   String def_fn, void (*func)() )
/* Pops up a pattern dialog for widget w, label str,
 default filename def_fn */
/* calls func if the user clicks on OK */

{
   Widget pattern_dialog;
   int beat,k,PattI;
   char name[40];
   unsigned char bytes[2];
   Widget tempw,beatw,tempformw;
   Widget shell = XtCreatePopupShell("pattern_dialog_shell", 
   				     transientShellWidgetClass, 
          		             top_level, NULL, 0);
  
   pattern_dialog = MW ("pattern_dialog", formWidgetClass, shell, 
      XtNorientation,XtorientVertical,
      NULL);
   
   
   PattI=PatternIndex;
   tempw=NULL;

   
   for (k=0;k<MAX_DRUMS;k++)  {
      if (FindBytes(&bytes[0],&bytes[1],PattI,k)>0) {

	 if (PatternIndexToString(PattI)!=NULL) {
	    if (strlen(PatternIndexToString(PattI))<2) {

	       
	       if (tempw==NULL) {
		  
		  tempw=MW("PatternLabel",commandWidgetClass,pattern_dialog,
			   XtNwidth, 40,
			   XtNlabel,PatternIndexToString(PattI), NULL);
		  
		  XtAddCallback(tempw, XtNcallback,set_active_pattern,
				 (XtPointer)PattI);
		  
		  tempformw= MW ( "PatternForm", formWidgetClass, pattern_dialog,
				 XtNfromVert, tempw,
				 NULL);
		  
		  tempw=MW("DrumButton",commandWidgetClass,tempformw,
			   XtNwidth, 60,
			   XtNlabel,Pattern[PattI].DrumPattern[k].Drum->Name, NULL);
	       }
	       else 
	       {
		  
		  tempformw= MW ( "PatternForm", formWidgetClass, pattern_dialog,
				 XtNfromVert, tempformw,
				 NULL);
		  
		  tempw=MW("DrumButton",commandWidgetClass,tempformw, 
			   XtNwidth, 60,
			   XtNlabel,Pattern[PattI].DrumPattern[k].Drum->Name, NULL);
	       }


	       
	       
	       beatw= MW ("PatternRadioButton", toggleWidgetClass,tempformw,
			  XtNfromHoriz,tempw, 
			  XtNstate, (bytes[0] & (0x01 << 7)) ? True : False,
			  XtNwidth, 15,
			  XtNradioData, "1", 
			  XtNlabel, "0", NULL);

	       
	       XtAddCallback (beatw, XtNcallback, ToggleBeat,
		(XtPointer)&(Pattern[PattI].DrumPattern[k].beat[0]));
	       
	       for (beat=1;beat<8;beat++) {
		  sprintf(name,"%d",beat%4);
		  beatw= MW ("PatternRadioButton", toggleWidgetClass, tempformw, 
			     XtNstate, (bytes[0] & (0x01 << (7-beat))) ? True : False,
			     XtNfromHoriz,beatw, 
			     XtNwidth, 15,
			     XtNradioData, "1", 
			     XtNlabel, name, NULL);
		  
		  XtAddCallback (beatw, XtNcallback, ToggleBeat,
				 (XtPointer)&(Pattern[PattI].DrumPattern[k].beat[beat]));
		  
	       }
	       
	       for (beat=8;beat<16;beat++) {
		  sprintf(name,"%d",beat%4);
		  beatw= MW ("PatternRadioButton", toggleWidgetClass, tempformw, 
			     XtNstate, (bytes[1] & (0x01 << (15-beat))) ? True : False,
			     XtNfromHoriz,beatw, 
			     XtNwidth, 15,
			     XtNradioData, "1", 
			     XtNlabel, name, NULL);
		     
		  XtAddCallback (beatw, XtNcallback, ToggleBeat,
				 (XtPointer)&(Pattern[PattI].DrumPattern[k].beat[beat]));
		  
	       }
	       
	    }
	    
	 }
	 
      }
      
   }
   

   
   if (tempw!=NULL) {

      
      tempw=MW("Pattern Label",commandWidgetClass,pattern_dialog,
	       XtNfromVert, tempformw,
	       XtNwidth, 40,
	       XtNlabel,"OK", NULL);
      
      XtAddCallback (tempw, XtNcallback,popdown_pattern_dialog,
		     (XtPointer) NULL);
      

      popup_pattern(shell);
   }
   /* TODO remove all the widgets and all that crap */
}

void popup_pattern(Widget w)
{
   int win_x, win_y; 
   int x, y;
   Dimension width, height, b_width;
   unsigned int mask;
   Window root, child;
   Cardinal num_args;

   XtRealizeWidget(w);

   XQueryPointer(dpy, XtWindow(w), 
    		 &root,  		/* root window under the pointer */
    		 &child, 		/* child window under the pointer */
    		 &x, &y, 		/* pointer coords relative to root */
	 	 &win_x, &win_y, 	/* pointer coords relative to window */
		 &mask);		/* state of modifier keys and buttons */
    		  
   num_args = 0;
   XtSetArg(args[num_args], XtNwidth, &width); num_args++;
   XtSetArg(args[num_args], XtNheight, &height); num_args++;
   XtSetArg(args[num_args], XtNborderWidth, &b_width); num_args++;
   XtGetValues(w, args, num_args);

   width += 2 * b_width;
   height += 2 * b_width;

   x -= ((int) width/2);
   y -= ( (Position) height/2 );

   num_args = 0;
   XtSetArg(args[num_args], XtNx, x); num_args++;
   XtSetArg(args[num_args], XtNy, y); num_args++;
   XtSetValues(w, args, num_args);
     
   XtPopup(w, XtGrabNone);
}

#endif
