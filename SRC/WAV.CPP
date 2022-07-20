// Listing 1 - LoadWav
#include "wav.h"
#include <stdio.h>

short int LoadWav(FILE *wav, dword *flags, dword *dataposition, dword *datasize) {

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
