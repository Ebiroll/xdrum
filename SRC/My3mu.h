#ifndef _MY3MUGUI_H
#define _MY3MUGUI_H

#include <windows.h>
#include <commctrl.h>
#include "drum.h"
#include "object.h"
#include "keyboard.h"



// Sliders
// IDC_EG_A,D,S,R
// ID_EG_VCF_MOD,VCA_MOD,VCO_MOD
// IDC_VCF_CUTOFF,IDC_VCF_RESONANCE


// Check boxes (radio buttons)
// IDC_VCO_TRIANG
// IDC_VCO_SQUARE
// IDC_VCO_FILE  (disabled)

#define MAX_SLIDERS 54


class My3mu {
public:
// This class is generated once but SetWnd is called each time a new Dialog appears....
   My3mu();
   int SetWnd( HWND M,HINSTANCE I);

// 
   int HandleClick(int x,int y,HWND hWnd);
   int HandleKeyPress(char ch,BOOL shift);
   int HandlePaint(HWND hWnd);

   int DrawKey(HWND hWnd,int keynum);

//  Theese two functions interface the play module 
   int GenerateSequence();
   int TestSequence();
//   int PlaySequence();

   int AdjustSliders(HWND hdwnd);	 // AdjustSliders will fill in S3MUGUI struct
   int HandleSlideAdjust(HWND hdwnd,HWND hSlideWnd);

   int HandleCheckBox(HWND hdwnd,int Id);
   int AdjustCheckBox(HWND hdwnd);	


   int SetSlider(int IdNumber,int max,int min,int scalef,double *data);	

//   struct Settings	MySettings;
//   struct Note		MySequence[16];

protected:

struct S3 { 

  HWND hTrackWnd;
  int  IdNumber;
  int  Max_Val;
  int  Min_Val;
  int  ScaleFactor;
  double *Data;	

} S3MGUI[MAX_SLIDERS];

   int MaxSliders;
   HWND MyWin;
   HINSTANCE myInstance;

//--------------------------------------- keyboard stuff
	 
	int AddLine(int from_x,int from_y,int to_x,int to_y);	  
	void half(int i,int keynum,int Height,int Width);
	void nmumd(int i,int keynum,int Height,int Width);
	void mumd(int i,int keynum,int Height,int Width);
	void munmd(int i,int keynum,int Height,int Width);



private:		//--------------------------------------- private

  Key Keys[13*18];
  
  int FromX[700];
  int FromY[700];
  int ToX[700];
  int ToY[700];
  int PI;	
  int Width;
  int Height;
  
};



#endif