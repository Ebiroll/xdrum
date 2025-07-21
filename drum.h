/* 
 * drum.h
 * 
 * Module for definition of the main structures as well as 
 * the functions in sbdrum used by xdrum.c
 * 
 * $Date: 1996/03/23 20:16:49 $
 * 
 * $Revision: 1.2 $
 * 
 * 
 */ 

#ifndef _DRUM_H
#define _DRUM_H
#include <stdbool.h>

#define MAX_DRUMS 120
#define MAX_PATTERNS 255
#define SONG_BUF_LENGTH  400

typedef struct  {
   char *Name;
   char *Filename;
   int Loaded;
   int Filetype;
   int SampleNum;
   int col;                        /* Column in GUI */
   int pan;                        /* Panning of drum */
   int vol;                        /* Volume of drum */
   int key;                        /* Key assignment of GUS drum */
   short *sample;
   int length;
} *pDrum,tDrum;


typedef struct  {
   pDrum Drum;
   int beat[16];
} *pDrumPattern,tDrumPattern;

typedef struct  {
   tDrumPattern DrumPattern[MAX_DRUMS];
} *pPattern,tPattern;


typedef struct {
  int Delay16thBeats;
  int On;
  int count;
  short *pointer;
  float leftc;
  float rightc;
} tDelay;



     
extern int MAIN(int argc, char **argv); 
extern void starttimer(void);
extern unsigned long GetUpdateIntervall(int PattI);
extern int StopPlaying(void);
extern int EmptyCard(void);
extern int load_sample (int samplenum,const char *name,int pan,int Intel);
void CopyBeat(int FromPattern, int ToPattern);
void SetBeats(int DrumNr);

#ifdef ALSADRUM
void close_alsa_device(void);
void PlayPatternThreaded(int pattern, int measure, bool loop);
int stop_playing(void);
#else
extern void PlayPattern(int Patt,int Measure);
#endif
#endif

