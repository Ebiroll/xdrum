/*
 *	sbdrum.c  - Drum player for some Soundcards and Linux.
 *              (C) Olof Astrand, 2021
 *      See README file for details.
 * * 
 * 
 *      Some of this code is based on the work made by, 
 *              Hannu Savolainen
 *      I hope you dont mind.
 * 
 *	NOTE!	This is a good example of unmanagable 
 *              code. Three versions that does almost the same
 *              thing.
 *              1. GUS version.
 *              2. 16 bit /dev/dsp
 *              3. 8 bit /dev/dsp
 * 
 *              Preprocessor statements like #ifndef NOGUS
 *              can be very confusing...
 *               
 */

/* Modify this part if you have GUS or want 8-bits*/ 
#define NOGUS 
#define SC16BITS

/* Leave the rest */

#ifdef DEBUG
#define debug(f,x)        fprintf(stderr,f,x);
#else
#define debug(f,x)        {  }
#endif

#define MAX_VOICE    32
#define BL 16384 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/soundcard.h>
//#include <sys/ultrasound.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>

#include "gdrum.h"
#include "drum.h"
#include "gusload.h"
#include <assert.h>

#ifndef DRUMS_ROOT_DIR 
#define DRUMS_ROOT_DIR	"." 
#endif 

#define MIN(a, b)		((a) < (b) ? (a) : (b))


#define SLEN 265535

int LeftOfBlock=BL;
int BlockCounter=0;
int BlockReady;

unsigned char            data[BL];

#define MAX_DELAY 200000

short delayData[MAX_DELAY];
int DelayPointer;

long                     tmpstore[SLEN];

typedef struct  {
   short *pointer;
   int count;
   int length;
   int on;
} tVoice;

tVoice Voice[MAX_VOICE];


tDelay Delay;

extern int PatternIndex;

tDrum         Drum[MAX_DRUMS];
tPattern      Pattern [MAX_PATTERNS];

/* Pattern is our primary source of information. */
/* At least in this application. */

int samplenr=0;
int total_mem=0;
double          tick_duration;


SEQ_DEFINEBUF (2048);

int             seqfd,dspfd;
int             tmp, dev=0;
double          this_time, next_time;
int             ticks_per_division;
double          clock_rate;	/* HZ  What is this.. */


int channel=0;

void            play_drum (int sample);
void            ParseDrums(char *FileName);
void            PlayDrumPattern(void);
void            InitPattern(void);

int Beat=0;  /* This keeps track of the beat */


extern int BPM;
extern int M;

/*
 * The function seqbuf_dump() must always be provided
 * Thats why its here.
 */

void
seqbuf_dump ()
{
  if (_seqbufptr)
    if (write (seqfd, _seqbuf, _seqbufptr) == -1)
      {
	perror ("write /dev/sequencer");
	exit (-1);
      }
  _seqbufptr = 0;
}

#ifndef NOGUS
#include "gusload.c"
/* Jeez this aint good */
#endif



/* Opens the d/a converter for non GUS (Wave table) soundcards */
int OpenDSP(void)  
{
  int             voi, n;
      
  if ((dspfd = open ("/dev/dsp", O_WRONLY, 0)) == -1)
    {
      perror ("/dev/dsp");
      exit (-1);
    }

   
   /* Now this is tricky !! */
   n=0x0003000e;
   if (ioctl (dspfd, SNDCTL_DSP_SETFRAGMENT, &n) == -1)
     {
	perror ("/dev/dsp setfragment size");
	exit (-1);
     }
   /* fprintf(stderr,"After setfrag %d\n",n);   
    */
  
  n=2048;
  if (ioctl (dspfd, SNDCTL_DSP_GETBLKSIZE, &n) == -1)
    {
      perror ("/dev/dsp getblk size");
      exit (-1);
    }
  
  /* fprintf(stderr,"After getblk %d\n",n);   
   */

#ifdef SC16BITS
   n=16;
#else
   n=8;
#endif
   
  if (ioctl (dspfd, SOUND_PCM_WRITE_BITS, &n) == -1)
    {
      perror ("/dev/dsp write bits");
      exit (-1);
    }

   n=0;
   
   if (ioctl (dspfd, SNDCTL_DSP_GETFMTS, &n) == -1)
     {
	perror ("/dev/dsp set formats");
	exit (-1);
     }
   
   
#ifdef SC16BITS
   n=AFMT_S16_LE;
#else
   n=AFMT_U8;   
#endif
  if (ioctl (dspfd, SNDCTL_DSP_SETFMT, &n) == -1)
    {
      perror ("/dev/dsp set formats");
      exit (-1);
    }

#ifdef STEREO
   n=2;
#else
   n=1;   
#endif

  if (ioctl (dspfd, SOUND_PCM_WRITE_CHANNELS, &n) == -1)
    {
      perror ("/dev/dsp write channels");
      exit (-1);
    }

   
#ifdef STEREO
   n=22000;
#else
   n=44000;   
#endif   
   
   if (ioctl (dspfd, SOUND_PCM_WRITE_RATE, &n) == -1)
     {
	perror ("/dev/dsp write rate");
	exit (-1);
    }
   
   for (voi=0;voi<MAX_VOICE;voi++)  {
      Voice[voi].count=-1;
   }
   
 return(0);   
}

/* Opens the sequencer for the GUS */
int OpenSequencer(void)  
{
#ifndef NOGUS

int             i, n;
struct synth_info info;
  if ((seqfd = open ("/dev/sequencer", O_RDWR, 0)) == -1)
    {
      perror ("/dev/sequencer");
      exit (-1);
    }

   /* Nr of synths available */
  if (ioctl (seqfd, SNDCTL_SEQ_NRSYNTHS, &n) == -1)
    {
      perror ("/dev/sequencer");
      exit (-1);
    }

  for (i = 0; i < n; i++)
    {
      info.device = i;

      if (ioctl (seqfd, SNDCTL_SYNTH_INFO, &info) == -1)
	{
	  perror ("/dev/sequencer");
	  exit (-1);
	}

      if (info.synth_type == SYNTH_TYPE_SAMPLE
	  && info.synth_subtype == SAMPLE_TYPE_GUS)
	dev = i;
    
    }

  if (dev == -1)
    {
      fprintf (stderr, "Gravis Ultrasound not detected\n");
      fprintf (stderr, "If you have a GUS MAX fix code in sbdrum.c\n");
      exit (-1);
    }

  GUS_NUMVOICES (dev, 24);

/*   tick_duration = 100.0 / clock_rate;  Whats this for?*/

   ioctl (seqfd, SNDCTL_SEQ_SYNC, 0);
   ioctl (seqfd, SNDCTL_SEQ_RESETSAMPLES, &dev);
	   
   SEQ_START_TIMER();
#endif
   return(0);
}


/*                 
 *      EmptyCard
 * 
 * Removes all loaded samples from the card
 * 
 * 
 *  
 */

int EmptyCard (void)
{
#ifndef NOGUS
int i;
   
   for(i=0;i<MAX_DRUMS;i++)  {
      Drum[i].Loaded=0;
   }
/* TODO fix the memory leakage */
      
   if (ioctl (seqfd, SNDCTL_SEQ_RESETSAMPLES, &dev) == -1)
     perror ("Sample reset Failed!!\n");
#endif
   
}

int
MAIN (int argc, char *argv[])
{
int i;
   
#ifndef NOGUS
   OpenSequencer();
#else
   OpenDSP();
   for (i=0;i++;i<MAX_DELAY) {
     delayData[i]=0;
   }

   DelayPointer=0;
   Delay.Delay16thBeats=230;
   Delay.leftc=0.5;
   Delay.rightc=0.5;
   Delay.pointer=&delayData[MAX_DELAY/2];

#endif
   InitPattern();
   EmptyCard(); /* just to make sure ... */
   for(i=0;i<MAX_DRUMS;i++)  {
      Drum[i].Loaded=0;
   }

   
   ParseDrums("DRUMS");
   
}
void starttimer(void) {

#ifndef NOGUS
   SEQ_START_TIMER();
#endif 

}


unsigned short
intelize (unsigned short v)
{
  return ((v & 0xff) << 8) | ((v >> 8) & 0xff);
}

unsigned long
intelize4 (unsigned long v)
{
  return
  (((v >> 16) & 0xff) << 8) | (((v >> 16) >> 8) & 0xff) |
  (((v & 0xff) << 8) | ((v >> 8) & 0xff) << 16);
}



/* returns how often PlayPattern should be called */
unsigned long GetUpdateIntervall(int PattI)  {
unsigned long l;

#ifdef SC16BITS
   l=(unsigned long)(1000*BL/44000);    
#else
   l=(unsigned long)(1000*BL/22000);    
#endif

   
#ifndef NOGUS
   return ((unsigned long)(1000*240/BPM));    
#else
   PlayPattern(PattI,0);
   return (l);
#endif   				 
}
#ifndef NOGUS
/*                 
 *      load_sample
 * 
 * Load a sample with filename name into patch with pan.
 * 
 *  Return the instrument number 
 * 
 *  If Intel=7 then we want to load a PATCH instead.
 *  We then use the LoadPatch function.
 * 
 * */
int
load_sample (int samplenum,char *name,int pan,int Intel)
{
  int             i, sample_fd;

  int             nr_samples;	/* 16 or 32 samples */
  int             slen, npat;
  char            mname[23];

  
   if (Intel==7) {
      LoadPatch(name,samplenum,dev);
      debug ("Loading GUS patch: %s\n", name); 
      samplenr++;
      return (samplenr-1);
      /*********************************/
   }
	       
   if ((sample_fd = open (name, O_RDONLY, 0)) == -1)
     {
	perror (name);
	return (-1);
     }

   fprintf (stderr, "Loading GUS sample: %s\n", name); 
   
     {
	int             j,len, loop_start, loop_end;
	unsigned short  loop_flags = 0;
	char            pname[22];
	char            tmpdata[SLEN];
	char            tmpdata2[SLEN];
	
	struct patch_info *patch;
	
	len=read(sample_fd, tmpdata, SLEN);

	loop_start = 0;
	loop_end = len;
       
	loop_flags = WAVE_16_BITS ;
	
	if (len > 0)
	  {
	     total_mem += len;
	     
	     patch = (struct patch_info *) calloc (1,(sizeof (*patch) + len));
	     
	     patch->key = GUS_PATCH;
	     patch->device_no = dev;
	     patch->instr_no = samplenum;
	     patch->mode = loop_flags;
	     patch->len = (len);
	     patch->loop_start = loop_start;
	     patch->loop_end = loop_end;
	     /* patch->base_note = 261630;*/	/* Middle C */
	     /*	  patch->base_freq = 4*8448; */
	     patch->base_note = 261630; 	/* Middle C */
	     patch->base_freq = 44000;  
	     patch->low_note = 0;
	     patch->high_note = 20000000;
	     patch->volume = 120;
	     patch->panning = pan;

	     if (Intel==0)  {
      
		for (j=0;j<len -1;)  {
		   tmpdata2[j+1]=(char)(tmpdata[j]); 
		   tmpdata2[j]=(char)(tmpdata[j+1]); 
		   j+=2;
		}
		memcpy(patch->data,tmpdata2,len);
	     } else  {
		memcpy(patch->data,tmpdata,len);
	     }

      	    /* This is confusing!! Fix this len and loop_end stuff */

	     SEQ_WRPATCH (patch, (sizeof (*patch) + len)-2);

	     free (patch);

	  }
    }

  close (sample_fd);
  return (samplenr-1);
}
#else
/*                 
 *      load_sample     NOGUS version!!
 * 
 * Load a sample with filename name into patch with pan pan.
 * 
 *  Return the instrument number 
 * 
 * */
int load_sample (int samplenum,char *name,int pan,int Intel)
{
   int             i,j, sample_fd;
   int             len;
   char            tmpdata[SLEN];
   char            tmpdata2[SLEN];
   
   
   if ((sample_fd = open (name, O_RDONLY, 0)) == -1)
     {
	perror (name);
	return -1;
     }
   i=samplenum;
   
   len=read(sample_fd, tmpdata, SLEN);
   
   /* fprintf(stderr,"len=%d",len); */
   
   assert(len>0);
   assert(len<SLEN);
   
   Drum[i].length=len;
   Drum[i].sample=calloc(len+4,sizeof(short));
   
   if (Intel==0)  {
      
      for (j=0;j<len -1;)  {
	 tmpdata2[j+1]=(char)(tmpdata[j]); 
	 tmpdata2[j]=(char)(tmpdata[j+1]); 
	 j+=2;
      }
      memcpy(Drum[i].sample,tmpdata2,len);
   } else  {
       memcpy(Drum[i].sample,tmpdata,len);
   }
   
   close (sample_fd);
   return (i);
}
#endif

/* Plays a drum */
void
play_drum (int sample)
{
   channel=channel+1;
   if (channel>23) channel=0;
   /* pitch=Drum[sn].key; */
/*   SEQ_STOP_NOTE (dev, channel , 60 , 100); */
   SEQ_SET_PATCH (dev, channel, sample);
   SEQ_START_NOTE (dev, channel , Drum[sample].key , 100);
}

void InitPattern()  {
 int i,j,k;
 
   for(i=0;i<MAX_DRUMS;i++)  {
      Drum[i].Filename=NULL;
   }
   
   for (k=0;k<MAX_PATTERNS;k++)  {
      for (j=0;j<MAX_DRUMS;j++) { 
	 Pattern[k].DrumPattern[j].Drum=NULL;
	 for (i=0;i<16;i++) {
	    Pattern[k].DrumPattern[j].beat[i]=0;
	 }
      }
   }
}

/* prepends drums_root_dir at filename */ 
char *add_root(char *name) 
{ 
  char drum_name[512]; 
  char *e; 
  if ((e=getenv("DRUMS_ROOT")) == NULL) 
     strcpy (drum_name, DRUMS_ROOT_DIR); 
  else 
     strcpy (drum_name, e); 
      
  strcat (drum_name,"/");	/* just in case it ain't present, and double '/' 
				   won't hurt */ 
  strcat (drum_name, name); 
  strcpy (name, drum_name); 
  return name; 
} 

   
/* Parse The Drums file that defines the drums we want to use in this setup */   
void  ParseDrums(char *FileName)
{
   FILE *drum_fd;
   int sn=0,i,Intel,KeyA;
   char buff[512];
   char name[512];
   char filename[512];
   int vol,pan,col;
   

   add_root(strcpy(filename,FileName)); 
   if ((drum_fd = fopen (filename,"r"))==NULL) 
     {
	perror (filename);
	return;
     }
   
   while (fgets(buff,512,drum_fd)) {	
      sscanf(buff,"%s\t%s\t%d\t%d\t%d\t%d\t%d",name,filename,&col,&pan,&vol,&Intel,&KeyA);
      if (filename[0] != '/') {	/* if it is not absolute path */ 
         add_root(filename); 
      }

      /* sn=load_sample(filename,pan,Intel); */
	 i=0;
	 Pattern[i].DrumPattern[sn].Drum=&Drum[sn];
	 Pattern[i].DrumPattern[sn].Drum->SampleNum=sn;
	 Pattern[i].DrumPattern[sn].Drum->Name=(char *)calloc(2+strlen(name),
								 sizeof(char));
	 strcpy(Pattern[i].DrumPattern[sn].Drum->Name,name);
	 Pattern[i].DrumPattern[sn].Drum->Filename=(char *)calloc(2+strlen(filename),sizeof(char));
	 strcpy(Pattern[i].DrumPattern[sn].Drum->Filename,filename);
	 Pattern[i].DrumPattern[sn].Drum->col=col;
	 Pattern[i].DrumPattern[sn].Drum->pan=pan;
	 Pattern[i].DrumPattern[sn].Drum->vol=vol;
         Pattern[i].DrumPattern[sn].Drum->key=KeyA;
	 Pattern[i].DrumPattern[sn].Drum->Filetype=Intel;
	 
	 for(i=1;i<MAX_PATTERNS;i++)  {
	    Pattern[i].DrumPattern[sn].Drum=&Drum[sn];
	 }
      sn++;

   }
   fclose(drum_fd);
}

#ifndef NOGUS

void  PlayPattern(int Patt,int Measure)
{
    double TPB;        /* Ticks per Beat */
    double TUM;    /* Ticks until Measure */
    double DBPM=(double) BPM;
   
   
/*   double TPB=12.5; */       /* Ticks per Beat */
/*   double TPM=TPB*16; */   /* Ticks per Measure */

   int sample;
   int j,i=0;
   
   
   TPB=(double)(1500/DBPM);        /* Ticks per Beat */
   TUM=(double) Measure*(24000/DBPM);          /* Ticks till this Measure */
   
   for (i=0;i<16;i++) {
      SEQ_WAIT_TIME((long)(TUM+(TPB*i)));
      for (j=0;j<MAX_DRUMS;j++) { 
	 if (Pattern[Patt].DrumPattern[j].Drum!=NULL) {
	    if (Pattern[Patt].DrumPattern[j].beat[i]>0)  {
	    play_drum(Pattern[Patt].DrumPattern[j].Drum->SampleNum);
	    }
	 }
      }
   }
   M++;
   SEQ_DUMPBUF ();   
}
int StopPlaying(void)  {

};
#else
/*
 *  WRITE  NOGUS version writes buffer
 * 
 * 
 */
int WRITE(int i) {
int l;
      
      if ((l=write(dspfd,data,BL))<0)  {      
	 perror("Could not write to /dev/dsp!");  
      }
      /*   fprintf(stderr,"Write %d,of %d\n",l,BL); */  
   
      if (l<BL) {
	 fprintf(stderr,"Underflow at write %d,of %d\n",l,BL);
      }
      
}

/*
 *  voice_on  NOGUS version. Starts a voice!!
 * 
 * 
 */
int voice_on(int i) {
   int j=0;  

   while ((Voice[j].count>-1)&&(j<MAX_VOICE))  {
      j++;
   }
	  
   if (j==MAX_VOICE)  {
      fprintf(stderr,"Underflow of voices\n");
   }
	  
   Voice[j].pointer=Drum[i].sample;

   if (Drum[i].Loaded==1) {      
      Voice[j].count=0;
   }
   
   
   Voice[j].length=Drum[i].length;
}


/* Called after push of stop button */
/* FInish playing some of the sound */
int StopPlaying(void)  {
   int j,voi,maxj=0;
   int CrossedLimit=0;
   
   
   for(j=0;j<=BL;j++) {
      tmpstore[j]=0;
   }
   
   for (voi=0;voi<MAX_VOICE;voi++)  {
      if (Voice[voi].count>-1)  {
	 for(j=0;j<=BL;j++) {
	    if (Voice[voi].count>Voice[voi].length)  {
	       if (j>maxj)  {
		  maxj=j;
	       }
	       Voice[voi].count=-1;
	       break;
	    } else  {
	       tmpstore[j]+=*Voice[voi].pointer;
	       Voice[voi].pointer++;
	       Voice[voi].count++;
	    }
	 }
      }
   }
   
   for(j=0;j<maxj;j++) {

#ifdef SC16BITS
      if (tmpstore[j]>32767) {
	 data[BlockCounter+1]=0x7F;
	 data[BlockCounter]=255;
      } else if (tmpstore[j]<-32767)  {
	 data[BlockCounter+1]=0x80;
	 data[BlockCounter]=0;
	 } 
      else {
	 data[BlockCounter+1]=(unsigned char)((tmpstore[j]&0xFF00) >> 8);
	 data[BlockCounter]=(unsigned char)((tmpstore[j]&0x00FF));
      }
      LeftOfBlock-=2;
      BlockCounter+=2;      
#else
      if (tmpstore[j]>32767) data[BlockCounter]=255; else if (tmpstore[j]<-32767) 
      	data[BlockCounter]=0; else data[BlockCounter]=(unsigned char)((tmpstore[j]^0x8000) >> 8);
      LeftOfBlock--;
      BlockCounter++;
#endif
      if (LeftOfBlock<1)  {
	 CrossedLimit=1;
	 WRITE(1);
	 LeftOfBlock=BL;
	 BlockCounter=0;
      }
   
   }

   tmpstore[j]=0; /* the rest is Zero */
  
   while (LeftOfBlock>0)  {

#ifdef SC16BITS
      
      data[BlockCounter+1]=(unsigned char)((tmpstore[j]&0xFF00) >> 8);
      data[BlockCounter]=(unsigned char)((tmpstore[j]&0x00FF));
      LeftOfBlock-=2;
      BlockCounter+=2;      
#else
      data[BlockCounter]=(unsigned char)((tmpstore[j]^0x8000) >> 8);
      LeftOfBlock--;
      BlockCounter++;
#endif
   }
   WRITE(1);
   Beat=0;

   for (j=0;j++;j<MAX_DELAY) {
     delayData[j]=0;
   }   
return (CrossedLimit);
}
   
/* This will add one/4 beat to the sample to be played */
/* Fo the NOGUS version                                */

int add_one_beat(int LENGTH)  {
   int j,voi,Soundlength,Soundlength2;
   int CrossedLimit=0;
   
   for(j=0;j<=LENGTH;j++) {
      tmpstore[j]=0;
   }
   
/* TODO optimize the major loop */
/* Get rid of the above use calloc memset or something !*/
/* This part should be optimized for better performance !! */   
   
   for (voi=0;voi<MAX_VOICE;voi++)  {
      if (Voice[voi].count>-1)  {
	 Soundlength=Voice[voi].length-Voice[voi].count;
	 if (Soundlength>LENGTH)  {
	    Soundlength=LENGTH;
	 }
	 if (Soundlength==0)  {
	    Voice[voi].count=-1;  
	 } else {
	    for(j=0;j<=Soundlength;j++) {
	       tmpstore[j]+=*Voice[voi].pointer;
	       Voice[voi].pointer++;
	    }
	 }
	Voice[voi].pointer--; 
	 
	 if (Soundlength==LENGTH)  {
	    Voice[voi].count+=Soundlength;
	 } else  {
	    /* fprintf(stderr,"[%d]",*(Voice[voi].pointer--)); */
	    Voice[voi].count=-1;  
	 }
      }	    
   }

if (Delay.On==1) {
  if (DelayPointer-(Delay.Delay16thBeats/16)*LENGTH>0) { 
    Delay.pointer=(short *)&delayData[DelayPointer-
				     (Delay.Delay16thBeats/16)*LENGTH];
  }
}


   
   for(j=0;j<=LENGTH;j++) {

#ifdef SC16BITS
if (Delay.On==1) {

  tmpstore[j]+=(long)((float)Delay.leftc*(float)(*Delay.pointer));
  Delay.pointer++;
  
  if (Delay.pointer>=(short *)&delayData[MAX_DELAY]) {
    Delay.pointer=(short*)&delayData[0];
  }

} else {
  Delay.pointer++;

  if (Delay.pointer>=(short *)&delayData[MAX_DELAY]) {
    Delay.pointer=(short*)&delayData[0];
  }
}
#endif




      
#ifdef SC16BITS
      /* TODO, Hey.. Do this better as well */
      if (tmpstore[j]>32766) {
	 data[BlockCounter+1]=0x7F;
	 data[BlockCounter]=255;
      } else if (tmpstore[j]<-32767)  {
	 data[BlockCounter+1]=0x80;
	 data[BlockCounter]=0;
	 } 
      else {
	 data[BlockCounter+1]=(unsigned char)((tmpstore[j]&0xFF00) >> 8);
	 data[BlockCounter]=(unsigned char)((tmpstore[j]&0x00FF));
      }

      delayData[DelayPointer]=*(short *)&data[BlockCounter];
      DelayPointer=(DelayPointer+1)%MAX_DELAY;

      LeftOfBlock-=2;
      BlockCounter+=2;      
#else
      if (tmpstore[j]>32767) data[BlockCounter]=255; else if (tmpstore[j]<-32767) 
      	data[BlockCounter]=0; else data[BlockCounter]=(unsigned char)((tmpstore[j]^0x8000) >> 8);
      LeftOfBlock--;
      BlockCounter++;
#endif
      if (LeftOfBlock<1)  {
	 CrossedLimit=1;
	 WRITE(1);
	 LeftOfBlock=BL;
	 BlockCounter=0;
      }
   
   }

return (CrossedLimit);
}

/* Play pattern for no GUS soundcards */
void  PlayPattern(int Patt,int Measure)
{
   double TPB;                           /* Ticks per Beat */
   double TUM;                           /* Ticks until Measure */
   double DBPM=(double) BPM;
   int j,i=0;
   int samplelen;
   int CrossedLimit,NextPatt;
   static int RememberPatt;   

   if (Beat>0)  {
      NextPatt=Patt;
      Patt=RememberPatt;
   }
	 
   TPB=(double)(1500/DBPM);                    /* Ticks per Beat */
   TUM=(double) Measure*(24000/DBPM);          /* Ticks till this Measure */
   /* samplelen=330000/BPM;                        Length of 1/4 beat */
   samplelen=660000/BPM;                       /* Length of 1/4 beat */
   
   for (i=Beat;i<16;i++) {
      for (j=0;j<MAX_DRUMS;j++) { 
 	 if (Pattern[Patt].DrumPattern[j].Drum!=NULL) {
	    if (Pattern[Patt].DrumPattern[j].beat[i]>0)  {
	       voice_on(Pattern[Patt].DrumPattern[j].Drum->SampleNum);
	    }
	 }
      }
      
      Beat++;
      CrossedLimit=add_one_beat(samplelen+1);
      if (CrossedLimit==1)  {
	 break;
      }	 
   }
   if (Beat>15)  {
      Beat=0;
      M++;
      Patt=NextPatt;
   }
   
   RememberPatt=Patt; 
}

#endif



