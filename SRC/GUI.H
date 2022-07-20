#ifndef _GUI_H
#define _GUI_H

#include <windows.h>
#include "drum.h"
#include "object.h"

#define MAX_OBJECT MAX_DRUMS*MAX_BEAT


class MainGui {
public:
   MainGui(HWND MainWin,HINSTANCE hInstance);
   int NewDrum(int DrumNum);
   int NewSequence(int seqnr,short *s,int Length);
   int SetCurrentPattern(int Patt);
   int HandleClick(int x,int y);
   int HandleKeyPress(char ch,BOOL shift);
   int HandlePaint(HWND hWnd);
   BOOL IsDrumLoaded(int DrumNr);
protected:
   
	Object *Buttons[MAX_DRUMS][MAX_BEAT];
	int ObjectPtr;

	HBITMAP hOn,hOff;
	HWND MyWin;
	HINSTANCE myInstance;
};

 class Mixer {
 public:
	Mixer(HWND MainWin,HINSTANCE hInstance);
	void PopUp();
  private:
	HWND MyWin;
	HINSTANCE myInstance;
};


#endif