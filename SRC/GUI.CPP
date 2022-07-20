#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <commctrl.h>
#include <assert.h>
#include "resource.h"
#include "player.h"


#include "drum.h"
#include "gui.h"

extern tDelay Delay;

int LoadedDrums[MAX_DRUMS];
int DrumPos=0;

extern tDrum Drum[];
extern tPattern Pattern[MAX_PATTERNS];
extern int LoadSequence (int sampnum,short *s,int Length);



int EditPattern=0;


typedef struct  {
	int DrumNr;
	int Beat;
} tData;

tData TheData[MAX_DRUMS][MAX_BEAT];


//
// Parameter to function is of the type tData
// Callback is called by object...
//
void PushCallback(void *Data){
//	char String[512];
	tData	*myData=(tData *)Data;
	int DrumIndex=myData->DrumNr;
	int BeatNr=myData->Beat;


 if (Pattern[EditPattern].DrumPattern[DrumIndex].beat[BeatNr]>0)  {
      Pattern[EditPattern].DrumPattern[DrumIndex].beat[BeatNr]=0;
   } else  {
      Pattern[EditPattern].DrumPattern[DrumIndex].beat[BeatNr]=1;
   }
   

//	sprintf(String,"Drum %u, Beat %u",myData->DrumNr,myData->Beat);
//	MessageBox(myWin,String,"Callback",MB_OK);

}


void PaintData(HWND hWnd) {

	// A PAINTSTRUCT holds data about the client area being drawn in
	// It is filled by the BeginPaint() call, and re-read by the
	// EndPaint() call to tell Windows which area has been updated.
	// A brush object is used to fill closed shapes with colour.

	HBRUSH hBr;     // A brush for painting closed shapes
	char demoStr[260];
	int i,sx=10,sy=60; // start pos
	int Length=250;
	//COLORREF txtColour,bgColour;

	HDC hDC;



	hDC = GetDC( hWnd );

//	txtColour == RGB( 0, 0, 0 );
//	SetTextColor( hDC, txtColour );
//	hBr = CreateSolidBrush( txtColour );
//	hBr = GetStockObject( LTGRAY_BRUSH );
//	hBr = SelectObject( hDC, hBr );

//	MoveToEx(hDC,sx,sy,NULL);
//	LineTo(hDC,sx,sy+20);

	SetBkMode(hDC,TRANSPARENT);
	i=0;
	while (LoadedDrums[i]>=0) {

//	  	SetTextAlign( hDC, TA_CENTER|TA_BASELINE );
		sprintf(demoStr,"%s",Drum[LoadedDrums[i]].Name);
		TextOut( hDC, 5, 40+20*i, demoStr, strlen( demoStr ) );
		i++;
	}

	hBr =(struct HBRUSH__ *) SelectObject( hDC, hBr );
	DeleteObject( hBr );
	
	DeleteDC( hDC );
};













//
// This function should be called from a menu selection...
// Remember to remove base from value...

int MainGui::NewDrum(int DrumNum) {
	int AddDrum=0;
	int xPos=0;
	
	if (DrumNum<MAX_DRUMS) {
		if (Drum[DrumNum].Loaded==0) {
			if (Player::instance()->LoadSample (DrumNum,Drum[DrumNum].Filename,
				Drum[DrumNum].pan,Drum[DrumNum].Filetype)>0) {
				AddDrum=1;
			}  else { 
				MessageBox(MyWin,Drum[DrumNum].Filename,"File not found",MB_OK);
			 }
			
		} else {
			AddDrum=1;
		}

	// hOn ,hOff
		if (AddDrum==1) {
			int i;
			// Fill in global data used by CallBack
			for (i=0;i<MAX_BEAT;i++) {
				TheData[DrumPos][i].Beat=i;
				TheData[DrumPos][i].DrumNr=DrumNum;
			}

			xPos=60;
			for (i=0;i<MAX_BEAT;i++) {
				xPos+=20;
				if (i%4==0) {
					xPos+=4;
				}
				// TODO	group theese objects to a unit....
				// We need to change state whenpatterns change...
				// Also key 1-8 Press & Shift 1-8 should toggle current
				// Drum pattern.. WHY Not [oooooooooooooooo]
				// Alt could move [oooo]
				Buttons[ObjectPtr][i]=new Object(xPos,40+DrumPos*20,hOn,myInstance);
				Buttons[ObjectPtr][i]->SetToggleBitmap(hOff);
				Buttons[ObjectPtr][i]->SetCallBackFunc(PushCallback,(void *)&TheData[DrumPos][i]);
				

			}
			ObjectPtr++;
			LoadedDrums[DrumPos]=DrumNum;
			DrumPos++;

		}

	}
return(1);
}

int MainGui::NewSequence(int seqnr,short *s,int Length) {

	char Buff[100];
	int AddDrum=0;
	int xPos=0;

	

	if (seqnr<MAX_DRUMS) {
		sprintf(Buff,"3MU seq");
		Drum[seqnr].Name=(char *)malloc(strlen(Buff)+1);
		strcpy(Drum[seqnr].Name,Buff);
		LoadSequence(seqnr,s,Length);
	}

	AddDrum=1;

	// hOn ,hOff
	if (AddDrum==1) {
		int i;
		// Fill in global data used by CallBack
		for (i=0;i<MAX_BEAT;i++) {
			TheData[DrumPos][i].Beat=i;
			TheData[DrumPos][i].DrumNr=seqnr;
		}

		xPos=60;
		for (i=0;i<MAX_BEAT;i++) {
			xPos+=20;
			if (i%4==0) {
				xPos+=4;
			}
			// TODO	group theese objects to a unit....
			// We need to change state whenpatterns change...
			// Also key 1-8 Press & Shift 1-8 should toggle current
			// Drum pattern.. WHY Not [oooooooooooooooo]
			// Alt could move [oooo]
			Buttons[ObjectPtr][i]=new Object(xPos,40+DrumPos*20,hOn,myInstance);
			Buttons[ObjectPtr][i]->SetToggleBitmap(hOff);
			Buttons[ObjectPtr][i]->SetCallBackFunc(PushCallback,(void *)&TheData[DrumPos][i]);
			
		}
		ObjectPtr++;
		LoadedDrums[DrumPos]=seqnr;
		DrumPos++;

	}


return(1);
}



MainGui::MainGui(HWND MainWin,HINSTANCE hInstance){
	int i,j;

	for (i=0;i<MAX_DRUMS;i++) {
		LoadedDrums[i]=-1;
	}

	this->MyWin=MainWin;
	this->myInstance=hInstance;
	ObjectPtr=0;
	
	
	hOn=LoadBitmap(hInstance,"ON");
	hOff=LoadBitmap(hInstance,"OFF");

	i=0;

   	for (i=0;i<MAX_DRUMS;i++) {
	  for (j=0;j<MAX_BEAT;j++) {
		Buttons[i][j]=NULL;
	  }
    }
}


int MainGui::SetCurrentPattern(int Patt){
// Here we go through all loaded drums and set the buttons correctly..	
	
return(1);
};

int MainGui::HandleClick(int x,int y){
 int i=0;
 int j=0;
 

	for (i=0;i<MAX_DRUMS;i++) {
	 for (j=0;j<MAX_BEAT;j++) {
		 if (Buttons[i][j]!=NULL) {
			if (Buttons[i][j]->HandleClick(x,y)) {
				Buttons[i][j]->Draw(MyWin);
				break;
			}
		 }
		
	}

  }
return(1);
}

int MainGui::HandleKeyPress(char ch,BOOL shift){
	int i,j;
	int OldEdit;

	if (shift==0) {
		EditPattern=ch-'a';	
		

		for (i=0;i<MAX_DRUMS;i++) {
			 for (j=0;j<MAX_BEAT;j++) {
				 if (Buttons[i][j]!=NULL) {
		 			if (Pattern[EditPattern].DrumPattern[LoadedDrums[i]].beat[j]>0)  {
						 Buttons[i][j]->SetState(1);
					} else  {
						 Buttons[i][j]->SetState(0);
					}
				}
			 }
		}



	
	

	} else {
		OldEdit=EditPattern;
	    EditPattern=ch-'A';
		
	
		for (i=0;i<MAX_DRUMS;i++) {
			 for (j=0;j<MAX_BEAT;j++) {
				 Pattern[EditPattern].DrumPattern[LoadedDrums[i]].beat[j]=Pattern[OldEdit].DrumPattern[LoadedDrums[i]].beat[j];
				 if (Buttons[i][j]!=NULL) {
		 			if (Pattern[EditPattern].DrumPattern[LoadedDrums[i]].beat[j]>0)  {
						 Buttons[i][j]->SetState(1);
					} else  {
						 Buttons[i][j]->SetState(0);
					}
				}
			 }
		}


	
	
	
	}
	

	assert((EditPattern>=0)&&(EditPattern<MAX_PATTERNS));
	return(1);
}

int MainGui::HandlePaint(HWND hWnd){
 	int i=0,j=0;

	PaintData(hWnd);


    for (i=0;i<MAX_DRUMS;i++) {
		for (j=0;j<MAX_BEAT;j++) {
		  if (Buttons[i][j]!=NULL) {
			  Buttons[i][j]->Draw(hWnd);
		  }
	  }
	}
return(1);
}

BOOL MainGui::IsDrumLoaded(int DrumNr){
	int i;

	for (i=0;i<DrumPos;i++) {
		if (LoadedDrums[i]==DrumNr) {
			return (TRUE);
		}
	}


return(FALSE);
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
static HINSTANCE hTrackInstance=NULL;

BOOL CALLBACK DialogFunc(HWND,UINT,WPARAM,LPARAM);


// CreateTrackbar - creates and initializes a trackbar.  
// 
// Global variable 
//     g_hinst - instance handle 
HWND WINAPI CreateVolTrackbar( 
    HWND hwndDlg,  // handle of dialog box (parent window) 
    int yPos,     // Position in window 
    int volPos	  // VOL Position (0 - 255) 
)   
{ 
 	HWND hwndTrack;

//    InitCommonControls(); // loads common control's DLL 
 
    hwndTrack = CreateWindowEx( 
        0,                             // no extended styles 
        TRACKBAR_CLASS,                // class name 
        "Trackbar Control",            // title (caption) 
        WS_CHILD | WS_VISIBLE | 
        TBS_AUTOTICKS | WS_TABSTOP ,
        280, yPos,                        // position 
        160, 20,                       // size 
        hwndDlg,                       // parent window 
        (HMENU__ *)55000,					// control identifier 
        hTrackInstance,                       // instance 
        NULL                           // no WM_CREATE parameter 
        ); 
 
    SendMessage(hwndTrack, TBM_SETRANGE, 
        (WPARAM) TRUE,                   // redraw flag 
        (LPARAM) MAKELONG(0, 255));  // min. & max. positions 

    SendMessage(hwndTrack, TBM_SETPAGESIZE, 
        0, (LPARAM) 4);                  // new page size 
 
//    SendMessage(hwndTrack, TBM_SETSEL, 
//        (WPARAM) FALSE,                  // redraw flag 
//        (LPARAM) MAKELONG(iSelMin, iSelMax));
// | TBS_ENABLESELRANGE,  // style 
	
    SendMessage(hwndTrack, TBM_SETPOS, 
        (WPARAM) TRUE,                   // redraw flag 
        (LPARAM) volPos); 

	SendMessage(hwndTrack,TBM_SETTICFREQ,(WPARAM) 8,
		(LPARAM) 0);


//    SetFocus(hwndTrack); 
 
    return hwndTrack; 
} 


HWND WINAPI CreatePanTrackbar( 
    HWND hwndDlg, // handle of dialog box (parent window) 
    int yPos,     // Position in window 
    int panPos	  // Pan Position (-128 - +128) 
)
{
HWND hTrackWnd;

hTrackWnd=CreateWindow(TRACKBAR_CLASS,
			"Trackbar", 
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS ,
			100,yPos,
			160,20,
			hwndDlg,NULL,hTrackInstance,NULL);
			
		SendMessage(hTrackWnd,TBM_SETRANGE,(WPARAM) 1,
			(LPARAM) MAKELONG(0,255));

		SendMessage(hTrackWnd,TBM_SETPOS,(WPARAM) 1,
			(LPARAM) panPos+128);

		SendMessage(hTrackWnd,TBM_SETTICFREQ,(WPARAM) 8,
			(LPARAM) 0);

		

return(hTrackWnd);
}

struct {
	HWND hPan;
	HWND hVol;
} Trackbar[MAX_DRUMS];

BOOL CALLBACK DialogFunc(HWND hdwnd,UINT message,
						 WPARAM wParam,LPARAM lparam) {
	static long udpos=0;
	static long trackpos=0;
	static HWND hEboxWnd; // Edit Box....
	static HWND hTrackWnd;
	int low=0,high=10;
	int i=0;

	switch (message) {
	case WM_PAINT:
		{
				PAINTSTRUCT myPS;
				HDC hpDC = BeginPaint( hdwnd, &myPS );
				PaintData(hdwnd);
				EndPaint( hdwnd, &myPS );
		}
		return(1);
	case WM_INITDIALOG:

//	CreateTrackbar(hdwnd,0,1000,30,800);
//	CreateNewTrackbar(hdwnd,0,256);
		i=0;
		while (LoadedDrums[i]>=0) {
			Trackbar[i].hPan=CreatePanTrackbar(hdwnd,40+20*i,Drum[LoadedDrums[i]].pan);
			Trackbar[i].hVol=CreateVolTrackbar(hdwnd,40+20*i,Drum[LoadedDrums[i]].vol);
			//	  	SetTextAlign( hDC, TA_CENTER|TA_BASELINE );
//sprintf(demoStr,"%s",Drum[LoadedDrums[i]].Name);
//TextOut( hDC, 5, 40+20*i, demoStr, strlen( demoStr ) );
		i++;
	}


		return (1);
	case WM_VSCROLL:
		break;
	case WM_HSCROLL:
		hTrackWnd=(HWND) lparam;
		i=0;
		while (LoadedDrums[i]>=0) {
			if (Trackbar[i].hPan==hTrackWnd) break;
			if (Trackbar[i].hVol==hTrackWnd) break;
			i++;
		}
		if (LoadedDrums[i]<0) return(1);   // This message is of no concern to us..
		switch(LOWORD(wParam)){
			case TB_TOP:
			case TB_BOTTOM:
			default:
				trackpos=SendMessage(Trackbar[i].hVol,TBM_GETPOS,0,0);
				Drum[LoadedDrums[i]].vol=(int)trackpos;
				trackpos=SendMessage(Trackbar[i].hPan,TBM_GETPOS,0,0);
				Drum[LoadedDrums[i]].pan=(int)trackpos-128;
			break;
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hdwnd,0);
			return (1);
		case IDOK:
			EndDialog(hdwnd,0);
			return (1);

		
		}
	}
return (0);
}




Mixer::Mixer(HWND MainWin,HINSTANCE hInstance){
	int i=0;
  	MyWin=MainWin;
	hTrackInstance=hInstance;
	for (i=0;i<MAX_DRUMS;i++) {
		Trackbar[i].hPan=NULL;
		Trackbar[i].hVol=NULL;
	}
}

void Mixer::PopUp(){
if (hTrackInstance!=NULL) {
	// Was "MYDB"
 DialogBox(hTrackInstance,"MYDB",MyWin,(DLGPROC)DialogFunc);



}

}


BOOL CALLBACK DelayDialogFunc(HWND hdwnd,UINT message,
						 WPARAM wParam,LPARAM lparam) {
	static long udpos=0;
	static long trackpos=0;
	static HWND hEboxWnd; // Edit Box....
	static HWND hTrackWnd;
	int low=0,high=10;
	int i=0;

	switch (message) {
	case WM_PAINT:
		{
				PAINTSTRUCT myPS;
				HDC hpDC = BeginPaint( hdwnd, &myPS );
//				PaintData(hdwnd);
				EndPaint( hdwnd, &myPS );
		}
		return(1);
	case WM_INITDIALOG:
		 if (Delay.On==1) {
			 SendDlgItemMessage(hdwnd,IDC_CHECK1,BM_SETCHECK,1,0);
		 } else {
			 SendDlgItemMessage(hdwnd,IDC_CHECK1,BM_SETCHECK,0,0);
		 }

		hTrackWnd=GetDlgItem(hdwnd,IDC_SLIDER1);


 		SendMessage(hTrackWnd,TBM_SETRANGE,(WPARAM) 1,
			(LPARAM) MAKELONG(0,200));

		SendMessage(hTrackWnd,TBM_SETPOS,(WPARAM) 1,
			(LPARAM) (Delay.leftc*200.0));

		SendMessage(hTrackWnd,TBM_SETTICFREQ,(WPARAM) 25,
			(LPARAM) 0);

		hTrackWnd=GetDlgItem(hdwnd,IDC_SLIDER2);


 		SendMessage(hTrackWnd,TBM_SETRANGE,(WPARAM) 1,
			(LPARAM) MAKELONG(0,16*16));

		SendMessage(hTrackWnd,TBM_SETPOS,(WPARAM) 1,
			(LPARAM) (Delay.Delay16thBeats));

		SendMessage(hTrackWnd,TBM_SETTICFREQ,(WPARAM) 16,
			(LPARAM) 0);



		return (1);
	case WM_VSCROLL:
		break;

	case WM_HSCROLL:
		hTrackWnd=(HWND) lparam;
		i=0;
	//	hTrackWnd=(void *)IDC_SLIDER2;
		switch(LOWORD(wParam)){
			case TB_TOP:
			case TB_BOTTOM:
			default:

			hTrackWnd=GetDlgItem(hdwnd,IDC_SLIDER1);
			trackpos=SendMessage(hTrackWnd,TBM_GETPOS,0,0);
			Delay.leftc=trackpos/200.0;
			Delay.rightc=trackpos/200.0;

			hTrackWnd=GetDlgItem(hdwnd,IDC_SLIDER2);
			trackpos=SendMessage(hTrackWnd,TBM_GETPOS,0,0);
			Delay.Delay16thBeats=trackpos;


			//	trackpos=SendMessage(Trackbar[i].hPan,TBM_GETPOS,0,0);
			//	Drum[LoadedDrums[i]].pan=(int)trackpos-128;
			break;
		}
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hdwnd,0);
			return (1);
			break;
		case IDOK:
			EndDialog(hdwnd,0);
			return (1);
			break;
		case IDC_CHECK1:
		 if (Delay.On==1) {
			 Delay.On=0;
		 } else {
			 Delay.On=1;
		 }
		break;

		
		}

	}
return (0);
}
