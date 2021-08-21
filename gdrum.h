/* 
 * gdrum.h
 * 
 * Definition of some usefull stuff for sbdrum.c
 * I just dont know if it is useful anymore though..
 * 
 * $Date: 1996/03/24 15:48:30 $
 * 
 * $Revision: 1.2 $
 * 
 * 
 */ 


#ifndef _GDRUM_H
#define _GDRUM_H

extern unsigned char _seqbuf[];
extern int _seqbuflen;
extern int _seqbufptr;

extern void seqbuf_dump(void);
extern int seqfd, dev;

extern int load_sample (int samplenum,char *name,int pan,int Intel);


#define CMD_ARPEG		0x00
#define CMD_SLIDEUP		0x01
#define CMD_SLIDEDOWN		0x02
#define CMD_SLIDETO		0x03
#define SLIDE_SIZE		8
#define CMD_VOLSLIDE		0x0a
#define CMD_JUMP		0x0b
#define CMD_VOLUME		0x0c
#define CMD_BREAK		0x0d
#define CMD_SPEED		0x0f
#define CMD_NOP			0xfe
#define CMD_NONOTE		0xff

struct note_info
  {
    unsigned char   note;
    unsigned char   vol;
    unsigned char   sample;
    unsigned char   command;
    short           parm1, parm2;
  };


struct voice_info
  {
    int             sample;
    int             note;
    int             volume;
    int             pitchbender;

    /* Pitch sliding */

    int             slide_pitch;
    int             slide_goal;
    int             slide_rate;

    int             volslide;
  };

#endif
