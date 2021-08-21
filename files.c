/* 
 * files.c
 * 
 * Module for save and load of drums.
 * 
 * $Date: 1996/04/06 13:34:37 $
 * 
 * $Revision: 1.6 $
 * 
 * 
 */ 


#define SONGPATH  "songs/"
#define M_ID      "xd1.0"
#include "drum.h"
#include "xdrum.h"

extern char *add_root(char *name);
extern char speed_buf[7];
extern char SongBuf[SONG_BUF_LENGTH];
extern tDrum Drum[MAX_DRUMS];
extern tPattern Pattern[MAX_PATTERNS];

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
 *  UnpackBytes
 *  byte1 first byte
 *  byte2 second byte
 * 
 *  pi is pattern index to unpack into
 *  di is drum index to unpack into
 */
int UnpackBytes(char *byte1, char *byte2,int pi,int di)  {
   int k;
   
   for (k=0;k<8;k++) {
      if ( (*byte1 & (0x01 << (7-k)))!=0)  {
	 Pattern[pi].DrumPattern[di].beat[k]=1;
      }
   }
   for (k=8;k<16;k++) {
      if ((*byte2 & (0x01 << (15-k)))!=0)  {
	 Pattern[pi].DrumPattern[di].beat[k]=1;
      }
   }
   return (1);
}

int FindBytes(char *byte1, char *byte2,int pi,int di)  {
  int k;
   int found=0;
   *byte1=0;
   *byte2=0;
   
   for (k=0;k<8;k++) {
      if (Pattern[pi].DrumPattern[di].beat[k]>0)  {
	 *byte1|=(0x01 << (7-k));
	 found=1;
      }
   }
   for (k=8;k<16;k++) {
      if (Pattern[pi].DrumPattern[di].beat[k]>0)  {
	 *byte2|=((0x01) << (15-k)); 
	 found=1;
      }
   }
   return (found);
}



/*
 * 
 *  MatchFileName(char *filename)  {
 *
 *  Tries to find the drum number in this drumset. 
 *   
 * 
 */ 
int MatchFileName(char *filename)  {
   int i=0;
   
   if (filename[0] != '/')	/* if it is not absolute path */ 
      add_root(filename); 

     while (i<MAX_DRUMS) {	
	if (Drum[i].Filename!=NULL)  {	 
	   if (strcmp(Drum[i].Filename,filename)==0)
	     {
		return (i);
	     }
	}
	i++;
     }
   fprintf(stderr,"You are missing the [%s] drum sample.\n",filename);
   fprintf(stderr,"This is no problem if no patterns are missing.\n");

   return(-1);
}
   

int FindNumberOfDrums(void )
{
   int i=0;
   int result=0;
   
   while (i<MAX_DRUMS) {	
      if (Drum[i].Filename!=NULL)  {	 
	 result++;
      }
      i++;
   }
   return(result);
}

/*
 * SaveFile
 * 
 * Saves the whole thing file in the folowing format
 * 
 * Xdrum 1.0 format
 * (I whish I could save this in some other format.)
 * 
 * IdText="xd1.0"
 * Pattern "adda\nssas\naddd"
 * text #of Drums 
 * filename1
 * filename2
 * ...
 * 'a' DrumIndex byte1 byte2
 * 'q' DrumIndex byte1 byte2
 * 
 */

int SaveFile(char *name)  {
   int fd;   
   char filename[512];
   char buff[512];
   char x;
   int pl,i,k,PattI;
   unsigned char bytes[2];
   
   
   sprintf(filename,"%s%s",SONGPATH,name);   
   if (filename[0] != '/')	/* if it is not absolute path */ 
     add_root(filename); 

   if ((fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR |S_IWUSR ))<0)
     {
	perror (name);
	return (0);
     }

   sprintf(buff,"%s",M_ID);   
   write(fd,buff,strlen(buff));
   /* Id */
   
   write(fd,SongBuf,strlen(SongBuf)+1);
   /* Pattern */
   
   write(fd,speed_buf,strlen(speed_buf)+1);
   /* tempo of song */
   
   pl=FindNumberOfDrums();
   sprintf(buff,"%d\n",pl);   
   write(fd,buff,strlen(buff));
   /* Number of drums */
   
   for (i=0;i<pl;i++)  {
      if (Drum[i].Filename!=NULL)  {	 
	 sprintf(buff,"%s\n",Drum[i].Filename);
	 write(fd,buff,strlen(buff));
	 /* Filename */
      }
   }
   
   for (PattI=0;PattI<MAX_PATTERNS;PattI++)  {
      for (k=0;k<pl;k++)  {
	 if (FindBytes(&bytes[0],&bytes[1],PattI,k)>0) {
	    /* Bug fix segv */
	    if (PatternIndexToString(PattI)!=NULL) {
	       if (strlen(PatternIndexToString(PattI))<2) {
		  sprintf(buff,"%s",PatternIndexToString(PattI));
		  write(fd,buff,1);
		  x=(char)k;
		  write(fd,&x,1);
		  write(fd,&bytes[0],2);
	       }		   
	    }
	    
	 }
	 
      }
	    
   }
   
   fprintf(stderr,"Done saving.\n");
   close(fd);
   
}

/*
 * LoadFile(char filename)
 * 
 * Loads the file..
 * 
 */ 

int LoadFile(char *name)  {
   int fd;   
   char filename[512];
   char buff[1024];
   int RealDrumIndex[MAX_DRUMS];
   char x;
   int pl,i,k,PattI,di;
   unsigned char bytes[2];
   char Test[3];
   
   sprintf(filename,"%s%s",SONGPATH,name);   
   if (filename[0] != '/')	/* if it is not absolute path */ 
     add_root(filename);    


   if ((fd = open(filename, O_RDONLY ))<0)
     {
	perror (name);
	return (0);
     }

   EmptyCard();
   
   /* Empty every beat! */
   for (PattI=0;PattI<MAX_PATTERNS;PattI++)  {
      for (di=0;di<MAX_DRUMS;di++)  {
	 for (k=0;k<16;k++) {
	    Pattern[PattI].DrumPattern[di].beat[k]=0;
	 }
      }
   }
   
   /* Empty the drum index table */
   for (di=0;di<MAX_DRUMS;di++)  {
      RealDrumIndex[di]=-1;
   }
   
   read(fd,buff,strlen(M_ID));
   if (strncmp(buff,M_ID,strlen(M_ID))!=0)  {
      fprintf(stderr,"This is not in the right file format\n");
      return(0);
   }
   
   
   /* Id */
   
   i=0;
   read(fd,&buff[i],1);
   while(buff[i]!='\0')  {
      i++;
      if (read(fd,&buff[i],1)<1) {
	 perror("File to short\n");
	 return (0);
      }
   }
   strcpy(SongBuf,buff);

   
   i=0;
   read(fd,&buff[i],1);
   while(buff[i]!='\0')  {
      i++;
      if (read(fd,&buff[i],1)<1) {
	 perror("File to short\n");
	 return (0);
      }
   }
   strcpy(speed_buf,buff);
   
   i=0;
   read(fd,&buff[i],1);
   while(buff[i]!='\n')  {
      i++;
      if (read(fd,&buff[i],1)<1) {
	 perror("File to short\n");
	 close(fd);   
	 return (0);
      }
   }
   
   sscanf(buff,"%d",&pl);
   
   if (pl>MAX_DRUMS)  {
      fprintf(stderr,"File to load contains way too many drums\n");
      close(fd);   
      return (0);
   }
   
   for (di=0;di<pl;di++)  {
      i=0;
      read(fd,&buff[i],1);
      while(buff[i]!='\n')  {
	 i++;
	 if (read(fd,&buff[i],1)<1) {
	    perror("File to short\n");
	    close(fd);
	    return (0);
	 }
      }
      buff[i]='\0';
      RealDrumIndex[di]=MatchFileName(buff);
   }
   
   Test[1]='\0';
   
   while (read(fd,buff,4)>1) {
     
      
      Test[0]=buff[0];
      PattI=StringToPatternIndex(Test);
      if (buff[1]>-1 && buff[1]<MAX_DRUMS)  {
	 di=RealDrumIndex[(int)buff[1]];
      } else  {
	 fprintf(stderr,"Something in the saved file is wrong!\n");
	 fprintf(stderr,"Xdrummer will try to ignore this.\n");
	 di=-1;
      }
      if (di<0)  {
	 fprintf(stderr,"Skiping a pattern due to unknown drum sample\n");
	 fprintf(stderr,"Modify your DRUMS file if you want it right\n");	
      } else  {
	 
	 if (Drum[di].Loaded!=1)  {
	    fprintf(stderr,"Load sample num %d\n",di);
	          load_sample(
		  Drum[di].SampleNum,
		  Drum[di].Filename,
		  Drum[di].pan,
		  Drum[di].Filetype);
	    Drum[di].Loaded=1; 
	 }
	 
	 
	 UnpackBytes(&buff[2],&buff[3],PattI,di);
      }
   }
   
   fprintf(stderr,"Done Loading.\n");
   close(fd);   
}



