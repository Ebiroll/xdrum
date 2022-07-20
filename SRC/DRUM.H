/* 
 * drum.h
 * 
 * Module for definition of the main data structures 
 * 
 * 
 * 
 */ 

#ifndef _DRUM_H
#define _DRUM_H

#include <windows.h>

#define MAX_DRUMS 80
#define MAX_PATTERNS 80
#define MAX_BEAT 16

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

extern tDelay Delay;


#endif

