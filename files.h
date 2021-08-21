/* 
 * files.h
 * 
 * Defines the functions used by xdrum.c to load and save.
 * 
 * $Date: 1996/03/23 20:16:51 $
 * 
 * $Revision: 1.1 $
 * 
 * 
 */ 


#ifndef FILES_H
#define FILES_H

int SaveFile(char *name);
int LoadFile(char *name);
int FindBytes(char *byte1, char *byte2,int pi,int di);
#endif
