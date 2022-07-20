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

      if (LeftOfBlock==50000)  {
 		   WRITE(1);
	  }
      if (LeftOfBlock<=0)  {
  		   WRITE(1);
	  }


      if (LeftOfBlock<=0)  {
//		DWORD WRPos=MyWritePos;
		CrossedLimit=1;
		if (TheFirstBuffer==1) {   // Fill PlayBuffer
		   //WRITE(1);
		};

		TheFirstBuffer=0;
		LeftOfBlock=BL;
		BlockCounter=0;
      }
   
   }

return (CrossedLimit);
}




