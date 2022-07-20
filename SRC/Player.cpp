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
#ifdef DEBUG
#define debug(f,x)        fprintf(stderr,f,x);
#else
#define debug(f,x)        {  }
#endif

#define MAX_VOICE    32


#include <stdio.h>
#include <math.h>
#include <string.h>
#include <malloc.h>

#include "drum.h"
#include <assert.h>
#include <windows.h>
#include <dsound.h>
#include "tb303.h"

/* Load-Wav stuff */
#define S3DF_STEREO 0x10
#define S3DF_8bit	0x20
#define S3DF_16bit	0x40


#define S3DF_8000	0x01
#define S3DF_11025	0x02
#define S3DF_22050	0x04
#define S3DF_44100	0x08

#define int16 short int
#define dword DWORD
int L_MAX_DRUMS=MAX_DRUMS;




// Listing 1 - LoadWav


int16 LoadWav(FILE *wav, dword *flags, dword *dataposition, dword *datasize) {

*flags=0;
*dataposition=0;
*datasize=0;

DWORD riffsignature, chunkid, chunksize, nextseek, filesize, hd32;
int16 hd16;

	fread(&riffsignature,1,sizeof(riffsignature),wav);
	// Make sure is a RIFF file
	if(riffsignature!=0x46464952)  return(-1); // RIFF

		// Get the size of the first chunk, not including the chunk size and id.
	fread(&chunksize,1,sizeof(filesize),wav);
	// Get the chunk id, make sure it is a WAV file.
	fread(&chunkid,1,sizeof(chunkid),wav);

	if(chunkid!=0x45564157)  return(-1 /*S3DE_BADTRACK*/); // WAVE

	filesize=chunksize+sizeof(riffsignature)+sizeof(chunksize);

	
	nextseek=sizeof(chunksize)+sizeof(riffsignature)+sizeof(chunkid);

	while(nextseek<filesize) {
      fseek(wav,nextseek,SEEK_SET);
      if (fread(&chunkid,1,sizeof(chunkid),wav)!=sizeof(chunkid)) return(-1);
// TODO what is buf for?
      if (fread(&chunksize,1,sizeof(chunksize),wav)!=sizeof(chunksize)) 
           return(-1);
      switch(chunkid) {
          case 0x20746D66: {//Format Chunk
              fread(&hd16,1,sizeof(hd16),wav);
               if (hd16!=0x0001) return(-1);    // Only accept PCM WAVE data
              fread(&hd16,1,sizeof(hd16),wav);  // Number of channels
              if (hd16>2) return(-1);
              if (hd16==2) *flags|=S3DF_STEREO;
              // Samples per second - values seem to vary from exact values 
              fread(&hd32,1,sizeof(hd32),wav);         // so accept ranges.
              if (hd32<=10512) *flags|=S3DF_8000;
              else if ((hd32>10512) && (hd32<=16537)) *flags|=S3DF_11025;
              else if ((hd32>16537) && (hd32<=33075)) *flags|=S3DF_22050;
              else *flags|=S3DF_44100;
              fread(&hd32,1,sizeof(hd32),wav); // Throughput bytes per second
              fread(&hd16,1,sizeof(hd16),wav); // Block alignment
              fread(&hd16,1,sizeof(hd16),wav); // Bits per sample
              if (hd16==8) *flags|=S3DF_8bit;
              else if (hd16==16) *flags|=S3DF_16bit;
              else return(-1);
              break;
          };
          case 0x61746164: {//Data Chunk
              *dataposition=nextseek+sizeof(chunksize)+sizeof(chunkid);
              *datasize=chunksize;
              break;
          };
          default: break;
    }
    nextseek=chunksize+nextseek+sizeof(chunkid)+sizeof(chunksize);
}
if ((*flags==0) || (*dataposition==0) || (*datasize==0)) return(-1);
return(0);
};





extern struct TB myTB;

static int TBOn=0;

void enableTB(){
   TBOn=1;
   StartPlayer();

}


void disableTB(){
  TBOn=0;

}




// Direct Sound stuff
LPDIRECTSOUND           lpDS = NULL;
LPDIRECTSOUNDBUFFER     lpDSB = NULL;

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

int BlockReady;

//char            data[BL];
short			data[BL];
long            tmpstorel[11000];
long            tmpstorer[11000];

tDelay Delay;

typedef struct  {
   short *pointer;
   int count;
   float leftc;
   float rightc;
} tVoice;

tVoice Voice[MAX_VOICE];

extern int PatternIndex;

tDrum         Drum[MAX_DRUMS];
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
int  FillPattern(int Patt);
int OpenBuffer(void);

//
// Initialize Voices
//
void InitVoice(void){
   int j=0;

   while (j<MAX_VOICE)  {
	   Voice[j].count=-1;
	   j++;
   }
	Delay.leftc=0.5;
	Delay.rightc=0.5;
	Delay.count=LeftOfBlock/2;
//	Delay.On=1;
	Delay.pointer=&data[BlockCounter];
	  
}



DWORD PlayThread(LPVOID param) {
	HRESULT dsRV;

	TheFirstBuffer=1;

	if (lpDSB==NULL) {
		return(0);
	}

//	while (PlayPattern(PatternIndex)==0) ;;;
	FillPattern(PatternIndex);

	dsRV=lpDSB->SetCurrentPosition(0);
	dsRV=lpDSB->Play(0,0,DSBPLAY_LOOPING);

	while(Playing==1) {
		// Sleep(20);
//		dsRV=lpDSB->GetCurrentPosition(&PlayPos,&WritePos);
//		while ((MyWritePos+BL)%BufferLength>PlayPos) {
//		   Sleep(1);
//		   dsRV=lpDSB->GetCurrentPosition(&PlayPos,&WritePos);
//		}
//		PlayPattern(PatternIndex);
//		PlayPattern(PatternIndex);
		PlayPattern(PatternIndex);
	}
   dsRV=lpDSB->Stop();
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

int InitPlayer(HWND hWnd)
{

int i;
HRESULT dsRV;
 
   InitPattern();
   EmptyCard(); 
/* Just to make sure ... */
	

dsRV = DirectSoundCreate( NULL, &lpDS, NULL );

if(dsRV!=DS_OK)  {
	lpDS = NULL;
	return (-1);
}

lpDS->SetCooperativeLevel(hWnd,DSSCL_NORMAL);   
//dsRV=lpDS->SetCooperativeLevel(hWnd,DSSCL_WRITEPRIMARY);

if (dsRV!=DS_OK) {
	lpDSB=NULL;
	return(-1);
}

if (OpenBuffer()<0) {
	return(-1);
}

   
return(1);
}


int CleanUpPlayer(void) {

	Playing=0;
	Sleep(1000);

	if (lpDSB!=NULL) {
		lpDSB->Release();
		lpDSB=NULL;
	}

	if (lpDS!=NULL) {
		//	IDirectSound_Release(lpDS);
		// lpDS->lpVtbl->Release(lpDS);
		lpDS->Release();
		lpDS=NULL;
	}
return (1);
}

int StartPlayer(void) {
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
	

//	if (hTR!=NULL) {
//		SetThreadPriority(hTR,THREAD_PRIORITY_TIME_CRITICAL); 
//	}	

	
	return (1);
}


int OpenBuffer() {	
	DSBUFFERDESC	dsbd;	
	HRESULT dsRV;
	PCMWAVEFORMAT  WF;

	if (lpDS==NULL) {
		return (-1);
	}

	if (lpDSB!=NULL) {
		lpDSB->Release();
		lpDSB=NULL;
	}


	memset( &WF,0,sizeof(PCMWAVEFORMAT));
	memset( &dsbd, 0, sizeof( DSBUFFERDESC ));
	dsbd.dwSize = sizeof( DSBUFFERDESC );
	dsbd.dwFlags =  DSBCAPS_CTRLVOLUME | DSBCAPS_STICKYFOCUS ;

	dsbd.dwBufferBytes = BufferLength;

	/* Set Format properties  */
	WF.wf.wFormatTag=WAVE_FORMAT_PCM;
	WF.wf.nChannels=2;
	WF.wf.nSamplesPerSec=44100;
	WF.wf.nAvgBytesPerSec=44100*4;
	WF.wf.nBlockAlign=4;
	WF.wBitsPerSample=16;
	// WF.cbSize=0;

	dsbd.lpwfxFormat =(LPWAVEFORMATEX) &WF;
	// wiWave.pwfx;
	// CreateSoundBuffer(&dsbd,&l
	dsRV=lpDS->CreateSoundBuffer( &dsbd,
                              &lpDSB,
                              NULL );
	if (dsRV!=DS_OK) {
		lpDSB=NULL;
		return(-1);
	}

	lpDSB->SetFrequency(44100);
	//lpDSB->SetVolume(0);

return(1);
}


//
// This tries to open the primary buffer!!!!
//

int OpenPrimBuffer() {	
	DSBUFFERDESC	dsbd;	
	HRESULT dsRV;
	PCMWAVEFORMAT  WF;

	if (lpDS==NULL) {
		return (-1);
	}

	if (lpDSB!=NULL) {
		lpDSB->Release();
		lpDSB=NULL;
	}


	memset( &WF,0,sizeof(PCMWAVEFORMAT));
	memset( &dsbd, 0, sizeof( DSBUFFERDESC ));
	dsbd.dwSize = sizeof( DSBUFFERDESC );
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER ;
	// |  DSBCAPS_CTRLDEFAULT | DSBCAPS_STICKYFOCUS 
	dsbd.dwBufferBytes = 0;

	/* Set Format properties  */
	WF.wf.wFormatTag=WAVE_FORMAT_PCM;
	WF.wf.nChannels=2;
	WF.wf.nSamplesPerSec=44100;
	WF.wf.nAvgBytesPerSec=44100*4;
	WF.wf.nBlockAlign=4;
	WF.wBitsPerSample=16;
	// WF.cbSize=0;

	dsbd.lpwfxFormat = NULL;
		// (LPWAVEFORMATEX) &WF;

	// wiWave.pwfx;
	// CreateSoundBuffer(&dsbd,&l


	dsRV=lpDS->CreateSoundBuffer( &dsbd,
                              &lpDSB,
                              NULL );
	if (dsRV!=DS_OK) {
		lpDSB=NULL;
		return(-1);
	}

return(1);
}



int StopPlayer(void){
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
int LoadSample (int samplenum,char *name,int pan,int Type)
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
void  ParseDrums(char *FileName)
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
	  TheFirstWrite++;
		
	 if ((ofd = fopen ("test.pcm", "wb")) == NULL)
     {
		return -1;
     }

	}

	
// Use BL for each Lock size call...

	dsRV=lpDSB->GetCurrentPosition(&PlayPos,&WritePos);

	if (dsRV!=DS_OK) {
		return(-1);
	}
	
	LastPos[j].Read=PlayPos;
	LastPos[j].Ticks=GetTickCount()-LastTime;
	LastPos[j].MyPlay=(DWORD )88*(GetTickCount()-StartTime);
	LastPos[j].MWP=MyWritePos;
	LastPos[j].Write=WritePos*2;
	LastPos[j].BA=BytesAhead;

	if (j==100) {
		j=0;
	}
	
	if (TheFirstBuffer==0) {
		if (PlayPos==LastPlayPos) {
			return (0);
		} else {
			if (PlayPos<LastPlayPos) {
				LengthOfWrite=BufferLength-LastPlayPos+PlayPos;
			} else {
				LengthOfWrite=PlayPos-LastPlayPos;
			}
		}	
	} else {
		if (LeftOfBlock<0) {
			MyWritePos=0;
			LengthOfWrite=BL;
			TheFirstBuffer=0;
		}  else {
			return (0);
		}
	}


	
	LastPlayPos=PlayPos;

//	while (BytesAhead>(BufferLength/2)) {
//		Sleep(3);
//		if (Playing==0) {
//			return(-1);
//		}
//	//	dsRV=lpDSB->GetCurrentPosition(&PlayPos,&WritePos);
//		BytesAhead-=(DWORD )(88*(GetTickCount()-LastTime));
//		LastTime=GetTickCount();
//	}
//
//	while (PlayPos<BL) {
//		   Sleep(1);
//		   dsRV=lpDSB->GetCurrentPosition(&PlayPos,&WritePos);
//	}
//	while (MyWritePos+BL>PlayPos) {
//			Sleep(1);
//			dsRV=lpDSB->GetCurrentPosition(&PlayPos,&WritePos);
//	}

if (MyWritePos+LengthOfWrite<BufferLength) {
	if (MyWritePos+LengthOfWrite>=PlayPos) {
		Sleep(40);
	}
}

dsRV=lpDSB->Lock(MyWritePos,LengthOfWrite,&pbData,&dwLength,&pbData2,&dwLength2,0L);

LastPos[j++].Data=(unsigned char *)pbData;

if (dsRV==DS_OK) {
	memcpy(pbData,&data[MyWritePos/2],dwLength);

	if (TheFirstWrite<40) {
		 len=fwrite(&data[MyWritePos/2],sizeof(char),dwLength,ofd);
		 TheFirstWrite++;
	} else {
		
	  if (ofd!=NULL)
          fclose (ofd);
	  ofd=NULL;
	}



	if (dwLength2) {
		memcpy(pbData2,(BYTE *)&data[0],dwLength2);

	if (TheFirstWrite<40) {
		 len=fwrite(&data[0],sizeof(char),dwLength2,ofd);
		 TheFirstWrite++;
	} else {
		if (ofd!=NULL)
           fclose (ofd);
	}

	
	}

	MyWritePos+=dwLength+dwLength2;
	BytesAhead+=dwLength+dwLength2;
	if (MyWritePos>=BufferLength) {
		MyWritePos-=BufferLength;
	}

} else {

	return (-1);
}

dsRV=lpDSB->Unlock(pbData,dwLength,pbData2,dwLength2);

      
return(1);
}

/*
 *  voice_on  NOGUS version. Starts a voice!!
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

	while ((TheFirstBuffer==0)&&((int)2*(BlockCounter)==(int)MyWritePos)) {
			WRITE(1);
			Sleep(2);
	}

	   

	/*
	Test of remove clipping.
	Noice might come when makeing Stereo from a mono sound....
    */
	//data[BlockCounter]=MIN(tmpstorel[j],0x7FFF);
	// data[BlockCounter]=tmpstore[j];
 
    if (tmpstorel[j]>(long)32766)  {
		data[BlockCounter]=32767;
	} else {
		data[BlockCounter]=tmpstorel[j];
	}


    if (tmpstorer[j]<-32767)  {
		data[BlockCounter]=-32768;
	} 
    BlockCounter+=1;      


    if (tmpstorer[j]>32766)  {
		data[BlockCounter]=32767;
	} else {
		data[BlockCounter]=tmpstorer[j];
	}


	
	// data[BlockCounter]=tmpstore[j];
 
    if (tmpstorer[j]<-32767)  {
		data[BlockCounter]=-32768;
	} 

    BlockCounter+=1;      

#if 0 
#endif

//	data[BlockCounter]=tmpstorel[j];
//    BlockCounter+=1;      
//	data[BlockCounter]=tmpstorer[j];

	LeftOfBlock-=4;


    if (LeftOfBlock<=0)  {
		CrossedLimit=1;
		LeftOfBlock=BL;
		BlockCounter=0;
    }
   
}


WRITE(1);
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
      if (CrossedLimit==1)  {
		break;
      }	 
   }
   if (Beat>15)  {
      Beat=0;
      Patt=NextPatt;
   }
   
   RememberPatt=Patt; 
return(CrossedLimit);
}



//
// fill_one_beat
//
/* This will fill one/4 beat to the sample to be played */


int fill_one_beat(int LENGTH)  {
   int j,voi,Soundlength;
   int CrossedLimit=0;

memset( &tmpstorel,0,LENGTH*sizeof(long));
memset( &tmpstorer,0,LENGTH*sizeof(long));

for (voi=0;voi<MAX_VOICE;voi++)  {
	Soundlength=MIN(LENGTH,Voice[voi].count);
	if (Soundlength>0)  {
		for(j=0;j<Soundlength;j++) {
			tmpstorel[j]+=(long)((float)Voice[voi].leftc*(double)(*Voice[voi].pointer));
			tmpstorer[j]+=(long)((float)Voice[voi].rightc*(double)(*Voice[voi].pointer));
			Voice[voi].pointer++;
		}
		Voice[voi].count-=Soundlength;
	} 
}  


   

   for(j=0;j<=LENGTH;j++) {
	   

	 data[BlockCounter]=MIN(tmpstorel[j],0x7FFF);
     if (tmpstorel[j]<-32767)  {
		data[BlockCounter]=-32768;
	 } 
	 LeftOfBlock-=2;
     BlockCounter+=1;      

	 
	 data[BlockCounter]=MIN(tmpstorer[j],0x7FFF);
     if (tmpstorer[j]<-32767)  {
		data[BlockCounter]=-32768;
	 } 
	 LeftOfBlock-=2;
     BlockCounter+=1;      

      if (LeftOfBlock<=0)  {
//		DWORD WRPos=MyWritePos;
		CrossedLimit=1;
		if (TheFirstBuffer==1) {   // Fill PlayBuffer
		   WRITE(1);
		};

		TheFirstBuffer=0;
		LeftOfBlock=BL;
		BlockCounter=0;
      }
   
   }


return (CrossedLimit);
}


//
// FillPattern
//
// returns 1: if Buffer is full.
// returns -1 if failed
//
/*  Fills Play Buffer & Data Before Play*/
int  FillPattern(int Patt)
{

   double DBPM=(double) BPM;
   int j,i=0;
   int samplelen;
   int CrossedLimit=0;

while (CrossedLimit==0) {
   Beat=0;
	 
   samplelen=660000/BPM;                       /* Length of 1/4 beat */
   
   for (i=Beat;i<16;i++) {
		for (j=0;j<L_MAX_DRUMS;j++) { 
	 		if (Pattern[Patt].DrumPattern[j].Drum!=NULL) {
				if (Pattern[Patt].DrumPattern[j].beat[i]>0)  {
					voice_on(Pattern[Patt].DrumPattern[j].Drum->SampleNum);
				}
			}
		}
      
		Beat++;
		CrossedLimit=fill_one_beat(samplelen+1);
		if (CrossedLimit==1)  {
			break;
		}	 

   }
   if (Beat>15)  {
      Beat=0;
   }

}
   
// Give Ahead start of Half BufferLength
while ((int)2*BlockCounter<(int)BufferLength/2-samplelen) {
	i=Beat;
	for (j=0;j<L_MAX_DRUMS;j++) { 
 		if (Pattern[Patt].DrumPattern[j].Drum!=NULL) {
			if (Pattern[Patt].DrumPattern[j].beat[i]>0)  {
				voice_on(Pattern[Patt].DrumPattern[j].Drum->SampleNum);
			}
		}
	}
      
	Beat++;
	i++;
    CrossedLimit=fill_one_beat(samplelen+1);
}
if (Beat>15)  {
	Beat=0;
}


Delay.count=LeftOfBlock+samplelen-1;
Delay.pointer=&data[BlockCounter-2*samplelen];





  
return(CrossedLimit);
}

