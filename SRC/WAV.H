#ifndef _WAV_H
#define _WAV_H

#include <stdio.h>
#include <windows.h>


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

extern short int LoadWav(FILE *wav, dword *flags, dword *dataposition, dword *datasize);



#endif