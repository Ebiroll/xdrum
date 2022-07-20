#include "my3mu.h"
#include "drum.h"
#include  <windows.h>
#include "resource.h"
#include "keyboard.h"
#include <stdio.h>
#include "tb303.h"
#include <math.h>

#include "Player.h"


// tb303 specific stuff...

/*
 *   1  3     6  8 10    13 15    18 20 22    25 27    30 32 34
 *   c  d     f  g  a     c  d     f  g  a     c  d     f  g  a
 *  C  D  E  F  G  A  B  C  D  E  F  G  A  B  C  D  E  F  G  A  B  C
 *  0  2  4  5  7  9 11 12 14 16 17 19 21 23 24 26 28 29 31 33 35 36
 */

char Note[16]=
{
//	24,23,19,17,19, 7,21,22, 10,20, 8,18,17,8,20,12

	14,21,21,19, 14,26,21,21, 16,19,17,17, 14,19,19,19
};



/*
 * 0 = rest
 * 1 = 16th note
 * 2 = tie
 */

char NoteLength[16]=
{
//	1,1,1,1,1,1,1,1, 2,1,1,1,2,1,1,1
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
};


/*
 * 0 = go instantly to the next note
 * 1 = slide to the new note
 */

char Slide[16] =
{
//	0,0,0,0,0,1,0,0, 1,0,0,0,0,0,0,0
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
};



struct TB myTB;


// End tb303 specific stuff...



extern tDrum         Drum[MAX_DRUMS];
extern void InsertNewSequence(short *s,int Length);

My3mu My3MUGUI;	// Keep data for sequence between updates

static char Octave[24]="4";
static int MyOctave=4;
static int NewOctave=4;

static int pointx[3];
static int pointy[4];

static int offsetX=10;
static int offsetY=245;

void CalculateXY(int i,int Height,int Width ) {   
   int MKL=Height/7;
   int MinKL=MKL/2;
   int MKW=Width;
   int MinKW=(int)(Width*3)/4;
   
   pointx[0]=offsetX;
   pointx[1]=offsetX+MinKW;
   pointx[2]=offsetX+MKW-0;
   
   pointy[0]=offsetY+i*MKL;
   pointy[1]=pointy[0]+MinKL/2;
   pointy[2]=pointy[0]+MKL-(MinKL/2);
   pointy[3]=pointy[0]+MKL;
};
void ModifyXY() {
   
   pointx[0]+=2;
   pointx[1]+=2;
   pointx[2]-=2;
   
   pointy[0]+=2;
   pointy[1]+=2;
   pointy[2]-=2;
   pointy[3]-=2;
   
}

int My3mu::AddLine(int from_x,int from_y,int to_x,int to_y){
  FromX[PI]=from_x;
  FromY[PI]=from_y;
  ToX[PI]=to_x;
  ToY[PI++]=to_y;

  return(1);
};



///////////////////// KEY Stuff ///////////////////
  Key::Key(){
	 IsPressed=0;
	 Area1=NULL;	 
	 Area2=NULL;
	 Area3=NULL;
      };

int Key::IsInArea(int x,int y) {
  int value=0;
  
  if (Area1!=NULL) {	 
    if (Area1->Area::IsInArea(x,y)) {
      value=1;
    }
  }
	 
	    
  if (Area2!=NULL) {	 
    if (Area2->Area::IsInArea(x,y)) {
      value=1;
    }
  }
	 
  if (Area3!=NULL) {	 
    if (Area3->Area::IsInArea(x,y)) {
      value=1;
    }
  }
	 
  return (value);
	 
};

///////////////////END KEY Stuff ///////////////////////



static short *olds=NULL;

int My3mu::TestSequence(){
short *s;

Player::instance()->enableTB();

#if 0
/*
   s=new short[4096*2];
//  s=Test3MUSeq(&MySettings,MySequence,&IL);

   FX_TB303(NULL, s, 4096*2, &myTB);

  if (olds!=NULL) {
	  // This doesnt guarantee that we are freeing a sound that is being played..
		free(olds);
	}

  olds=Drum[0].sample;

  Drum[0].sample=s;
	 

//	  while (*Drum[i].sample>0) {
//		  Drum[i].sample++;
//		  D--;
//	  }
	  
      Drum[0].length=4096*2;
*/
#endif

return(1);
}



int My3mu::GenerateSequence(){
int IL,ix;
short *s,*os;
short max=-10000,min=10000;
   s=new short[4096*20];
//  s=Test3MUSeq(&MySettings,MySequence,&IL);

   os=s;
   FX_TB303(NULL, os, 4096*20, &myTB);

   for(ix=0;ix<4096*20;ix++) {
	   if(*os>max) {
		   max=*os;
	   }

	   if(*os<min) {
		   min=*os;
	   }
	   os++;
   }

  Drum[0].sample=s;
  olds=NULL;	 

//	  while (*Drum[i].sample>0) {
//		  Drum[i].sample++;
//		  D--;
//	  }
	  
      Drum[0].length=4096*20;

	InsertNewSequence(s,4096*20);

return(1);
}

//
// We move keyboard drawing into these functions.
// 

int My3mu::SetWnd( HWND M,HINSTANCE I){



	MyWin=M;
	myInstance=I;

return(1);
}


int My3mu::HandleClick(int x,int y,HWND hWnd){
	int i;
	for (i=0;i<8*12;i++) {
		if (Keys[i].IsInArea(x,y)) {
			int Beat=i/12;
			int Notenum=(MyOctave+1)*12-i+Beat*12;

			if (Keys[i].IsPressed==1){
				Keys[i].IsPressed=0;
				Note[Beat]=0;
				NoteLength[Beat]=0;
				InvalidateRect( hWnd, NULL, TRUE );
			} else {
				Keys[i].IsPressed=1;
				Note[Beat]=Notenum;
				NoteLength[Beat]=1;
				DrawKey(hWnd,i);
			}
		}
	}

return(1);
}
int My3mu::HandleKeyPress(char ch,BOOL shift){

return(1);
}

int My3mu::DrawKey(HWND hWnd,int keynum){
	HDC DC=GetDC(hWnd);
	int i=keynum;
	
	if (Keys[i].Area1) {
			 Rectangle(DC,Keys[i].Area1->LeftX,Keys[i].Area1->LowY,
				 Keys[i].Area1->RightX,Keys[i].Area1->HighY);		   
		 }



	 if (Keys[i].Area2) {
		 Rectangle(DC,Keys[i].Area2->LeftX,Keys[i].Area2->LowY,
			 Keys[i].Area2->RightX,Keys[i].Area2->HighY);		   
	 }

	if (Keys[i].Area3) {
		 Rectangle(DC,Keys[i].Area3->LeftX,Keys[i].Area3->LowY,
			 Keys[i].Area3->RightX,Keys[i].Area3->HighY);		   
	 }

return(1);
}

int My3mu::HandlePaint(HWND hWnd){
	HDC DC;
	int i;

	DC=GetDC(hWnd);
	
		for (i=0;i<PI;i++) {
//			DrawLine(FromX[i],FromY[i],ToX[i],ToY[i]);
		MoveToEx(DC,FromX[i],FromY[i],NULL);
		LineTo(DC,ToX[i],ToY[i]);
		}
   for (i=0;i<13*8;i++) {
	   if (Keys[i].IsPressed) {
			DrawKey(hWnd,i);
	   }
   }
return(1);
}


void My3mu::nmumd(int i,int keynum,int Height,int Width)
  {
     CalculateXY(i,Height,Width );
   
   
   // No minor up, Minor down
   
   AddLine(pointx[0],pointy[0],pointx[2],pointy[0]);
   AddLine(pointx[2],pointy[0],pointx[2],pointy[3]);
   AddLine(pointx[2],pointy[3],pointx[1],pointy[3]);
   AddLine(pointx[1],pointy[3],pointx[1],pointy[2]);
   AddLine(pointx[1],pointy[2],pointx[0],pointy[2]);
   AddLine(pointx[0],pointy[2],pointx[0],pointy[0]);

   ModifyXY();
   
   Keys[keynum].Area1=NULL;
   Keys[keynum].Area2=new Area(pointx[0],pointx[2],pointy[0],pointy[2]);
   Keys[keynum].Area3=new Area(pointx[1],pointx[2],pointy[2],pointy[3]);

  }


void My3mu::mumd(int i,int keynum,int Height,int Width)
  {
   CalculateXY(i,Height,Width );

   // Minor up, Minor down
   
   AddLine(pointx[0],pointy[1],pointx[1],pointy[1]);
   AddLine(pointx[1],pointy[1],pointx[1],pointy[0]);
   AddLine(pointx[1],pointy[0],pointx[2],pointy[0]);
   AddLine(pointx[2],pointy[0],pointx[2],pointy[3]);
   AddLine(pointx[2],pointy[3],pointx[1],pointy[3]);
   AddLine(pointx[1],pointy[3],pointx[1],pointy[2]);
   AddLine(pointx[1],pointy[2],pointx[0],pointy[2]);
   AddLine(pointx[0],pointy[2],pointx[0],pointy[1]);

   ModifyXY();
   
   Keys[keynum].Area1=new Area(pointx[1],pointx[2],pointy[0],pointy[1]);
   Keys[keynum].Area2=new Area(pointx[0],pointx[2],pointy[1],pointy[2]);
   Keys[keynum].Area3=new Area(pointx[1],pointx[2],pointy[2],pointy[3]);   
  }



void My3mu::munmd(int i,int keynum,int Height,int Width)
  {
    
   CalculateXY(i,Height,Width );


   // Minor up, No Minor down
   
   AddLine(pointx[0],pointy[1],pointx[1],pointy[1]);
   AddLine(pointx[1],pointy[1],pointx[1],pointy[0]);
   AddLine(pointx[1],pointy[0],pointx[2],pointy[0]);
   AddLine(pointx[2],pointy[0],pointx[2],pointy[3]);
   AddLine(pointx[2],pointy[3],pointx[0],pointy[3]);
   AddLine(pointx[0],pointy[3],pointx[0],pointy[1]);
   
   ModifyXY();
   
   Keys[keynum].Area1=new Area(pointx[1],pointx[2],pointy[0],pointy[1]);
   Keys[keynum].Area2=new Area(pointx[0],pointx[2],pointy[1],pointy[3]);
   Keys[keynum].Area3=NULL;
   

  }

void My3mu::half(int i,int keynum,int Height,int Width)
  {
   int MKL=Height/7;
   int MinKL=MKL/2;
   int MinKW=(int)(Width*3)/4;
   
   pointx[0]=offsetX;
   pointx[1]=offsetX+MinKW;
   
   pointy[0]=offsetY+i*MKL-(MinKL/2);
   pointy[1]=pointy[0]+MinKL;

   // Halfnote
   
   AddLine(pointx[0],pointy[0],pointx[0],pointy[1]);
   AddLine(pointx[0],pointy[1],pointx[1],pointy[1]);
   AddLine(pointx[1],pointy[1],pointx[1],pointy[0]);
   AddLine(pointx[1],pointy[0],pointx[0],pointy[0]);

//   ModifyXY();
   
   pointx[0]+=2;
   pointx[1]-=2;
   pointy[0]+=2;
   pointy[1]-=2;
   
   Keys[keynum].Area1=new Area(pointx[0],pointx[1],pointy[0],pointy[1]);
   Keys[keynum].Area2=NULL;
   Keys[keynum].Area3=NULL;
   
 
  }




int My3mu::HandleCheckBox(HWND hdwnd,int Id){
	switch(Id) {
		case IDC_VCO_TRIANG:
		     SendDlgItemMessage(hdwnd,IDC_VCO_TRIANG,BM_SETCHECK,1,0);
		     SendDlgItemMessage(hdwnd,IDC_VCO_SQUARE,BM_SETCHECK,0,0);
		     myTB.square=0;
		break;
		case IDC_VCO_SQUARE:
		     SendDlgItemMessage(hdwnd,IDC_VCO_TRIANG,BM_SETCHECK,0,0);
		     SendDlgItemMessage(hdwnd,IDC_VCO_SQUARE,BM_SETCHECK,1,0);
		     myTB.square=1;
		break;
	}
return(1);
}

int My3mu::AdjustCheckBox(HWND hdwnd){

	if (myTB.square==0) {
		     SendDlgItemMessage(hdwnd,IDC_VCO_TRIANG,BM_SETCHECK,1,0);
	} else {
		     SendDlgItemMessage(hdwnd,IDC_VCO_TRIANG,BM_SETCHECK,0,0);
	}

	if (myTB.square==1) {
		     SendDlgItemMessage(hdwnd,IDC_VCO_SQUARE,BM_SETCHECK,1,0);
	} else {
		     SendDlgItemMessage(hdwnd,IDC_VCO_SQUARE,BM_SETCHECK,0,0);
	}

return(1);
}	



int My3mu::HandleSlideAdjust(HWND hdwnd,HWND hSlideWnd){
int i,trackpos;

  for(i=0;i<MaxSliders;i++) {
     if (S3MGUI[i].hTrackWnd==hSlideWnd) {
		trackpos=SendMessage(hSlideWnd,TBM_GETPOS,0,0);
		*S3MGUI[i].Data=(float)trackpos/(float)S3MGUI[i].ScaleFactor;
		return(1);
	}
  }

return(0);
}


//
// The IDC_VALUES must correspond to the ones in the resource file..
//

int My3mu::SetSlider(int IdNumber,int max,int min,int scalef,double *data){

  S3MGUI[MaxSliders].IdNumber=IdNumber;
  S3MGUI[MaxSliders].Max_Val=max;
  S3MGUI[MaxSliders].Min_Val=min;
  S3MGUI[MaxSliders].ScaleFactor=scalef;
  S3MGUI[MaxSliders].Data=data;

  MaxSliders++;
  return(1);
}


int My3mu::AdjustSliders(HWND hdwnd){

	HWND hTrackWnd;

	for (int i=0;i<MaxSliders;i++) {

		hTrackWnd=GetDlgItem(hdwnd,S3MGUI[i].IdNumber);
		S3MGUI[i].hTrackWnd=hTrackWnd;

		SendMessage(hTrackWnd,TBM_SETRANGE,(WPARAM) 1,
			(LPARAM) MAKELONG(S3MGUI[i].Min_Val,S3MGUI[i].Max_Val));

		SendMessage(hTrackWnd,TBM_SETPOS,(WPARAM) 1,
			(LPARAM) ((double)*S3MGUI[i].Data*(double)S3MGUI[i].ScaleFactor));

		SendMessage(hTrackWnd,TBM_SETTICFREQ,(WPARAM)(S3MGUI[i].Max_Val-S3MGUI[i].Min_Val)/8,
			(LPARAM) 0);

		}

	return(1);
}


My3mu::My3mu() {
   int i,j,row;
   PI=0;
	
   Height=200;
   Width=40;

   /*
	tb->tune   = (Controls[0].val) / 12.0;
	tb->freq   = Controls[1].val / 100.0;
	tb->reso   = Controls[2].val / 100.0;
	tb->envmod = (Controls[3].val / 100.0); // * (1.0 - tb->freq);
	tb->decay  = 1.0 - pow(10.0, -2 -4 * (Controls[4].val / 100.0));
	tb->accent = Controls[5].val / 100.0;

	tb->notelength = (int)((44100 * 0.125 * 120) / Controls[6].val);
	tb->volume     = Controls[7].val / 100.0;
    */


	myTB.hpffreq    = 0.92;
	myTB.hpfq       = 1.00;;
	myTB.sqrrise    = 0.55;
	myTB.sqrfall    = 0.45;
	myTB.sawrise    = 0.01;
	myTB.notelength = 4;

	myTB.volume=1.0;
//	myTB.decay=0.999999;
	myTB.decay=0.56;

	myTB.Tempo=120;
	myTB.Volume=1.00;

	myTB.square=0;
	myTB.tune   = 0.0;
	myTB.freq   = 0.3;
	myTB.reso   = 0.25;
	myTB.envmod = 0.95;
	myTB.decay  = 1.0 - pow(10.0, -2 -4 * (56 / 100.0));
//	myTB.decay  = 0.9;
	myTB.accent = 0.0;

//	myTB.notelength = (int)((44100 * 0.125 * 120) / Controls[6].val);
	myTB.volume     = 0.7;
		

	// Extra stuff..
	myTB.notelength = (int)((44100 * 0.125 * 120)/120);

	MaxSliders=0;


    SetSlider(IDC_VCF_CUTOFF,100,1,100,&(myTB.freq));
//	SetSlider(IDC_VCF_CUTOFF,100,1,100,&(myTB.hpfq)); or hpfreq
	SetSlider(IDC_VCF_RESONANCE,100,0,100,&(myTB.reso));

    SetSlider(IDC_EG_A,100,0,100,&(myTB.attack));
	SetSlider(IDC_VCO_PULSEWIDTH,100,0,100,&(myTB.sawrise));
	SetSlider(IDC_VCO_DETUNE,13,-13,13,&(myTB.tune));

//  SetMyDefaultData(&MySettings);



// 
  // Call Default values for MySettings & MySequence
	SetSlider(IDC_EG_D,200,0,100,&(myTB.decay));

  
//	SetSlider(IDC_VCF_CUTOFF,1000,0,1,&(myTB.hpffreq));
//	SetSlider(IDC_VCF_RESONANCE,100,0,1,&(myTB.hpfq));

  
  // Dunno about this..
  //SetSlider(IDC_EG_S,10000,0,1,&(myTB.sustain));
  //
//  SetSlider(IDC_EG_R,10000,0,1,&(MySettings.Release));
//  SetSlider(IDC_VCO_PULSEWIDTH,100,0,1,&(MySettings.PulseRatio));
#if 0
  /* Setup this l8tr
  

SetSlider(IDC_VCF_CUTOFFFACT,110,0,1,&(MySettings.CutoffFactor));
SetSlider(IDC_EG_ADSRFACT,110,0,1,&(MySettings.ADSRFactor));
SetSlider(IDC_VCF_RESOFACT,110,0,1,&(MySettings.ResoFactor));
SetSlider(IDC_VCF_AMOUNT,100,0,1,&(MySettings.VCFamount));




//SetSlider(ID_EG_VCF_MOD,110,0,1,&(MySettings.VCFmodFactor));
//SetSlider(ID_EG_VCO_MOD,110,0,1,&(MySettings.PitchModFactor));
//SetSlider(ID_EG_VCA_MOD,110,0,1,&(MySettings.AmpModFactor));
SetSlider(ID_EG_VCF_MOD,110,0,1,&(MySettings.EnvModVCF));
SetSlider(ID_EG_VCO_MOD,110,0,1,&(MySettings.EnvModPitch));
SetSlider(ID_EG_VCA_MOD,110,0,1,&(MySettings.EnvModAmp));

SetSlider(IDC_VCO_GAIN,30,-10,1,&(MySettings.Gain));
  
*/
#endif

//--- Keyboard Stuff


   // We need to define Areas for user input etc
   j=0;
   
   for (row=1;row<9;row++) {
	  offsetX+=45;
   	  i=0;

	  nmumd(i++,j++,Height,Width);
	  half(i,j++,Height,Width);

	  mumd(i++,j++,Height,Width);
	  half(i,j++,Height,Width);
	  mumd(i++,j++,Height,Width);
	  half(i,j++,Height,Width);
	  munmd(i++,j++,Height,Width);
	  nmumd(i++,j++,Height,Width);
	  half(i,j++,Height,Width);
	  mumd(i++,j++,Height,Width);
	  half(i,j++,Height,Width);
	  munmd(i++,j++,Height,Width);
   }
     
  

}



//
// This function is exported to wndprog...
// It handles the 3MU dialog.
//

BOOL CALLBACK My3MUDialog(HWND hdwnd,UINT message,
						 WPARAM wParam,LPARAM lparam) {
  	static HWND hTrackWnd;
	int checknum=1;

	switch (message) {
		 case WM_LBUTTONDOWN:
		    My3MUGUI.HandleClick(LOWORD(lparam),HIWORD(lparam),hdwnd);
		 break;
	case WM_PAINT:
		{
			PAINTSTRUCT myPS;
			HDC hpDC = BeginPaint( hdwnd, &myPS );
			My3MUGUI.HandlePaint(hdwnd);
			EndPaint( hdwnd, &myPS );
		}
		return(1);
	case WM_INITDIALOG:
//	Instance found elsewhere	My3MUGUI.SetWnd( HWND M,HINSTANCE I);
		My3MUGUI.AdjustSliders(hdwnd);
		My3MUGUI.AdjustCheckBox(hdwnd);
		SetDlgItemText(hdwnd,IDC_EDIT2,Octave);
		{
			int i;
			for (i=IDC_CHECK1;i<=IDC_CHECK8;i++) {
//				 if (My3MUGUI.MySequence[i-IDC_CHECK1].Porta>0) {
//			        SendDlgItemMessage(hdwnd,i,BM_SETCHECK,1,1);
//				 } else 
//				 {
//					 SendDlgItemMessage(hdwnd,i,BM_SETCHECK,0,1);
//				 }
			}
		}
		
		
		return (1);
	case WM_VSCROLL:
		hTrackWnd=(HWND) lparam;
		
       		switch(LOWORD(wParam)){
			case TB_TOP:
			case TB_BOTTOM:
			default:
				My3MUGUI.HandleSlideAdjust(hdwnd,hTrackWnd);
			break;
		}
		break;

		break;

	case WM_HSCROLL:
		hTrackWnd=(HWND) lparam;
		
       		switch(LOWORD(wParam)){
			case TB_TOP:
			case TB_BOTTOM:
			default:
				My3MUGUI.HandleSlideAdjust(hdwnd,hTrackWnd);
			break;
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
			case IDOK:
				Player::instance()->disableTB();
				EndDialog(hdwnd,0);
				return (1);
				break;
			case IDCANCEL:
		
				 My3MUGUI.GenerateSequence();

				break;
			case IDC_BUTTON1:
				  My3MUGUI.TestSequence();
				break;

			case IDC_CHECK1:
				checknum=0;
				if (Slide[checknum]!=1) {
				  Slide[checknum]=2; 
				} else {
				  Slide[checknum]=1;
				}

				break;
			case IDC_CHECK2:
				checknum=1;
				if (Slide[checknum]!=1) {
				  Slide[checknum]=2; 
				} else {
				  Slide[checknum]=1;
				}

				break;
			case IDC_CHECK3:
				checknum=2;
				if (Slide[checknum]!=1) {
				  Slide[checknum]=2; 
				} else {
				  Slide[checknum]=1;
				}

				break;
			case IDC_CHECK4:
				checknum=3;
				if (Slide[checknum]!=1) {
				  Slide[checknum]=2; 
				} else {
				  Slide[checknum]=1;
				}

				break;
			case IDC_CHECK5:
				checknum=4;
				if (Slide[checknum]!=1) {
				  Slide[checknum]=2; 
				} else {
				  Slide[checknum]=1;
				}

				break;
			case IDC_CHECK6:
				checknum=5;
				if (Slide[checknum]!=1) {
				  Slide[checknum]=2; 
				} else {
				  Slide[checknum]=1;
				}

				break;
			case IDC_CHECK7:
				checknum=6;
				if (Slide[checknum]!=1) {
				  Slide[checknum]=2; 
				} else {
				  Slide[checknum]=1;
				}

				break;


			case IDC_CHECK8:
				checknum=7;
				if (Slide[checknum]!=1) {
				  Slide[checknum]=2; 
				} else {
				  Slide[checknum]=1;
				}

				break;


			case IDC_EDIT2:
				   GetDlgItemText(hdwnd,IDC_EDIT2,Octave,10);
				   sscanf(Octave,"%d",&NewOctave);
				   if (NewOctave!=MyOctave) {
					   for(int i=0;i<9;i++) {
							Note[i]+=(NewOctave-MyOctave)*12;
							if (Note[i]<0) Note[i]=0;
					   }
					   MyOctave=NewOctave;

				   } 
				   
				   break;
			case IDC_VCO_TRIANG:
			case IDC_VCO_SQUARE:
			case IDC_VCO_FILE:
			 	My3MUGUI.HandleCheckBox(hdwnd,LOWORD(wParam));
			break;
		}
		break;
	}
return (0);
}

