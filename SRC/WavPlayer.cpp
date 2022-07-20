/*
 *              (C) Olof Astrand, 1997
 *      See README file for details.
 *
 *      $Date: 1996/03/23 20:16:49 $
 *      $Revision: 1.5 $
 * 
 */

//
// Only first part of Data is used....
// That is rather stupid..
//

/* Leave the rest */




#include <stdio.h>
#include <math.h>
#include <string.h>
#include <malloc.h>

#include "drum.h"
#include <assert.h>
#include <windows.h>
#include "tb303.h"
#include <mmsystem.h>

#include "player.h"
#include "wav.h"

Player* Player::mInstance;

HWAVEOUT phwo;


HANDLE hEvent;

int nextWrite=1;


struct tVoice {

		short *pointer;
		int count;
		float leftc;
		float rightc;
	};


tVoice Voice[MAX_VOICE];


int L_MAX_DRUMS=MAX_DRUMS;
HWND myhWnd;


WAVEHDR pwh1;
WAVEHDR pwh2;


extern struct TB myTB;

static int TBOn=0;

void Player::enableTB(){
   TBOn=1;
   StartPlayer();

}


void Player::disableTB(){
  TBOn=0;

}

int Player::getPlayPos() {

	MMTIME time;

	time.wType=TIME_SAMPLES;

	waveOutGetPosition(phwo,&time,sizeof(time));

	return (time.u.sample);
};



#define BL 100000 

int TheFirstBuffer=1;

int LeftOfBlock=BL;
int BlockCounter=0;

DWORD	BufferLength=BL;
DWORD	PlayId;
DWORD	MyWritePos=0;
DWORD	StartTime;


#define MIN(a, b)		((a) < (b) ? (a) : (b))


#define SLEN 300000


short			data[BL];
long            tmpstorel[11000];
long            tmpstorer[11000];


tDelay Delay;


extern int PatternIndex;

tDrum Drum[MAX_DRUMS];
extern tPattern      Pattern [MAX_PATTERNS];

/* Pattern is our primary source of information. */
/* At least in this application. */

int Playing=0;
int samplenr=0;
 

int             tmp;
double          this_time, next_time;
int channel=0;
int Beat=0;  /* This keeps track of the beat */

extern int BPM;
/* OLAAS TODO 
extern int BPM;
extern int M;
*/

int  PlayPattern(int Patt);
int OpenBuffer(void);

//
// Initialize Voices
//
void InitVoice() {
	int j=0;

	while (j<MAX_VOICE)  
		{
			Voice[j].count=-1;
			j++;
		}
}

DWORD PlayThread(LPVOID param) {
	TheFirstBuffer=1;

//	while (PlayPattern(PatternIndex)==0) ;;;
//	FillPattern(PatternIndex);

	while(Playing==1) {
		PlayPattern(PatternIndex);
	}
 return(0);
}

extern "C" {
void CALLBACK myWaveOutProc(  HWAVEOUT hwo, UINT uMsg,  DWORD dwInstance,    DWORD dwParam1,      DWORD dwParam2     )
{

// 
//	Playing=1;


	switch (uMsg)
	{
		
	case WOM_DONE:
		if (nextWrite==1) 
		{
			nextWrite=2;
		} else {
			nextWrite=1;
		}

		SetEvent(hEvent);

		break;
		
	default:
		break;
	}


}
} /* end extern */

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

	int i,voi;
	for(i=0;i<MAX_DRUMS;i++)  {
		Drum[i].Loaded=0;
		Drum[i].sample=(short *)NULL;
	}

	for (voi=0;voi<MAX_VOICE;voi++)  {
		Voice[voi].count=-1;
	}

/* Todo Fix Memory Leakage */   
return(1);
}

void InitPattern(void);


int Player::InitPlayer(HWND hWnd)
{
	WAVEFORMATEX  WF;
	MMRESULT ret;

      
	DWORD dwCallbackInstance;  

	Delay.leftc=0.5;
	Delay.rightc=0.5;
	Delay.count=LeftOfBlock/2;
//	Delay.On=1;
	Delay.pointer=&data[BlockCounter];
              
	myhWnd=hWnd;
 
   InitPattern();
   EmptyCard(); 
/* Just to make sure ... */
	
 

	/* Set Format properties  */
	WF.wFormatTag=WAVE_FORMAT_PCM;
	WF.nChannels=2;
	WF.nSamplesPerSec=44100;
	WF.nAvgBytesPerSec=44100*4;
	WF.nBlockAlign=4;
	WF.wBitsPerSample=16;
	WF.cbSize=0;


	hEvent=CreateEvent(NULL,false,false,"DonePlaying");

	
	ret=waveOutOpen(
		&phwo,           
		WAVE_MAPPER ,            
		&WF,       
		(unsigned long)myWaveOutProc/*DWORD dwCallback*/,          
		NULL /*DWORD dwCallbackInstance*/,  
		CALLBACK_NULL | CALLBACK_FUNCTION               
		);


if (ret!=MMSYSERR_NOERROR ) {
	char Text[1024];

	waveOutGetErrorText( ret,Text,1024 );
   
	MessageBox(myhWnd,Text,"WaveOutOpen error",MB_OK);
}



	pwh1.lpData=(char *)&data[0];
	pwh1.dwBufferLength=100000;
	pwh1.dwBytesRecorded; 
    pwh1.dwUser=NULL;
//	pwh1.dwFlags=WHDR_BEGINLOOP ;     
	pwh1.dwLoops=0;
	//pwh1.lpNext=&pwh2;


	pwh2.lpData=(char *)&data[50000];
	pwh2.dwBufferLength=100000;
	pwh2.dwBytesRecorded; 
    pwh2.dwUser=NULL;
//	pwh2.dwFlags=WHDR_BEGINLOOP ;     
	pwh2.dwLoops=0;
	//pwh2.lpNext=&pwh1;



	ret=waveOutPrepareHeader( phwo,&pwh1,sizeof(pwh1));

	if (ret!=MMSYSERR_NOERROR ) {
		char Text[1024];

		waveOutGetErrorText( ret,Text,1024 );
   
		MessageBox(myhWnd,Text,"WaveOutPrepareHeader 1 error",MB_OK);
	}

	ret=waveOutPrepareHeader( phwo,&pwh2,sizeof(pwh1));

	if (ret!=MMSYSERR_NOERROR ) {
		char Text[1024];

		waveOutGetErrorText( ret,Text,1024 );
   
		MessageBox(myhWnd,Text,"WaveOutPrepareHeader 2 error",MB_OK);
	}
 



return(1);
}


int Player::CleanUpPlayer(void) {

	Playing=0;
	Sleep(1000);

	return (1);
}

int Player::StartPlayer(void) {
	HANDLE hTR;

	if (Playing==1) {
		Sleep(1000); // Wait for Play Thread to die
	}
	Playing=1;

	InitVoice();
	MyWritePos=0;
	Beat=0;
	LeftOfBlock=BL;
	BlockCounter=0;


	hTR=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE) PlayThread,
					(LPVOID) 0,0,&PlayId);
	
	
	return (1);
}



int Player::StopPlayer(void){
	Playing=0;	
	Sleep(1000);
	return(1);	
};




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


   l=(unsigned long)(1000*BL/44000);    
   
//   PlayPattern(PattI,0);
   return (l);   				 
}



/*                 
 *      LoadSequence    
 * 
 * To remember the sequence we have created.
 * 
 *  
 * 
 * */
int LoadSequence (int sampnum,short *s,int Length)
{
  int i=sampnum;

   
	  Drum[i].Loaded=1;
	  Drum[i].sample=s;
	  
      Drum[i].length=Length;
	  Drum[i].SampleNum=i;
	  Drum[i].col=6;
	  Drum[i].pan=0;
	  Drum[i].vol=200;  // vol
	  Drum[i].key=64;



	  for(i=0;i<MAX_PATTERNS;i++)  {
			Pattern[i].DrumPattern[sampnum].Drum=(pDrum)&Drum[sampnum];
	  }


	  return(1);
}







/*                 
 *      LoadSample    
 * 
 * Load a sample with filename name into patch with pan.
 * 
 *  Return the instrument number 
 * 
 * */
int Player::LoadSample (int samplenum,char *name,int pan,int Type)
{
   FILE			*sample_fd;
   int             i,j;
   int             len,totLen;
   char            tmpdata[SLEN];
   char            tmpdata2[SLEN];


   if (strcmp(name,"-#-")==0) {
	  // Look for the drum where data is loaded..... 
	   // We only Load data once 
	 j=samplenum;
	 
//	 Drum[sn].Name

     i=samplenum;

     Drum[i].Loaded=1;


   };

 
#if 0
   if (strcmp(name,"3MU")==0) {
	  int D;
//	  Drum[i].sample=(short *)
	  Drum[i].Loaded=1;
//	  Drum[i].sample=Test3MU(&D);

	  

      Drum[i].length=D;



   	  return (1);
   };
#endif


   if ((sample_fd = fopen (name, "rb")) == NULL)
     {
		return -1;
     }

   if (Type==17) {
	   dword flags;
	   dword dataposition;
	   dword datasize;

		LoadWav(sample_fd, &flags,&dataposition, &datasize);
		
		// Only accept 16-bit 44100kHz data
		if (flags && S3DF_16bit!=0) {
			fseek(sample_fd,dataposition,SEEK_SET);
			fread(&tmpdata,datasize,sizeof(short),sample_fd); 
		}

		i=samplenum;
		totLen=datasize;

//		for (j=0;j<totLen-1;j++)  {
//			tmpdata[j]=(short)(tmpArr[j]); 
//		}


   } else {



	i=samplenum;
	len=1;
	totLen=-1;
	while (len>0)		
	{
			totLen+=len;
			len=fread(&tmpdata[totLen],sizeof(char),SLEN-1,sample_fd);
 			if(len>SLEN-2) 
			{
				totLen=SLEN-1;
				break;
			}
	}   
   }

   // fprintf(stderr,"len=%d",len);
   
   assert(totLen>0);
   if (totLen>SLEN) totLen=SLEN-1;
   assert(totLen<SLEN);
   
   Drum[i].length=totLen;
   if (Drum[i].sample==NULL) {
	   Drum[i].sample=(short *)calloc(totLen+4,sizeof(short)); 

	   if (Type==0)  {
			for (j=0;j<totLen-1;)  {
				tmpdata2[j+1]=(char)(tmpdata[j]); 
				tmpdata2[j]=(char)(tmpdata[j+1]); 
				j+=2;
			}
			memcpy(Drum[i].sample,tmpdata2,totLen);
		} else  {
   			for (j=0;j<totLen-1;)  {
				tmpdata2[j]= (char)(tmpdata[j]) ^ 0x80 ;
				j+=2;
			}
  			memcpy(Drum[i].sample,tmpdata,totLen);
		}
   }

   Drum[i].Loaded=1;

   fclose (sample_fd);
   return (1);
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

   
/* Parse The Drums file that defines the drums we want to use in this setup */   
void  Player::ParseDrums(char *FileName)
{
	FILE *drum_fd;
	int sn=0,i,Intel,KeyA;
	char buff[512];
	char name[512];
	char filename[512];
	int vol,pan,col;
   
	if ((drum_fd = fopen ("DRUMS.INI","r"))==NULL) {
		if ((drum_fd = fopen ("c:\\WINDRUM\\DRUMS.INI","r"))==NULL) {
			return;
		}
	}

while (fgets(buff,512,drum_fd)) {	
	sscanf(buff,"%s\t%s\t%d\t%d\t%d\t%d\t%d",name,filename,&col,&pan,&vol,&Intel,&KeyA);
	if (strcmp(name,"#")!=0) {
	
		i=0;
		Pattern[i].DrumPattern[sn].Drum=(pDrum)&Drum[sn];
		Drum[sn].SampleNum=sn;
		Drum[sn].Name=(char *)calloc(2+strlen(name),
								 sizeof(char));
		strcpy(Drum[sn].Name,name);
		Drum[sn].Filename=(char *)calloc(2+strlen(filename),sizeof(char));
		strcpy(Drum[sn].Filename,filename);
		Drum[sn].col=col;
		Drum[sn].pan=pan;
		Drum[sn].vol=128;  // vol
		Drum[sn].key=KeyA;
		Drum[sn].Filetype=Intel;
	 
		for(i=1;i<MAX_PATTERNS;i++)  {
			Pattern[i].DrumPattern[sn].Drum=&Drum[sn];
		}
		sn++;
	}
}

fclose(drum_fd);
}




/*
 *  WRITE , Check Play Position and moves dat
 * 
 * 
 */


int WRITE(int i) {
	void	*pbData=NULL;
	void	*pbData2=NULL;
	DWORD	dwLength;
	DWORD	dwLength2;
	DWORD	PlayPos,WritePos;
	static DWORD	LastPlayPos=0;
	HRESULT dsRV;
	static DWORD BytesAhead=0;
	static DWORD LastTime=StartTime;
	static struct {
		DWORD   Ticks;
		DWORD   Read;
		DWORD	Write;
		DWORD	MyPlay;
		DWORD   MWP;
		DWORD	BA;
		BYTE	*Data;
	}LastPos[600];
	static int j=0;
	DWORD LengthOfWrite;


	static int TheFirstWrite=0;
	static FILE *ofd;
	int len;


	if (TheFirstWrite==0) {
		
	 if ((ofd = fopen ("test.pcm", "wb")) == NULL)
     {
		return -1;
     }

	}

	
// Use BL for each Lock size call...



	
	LastPos[j].Read=PlayPos;
	LastPos[j].Ticks=GetTickCount()-LastTime;
	LastPos[j].MyPlay=(DWORD )88*(GetTickCount()-StartTime);
	LastPos[j].MWP=MyWritePos;
	LastPos[j].Write=WritePos*2;
	LastPos[j].BA=BytesAhead;

//	PlayPos=(unsigned long)&data[0];
//	WritePos=(unsigned long)&data[44100];


	if (j==100) {
		j=0;
	};

	MMRESULT ret;

	if (i==1) {
		ret=waveOutWrite(  phwo,&pwh1,sizeof(pwh1)); 
	} else {
		ret=waveOutWrite(  phwo,&pwh2,sizeof(pwh2)); 
	}

	if (TheFirstWrite==0) 
	{
		TheFirstWrite++;
		nextWrite=2;
		ResetEvent(hEvent);

	} 
	else 
	{
		WaitForSingleObject(hEvent,INFINITE);
	}

	if (ret!=MMSYSERR_NOERROR ) {
		char Text[1024];

		waveOutGetErrorText( ret,Text,1024 );
   
		MessageBox(myhWnd,Text,"WaveOutWrite error",MB_OK);
	}


//	Sleep(1500);
      
	return(1);
}

/*
 *  voice_on  Starts a voice!!
 * 
 * 
 */
int voice_on(int i) {
   int j=0;
//   short Test;
   assert(i<MAX_DRUMS);
   assert(i>=0);


   while ((Voice[j].count>0)&&(j<MAX_VOICE))  {
      j++;
   }
	  
   if (j==MAX_VOICE)  {
	   return(-1);
//      fprintf(stderr,"Underflow of voices\n");
   }

   Voice[j].leftc=(float) ((float)Drum[i].vol/(float)128)*( (double) (128-Drum[i].pan)/256.0);
   Voice[j].rightc=(float)((float)Drum[i].vol/(float)128)*( (double) (128+Drum[i].pan)/256.0);

   Voice[j].pointer=Drum[i].sample;

   if (Drum[i].Loaded==1) {      
      Voice[j].count=Drum[i].length;
   } else {
	  Voice[j].count=-1;
   }
   //Test=*Voice[j].pointer;

return(1);
}

//
// add_one_beat
//
/* This will add one/4 beat to the sample to be played */
/* Fo the NOGUS version                                */

int add_one_beat(int LENGTH)  {

int j,voi,Soundlength,Soundlength2;
int CrossedLimit=0;
short TBBuffer[65000];
static short *nisse;

memset( &tmpstorel,0,(3+LENGTH)*sizeof(long));
memset( &tmpstorer,0,(3+LENGTH)*sizeof(long));

if (TBOn==1) {

   FX_TB303(NULL, TBBuffer, LENGTH, &myTB);

   Soundlength=LENGTH;
   if (Soundlength>0)  {
       for(j=0;j<=Soundlength;j++) {
			tmpstorel[j]+=(long)((double)1.0*(float)(TBBuffer[j]));
			tmpstorer[j]+=(long)((double)1.0*(float)(TBBuffer[j]));
	   }

   }
}
   
for (voi=0;voi<MAX_VOICE;voi++)  {
	Soundlength=MIN(LENGTH,Voice[voi].count);
	if (Soundlength>0)  {
		for(j=0;j<=Soundlength;j++) {
//			tmpstorel[j]+=(long)((double)Voice[voi].leftc*(double)(*Voice[voi].pointer));
//			tmpstorer[j]+=(long)((double)Voice[voi].rightc*(double)(*Voice[voi].pointer));
			tmpstorel[j]+=(long)(*Voice[voi].pointer);
			tmpstorer[j]+=(long)(*Voice[voi].pointer);

			Voice[voi].pointer++;
		}
		Voice[voi].count-=Soundlength+1;
	} 
}  



if (Delay.On==1) {
	if (BlockCounter-(Delay.Delay16thBeats/16)*LENGTH-2>0) { // -2*LENGTH
	nisse=Delay.pointer;
	nisse=Delay.pointer=(short *)&data[BlockCounter-(Delay.Delay16thBeats/16)*LENGTH-2];
	} else {
	nisse=Delay.pointer;
//	Delay.pointer++;
//	Delay.pointer++;
//	Delay.pointer=&data[BufferLength+BlockCounter-2*LENGTH-2];
	nisse=Delay.pointer;
	}
	Soundlength=MIN(LENGTH,Delay.count);
	Soundlength2=LENGTH-Soundlength;
	Soundlength=LENGTH;
	for(j=0;j<=Soundlength;j++) {
	
		Delay.pointer++;
		
		tmpstorel[j]+=(long)((float)Delay.leftc*(float)(*Delay.pointer));

	if (Delay.pointer>=(short *)&data[BufferLength/2]) {
		Delay.count=BufferLength/2;
    	Delay.pointer=&data[0];

    }
			tmpstorer[j]+=(long)((float)Delay.rightc*(float)(*Delay.pointer));
	
//  			tmpstorer[j]+=(long)(*Delay.pointer);

			Delay.pointer++;


	if (Delay.pointer>=(short *)&data[BufferLength/2]) {
		Delay.count=BufferLength/2;
    	Delay.pointer=&data[0];

    }



//			tmpstorel[j]+=(*Delay.pointer);
//			Delay.pointer++;
//			tmpstorer[j]+=(*Delay.pointer);
//			Delay.pointer++;
	
		}
		Delay.count-=Soundlength;
	}
#ifdef TEST 
	if (Soundlength2>0)  {
	Delay.count=BufferLength/2;
	Delay.pointer=&data[0];

		for(j=Soundlength;j<=LENGTH;j++) {
			tmpstorer[j]+=(long)(*Delay.pointer);
//	   		tmpstorer[j]+=(long)((float)Delay.rightc*(float)(*Delay.pointer));
			Delay.pointer++;

//			tmpstorel[j]+=(long)((float)Delay.leftc*(float)(*Delay.pointer));
			Delay.pointer++;

//			tmpstorel[j]+=*Delay.pointer;
//			Delay.pointer++;
//			tmpstorer[j]+=*Delay.pointer;
//			Delay.pointer++;

		
		}
		Delay.count-=Soundlength2;
}

if (Delay.count<=0) {
		Delay.count=BufferLength/2;
    	Delay.pointer=&data[0];

}
#endif

for(j=0;j<LENGTH;j++) {

//	while ((TheFirstBuffer==0)&&((int)2*(BlockCounter)==(int)MyWritePos)) {
//			WRITE(1);
//			Sleep(2);
//	}

	   

	/*
	Test of remove clipping.
	Noice might come when makeing Stereo from a mono sound....
    */
	//data[BlockCounter]=MIN(tmpstorel[j],0x7FFF);
	// data[BlockCounter]=tmpstore[j];
 

//	for (int channel=0;channel<2; channel++) {
		
		if (tmpstorel[j]>(long)32766)  {
			data[BlockCounter]=32767;
		} else {
			data[BlockCounter]=tmpstorel[j];
		}
		
		if (tmpstorel[j]<-32767)  {
			data[BlockCounter]=-32767;
		} 
		BlockCounter+=1;      

		if (tmpstorer[j]>(long)32766)  {
			data[BlockCounter]=32767;
		} else {
			data[BlockCounter]=tmpstorel[j];
		}
		
		if (tmpstorer[j]<-32767)  {
			data[BlockCounter]=-32767;
		} 
		BlockCounter+=1;      


//	}

	LeftOfBlock-=2;


    if (BlockCounter==50000)  {
	   WRITE(1);
	}

    if (BlockCounter==100000)  {
  	   WRITE(2);
	}



    if (BlockCounter==100000)  {
		CrossedLimit=1;
		LeftOfBlock=BL;
		BlockCounter=0;
    }
   
}

return (CrossedLimit);
}


//
// PlayPattern
//
// returns 1: if Buffer is full.
// returns -1 if failed
//
/* Play pattern for no GUS soundcards */
int  PlayPattern(int Patt)
{

   double DBPM=(double) BPM;
   int j,i=0;
   int samplelen;
   int CrossedLimit,NextPatt;
   static int RememberPatt;   

	if (Beat>0)  {
		NextPatt=Patt;
		Patt=RememberPatt;
	}
	 
   samplelen=660000/BPM;                       /* Length of 1/4 beat */
   
   for (i=Beat;i<16;i++) {
	 samplelen=660000/BPM; 
      for (j=0;j<L_MAX_DRUMS;j++) { 
	 	 if (Pattern[Patt].DrumPattern[j].Drum!=NULL) {
		    if (Pattern[Patt].DrumPattern[j].beat[i]>0)  {
			   voice_on(Pattern[Patt].DrumPattern[j].Drum->SampleNum);
			}
		}
	}
      
    Beat++;
    CrossedLimit=add_one_beat(samplelen+1);
//     if (CrossedLimit==1)  {
//		break;
//    }	 
   }
   if (Beat>15)  {
      Beat=0;
      Patt=NextPatt;
   }
   
   RememberPatt=Patt; 
return(CrossedLimit);
}



