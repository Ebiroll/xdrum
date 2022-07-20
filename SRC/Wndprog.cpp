
//
//       Win32 Version of XDRUM
//
//       Copyright © 2022 by olof Åstrand olof.astrand@gmail.com

//
//
//
//	This program is free software; you can redistribute it and/or
//	modify it under the terms of the GNU General Public License
//	as published by the Free Software Foundation; either version 2
//	of the License, or (at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//

#include "resource.h"
#include <string.h>
#include <stdio.h>
#include "windows.h"
#include "windowsx.h"
#include <commctrl.h>


#include "object.h"
#include "drum.h"
#include "gui.h"
#include "player.h"

//#define DEBUG

MainGui *GUI=NULL;
Object *OnOff;
Mixer	*TheMix;

Player *myPlayer=Player::instance();

int TheMaxDrum=0;


void StartPlay(void *Data);
BOOL CALLBACK AboutDialog(HWND hdwnd,UINT message,WPARAM wParam,LPARAM lparam); 

// External variables, from Player.
// In order to
extern int		BlockCounter; 
extern DWORD	BufferLength;
extern DWORD	MyWritePos;
extern tDelay Delay;

extern BOOL CALLBACK My3MUDialog(HWND hdwnd,UINT message,WPARAM wParam,LPARAM lparam);

//
int BPM=120;

tPattern Pattern[MAX_PATTERNS];
extern tDrum Drum[];

int PatternIndex=0;		/* OLAAS This is pattern being edited */


HBITMAP hPlay,hStop;


LRESULT CALLBACK MyMainWndProc( HWND, UINT, WPARAM, LPARAM );


HWND myWin;              // Handle for the top-level window
HINSTANCE myInstance;

void SetUpButtons(HINSTANCE hInstance);

void InsertNewSequence(short *s,int Length){

	TheMaxDrum++;
	GUI->NewSequence(TheMaxDrum,s,Length);

}



int CreateMenues (HWND hWnd) {
 	int k;
	HMENU hMenu=GetMenu(hWnd);
	HMENU hSub;

	   
	for (k=0;k<MAX_DRUMS;k++)  {
	  if (Drum[k].Name!=NULL) {
		 
			hSub=GetSubMenu(hMenu,Drum[k].col);

			AppendMenu(hSub,MF_STRING,ID_CUSTOM_BASE+k,
				Drum[k].Name);
		TheMaxDrum=k;
	  }
	}

	
 return(1);
}

int HandleCustomMenu(int Data){

//	char String[512];

//	sprintf(String,"Select %u",Data);
//	MessageBox(myWin,String,"Menu Selection",MB_OK);

	if (GUI) {
		if (GUI->IsDrumLoaded(Data-ID_CUSTOM_BASE)) {
			return(-1);
		}

		GUI->NewDrum(Data-ID_CUSTOM_BASE);
	}
 return(1);
}




void DrawPointers(HWND hWnd) {

	// A PAINTSTRUCT holds data about the client area being drawn in
	// It is filled by the BeginPaint() call, and re-read by the
	// EndPaint() call to tell Windows which area has been updated.
	// A brush object is used to fill closed shapes with colour.

	HBRUSH hBr;     // A brush for painting closed shapes
	COLORREF txtColour;
	int sx=80,sy=15; // start pos
	static	int rx1,rx2;   // Pointer pos
	int Length=250;


	PAINTSTRUCT myPS;
	HDC hDC;

	hDC = GetDC( hWnd );

//	txtColour=RGB(255,255,255);
//  	hBr = CreateSolidBrush( txtColour );
//	hBr = SelectObject( hDC, hBr );
// Erase Old lines
//	MoveToEx(hDC,rx1,sy+2,NULL);
//	LineTo(hDC,rx1,sy+10);

//	MoveToEx(hDC,rx2,sy+10,NULL);
//	LineTo(hDC,rx2,sy+18);


	txtColour=RGB(0,0,0);
  	hBr = CreateSolidBrush( txtColour );
	hBr = (struct HBRUSH__ *)SelectObject( hDC, hBr );


	MoveToEx(hDC,sx,sy,NULL);
	LineTo(hDC,sx,sy+20);
	LineTo(hDC,sx,sy+10);
	LineTo(hDC,sx+Length,sy+10);
	LineTo(hDC,sx+Length,sy);
	LineTo(hDC,sx+Length,sy+20);

	rx1=sx+(Length*MyWritePos/BufferLength);
	MoveToEx(hDC,rx1,sy+2,NULL);
	LineTo(hDC,rx1,sy+10);



	rx2=sx+(Length*2*BlockCounter/BufferLength);
	rx2=sx+(Length*(myPlayer->getPlayPos()%100000)/100000);
	MoveToEx(hDC,rx2,sy+10,NULL);
	LineTo(hDC,rx2,sy+18);


	rx2=sx+(((unsigned int)Delay.pointer*Length)/BufferLength);
	MoveToEx(hDC,rx2,sy+5,NULL);
	LineTo(hDC,rx2,sy+30);


	DeleteDC( hDC );


}


void CALLBACK DisplayData(HWND hWnd,UINT Id,WPARAM Wpar,LPARAM LPar) {
//	   SendData(DataSize);
//	 SetTimer(myWin,1,DelayTime,(TIMERPROC) DisplayData);
// Here we display data that has been collected
   DrawPointers(hWnd);
 //  InvalidateRect( hWnd, NULL, TRUE );
}

// Main entry point of the program

int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
			LPSTR lpCmdLine, int nCmdShow )
    {
	//
	// Variables used in the creation and handling of the top
	// level window in this application
	//

	int x = 100, y = 100;       // TLHC of window positioned here
	int sx = 450, sy = 250;     // Size of window
	LPSTR title = "Win Drum"; // Title bar text for window

	//
	//
	//

	if( hPrevInstance == NULL )
	    {
		//
		//
		WNDCLASS myWC;

		myWC.hInstance = hInstance; // Handle of application instance
		myWC.style = CS_DBLCLKS;    // Support double click messages
		myWC.lpfnWndProc = MyMainWndProc; // Main window function
		myWC.cbClsExtra = myWC.cbWndExtra = 0;
		myWC.hIcon = LoadIcon( 0, "WINDICON" );
		myWC.hCursor = LoadCursor( 0, IDC_ARROW );
		myWC.hbrBackground =(struct HBRUSH__ *) GetStockObject( WHITE_BRUSH );
		myWC.lpszMenuName = "wMenu";   // 
		myWC.lpszClassName = "Demo";      // Our unique class name
		RegisterClass( &myWC );     // Register this Class
	    }

	//
	//

	myWin = CreateWindow(
		"Demo", title,              // Class name and title bar text
		WS_OVERLAPPEDWINDOW, x, y,  // Normal top-level window
		sx, sy, (HWND)0, (HMENU)0,  // Has no parent, menu in class
		hInstance, NULL );

	//
	// ShowWindow() allows the border, title bar etc. to be drawn.
	// UpdateWindow() forces a request for the client area of your window
	// to be drawn.
	//

	myInstance=hInstance;
	SetUpButtons(hInstance);
	InitCommonControls();

	ShowWindow( myWin, nCmdShow );      // Window's first showing
	UpdateWindow( myWin );              // Redraw client area

	if (myPlayer->InitPlayer(myWin)<0) {

		MessageBox(myWin,"Wav device error Properly","See HELP",MB_OK);

	}

	GUI=new MainGui(myWin,hInstance);
	TheMix=new Mixer(myWin,hInstance);
	myPlayer->ParseDrums("DRUMS");

	CreateMenues(myWin);
	// See what happens 
	GUI->NewDrum(0);



#ifdef DEBUG
	SetTimer(myWin,1,100,(TIMERPROC) DisplayData);
#endif
//  Add This for debugging of the player
	
	
	InvalidateRect( myWin, NULL, TRUE );


	    {
		// A data structure to hold each incoming message
		MSG mess;

		// GetMessage() only returns FALSE when WM_QUIT arrives
		while( GetMessage( &mess, (HWND)0, 0, 0 ) )
		    {
			// Preprocess keyboard input. E.g. Convert key-down,
			// key-up pairs into WM_CHAR messages
			TranslateMessage( &mess );

			// Send the messages to the window function
			DispatchMessage( &mess );
		    }

		return mess.wParam;     // A traditional returned value
	    }
    }

//
// Prototypes for message cracker functions used in the window function.
// These are needed, so that the HANDLE_MSG() macros can use their names
// before the actual function bodies have been parsed by the compiler.
//

void Demo_OnDestroy( HWND hWnd );
void Demo_OnCommand( HWND hWnd, int menuID, HWND hCtrl, UINT notifyCode );
void Demo_OnLButtonDown( HWND hWnd, BOOL dblClk, int x, int y, UINT metas );
void Demo_OnPaint( HWND hWnd );
void Demo_OnTimer(HWND hwnd, UINT id);
void Demo_OnChar(HWND hwnd, TCHAR ch, int cRepeat);


//
// The window function for the main (and only!) window. This is
// called once, directly by Windows, for each incoming message
// arriving for the application.
//

LRESULT CALLBACK
    MyMainWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
    {
	switch( msg )
	    {
		// Sent when window is about to be annihilated
		HANDLE_MSG( hWnd, WM_DESTROY, Demo_OnDestroy );

		// Sent whenever window client area requires redrawing
		HANDLE_MSG( hWnd, WM_PAINT, Demo_OnPaint );

		// ##5 - Sent whenever any menu selection is made
		HANDLE_MSG( hWnd, WM_COMMAND, Demo_OnCommand );

		// Sent whenever a single mouse click occurs
		HANDLE_MSG( hWnd, WM_LBUTTONDOWN, Demo_OnLButtonDown );

		// Sent whenever a double click occurs
		// N.B. At the moment a bug in <windowsx.h> causes a compiler
		// warning in the next line!
		HANDLE_MSG( hWnd, WM_LBUTTONDBLCLK, Demo_OnLButtonDown );

		HANDLE_MSG(hWnd,WM_CHAR, Demo_OnChar);


	    default:
		return DefWindowProc( hWnd, msg, wParam, lParam );
	    }
    }

//
// The message cracker function that is called to handle destruction
// of the top level window when WM_DESTROY is received. Parameter types
// for these functions are looked up in C:\MSTOOLS\H\WINDOWSX.H, when you
// need to write a particular message cracker function.
//

void Demo_OnTimer(HWND hwnd, UINT id)
{

//	SendData(DataSize);
}


void Demo_OnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
	HDC hDC;

	hDC = GetDC( hwnd );
	if (!GUI) return;

	if (ch==' '){
		StartPlay(0);
	}

	if ((ch>='a') && (ch<='z'))
	{ 
		GUI->HandleKeyPress(ch,0);
		PatternIndex=ch-'a';
	} 
	
	if ((ch>='A') && (ch<='Z')) {
		GUI->HandleKeyPress(ch,1);
		PatternIndex=ch-'A';
	}

	
	InvalidateRect( myWin, NULL, TRUE );

}



void Demo_OnDestroy( HWND hWnd )
    {
	myPlayer->CleanUpPlayer();
	PostQuitMessage( 0 );   // Cause WinMain() to return from program
    }



//
// The message cracker function that is called to handle menu
// selections. The second parameter uniquely identifies which
// menu item was clicked on by a unique integer value.
// Note the third and fourth parameters below are unused in
// menu selections (Used for other things when WM_COMMAND is
// received from child controls instead).
//
extern BOOL CALLBACK DelayDialogFunc(HWND hdwnd,UINT message,
						 WPARAM wParam,LPARAM lparam);

void Demo_OnCommand( HWND hWnd, int menuID, HWND hCtrl, UINT notifyCode )
    {
	short *data;
	switch( menuID )        
	{
	case ID_FILES_MIXER:
			TheMix->PopUp();
		break;
	case ID_FILES_DELAY:
		 DialogBox(myInstance,"MYDBTEST",myWin,(DLGPROC)DelayDialogFunc);

		break;
	case ID_BPM_90:
			BPM=90;
		 break;
	case ID_BPM_100:
			BPM=110;
		 break;
	case ID_BPM_110:
			BPM=110;
		 break;
	case ID_BPM_120:
			BPM=120;
		 break;
	case ID_BPM_130:
			BPM=130;
		 break;
	case ID_BPM_140:
			BPM=130;
		 break;
	case ID_BPM_150:
			BPM=150;
		 break;
	case ID_BPM_160:
			BPM=160;
		 break;
	case ID_BPM_180:
			BPM=180;
		 break;
	case ID_FILES_GENERATETB303SOUND:
		DialogBox(myInstance,"MY3MU",myWin,(DLGPROC)My3MUDialog);
		
		//  Test3MU(&data);
		break;
	case ID_HELP_ABOUT:
		DialogBox(myInstance,"ABOUT_D",myWin,(DLGPROC)AboutDialog);
		break;
	case ID_FILES_QUIT:
		myPlayer->CleanUpPlayer();
		PostQuitMessage( 0 );  
		break;

	default:

		if ((menuID>ID_CUSTOM_BASE)&&(menuID<ID_CUSTOM_BASE+MAX_DRUMS)){
			HandleCustomMenu(menuID);
		}


		// We must pass on any unprocessed WM_COMMANDs
		// for DefWindowProc() to handle. There may be some from
		// other sources that we do not process.
		//
		FORWARD_WM_COMMAND( hWnd, menuID, hCtrl,
					notifyCode, DefWindowProc );
	}
	InvalidateRect( hWnd, NULL, TRUE );
}

// Message cracker function that handles left button down
// and also left button double-click messages.
// This routine merely causes a simple message to be
// displayed at the bottom of the window's client area.
//

//      Static variable used to hold string for painting
static char mouseButtonStr[128];

void Demo_OnLButtonDown( HWND hWnd, BOOL dblClk, int x, int y, UINT metas )
    {
	int i;
	// Update the mouseButtonStr string with mouse activity information
	sprintf( mouseButtonStr, "%s%s%sClicked at (%d,%d)",
			metas & MK_CONTROL ? "Ctrl-" : "",
			metas & MK_SHIFT ? "Shift-" : "" ,
			dblClk ? "Double-" : "",
			x, y );

	if (GUI) GUI->HandleClick(x,y);
    if (OnOff->HandleClick(x,y)) {
		// OnOff->Draw(hWnd);
		StartPlay(0);
	}

	
	
	//

//	InvalidateRect( hWnd, NULL, TRUE );
    }


// Message cracker function that handles painting requests from
// Win32. This is called every time a WM_PAINT message arrives.
//

void Demo_OnPaint( HWND hWnd )
    {
	int i;
	PAINTSTRUCT myPS;
	

	HDC hpDC = BeginPaint( hWnd, &myPS );  // Get a DC ha


	if (GUI!=NULL) GUI->HandlePaint(hWnd);
	OnOff->Draw(hWnd);

	EndPaint( hWnd, &myPS );


}


void StartPlay(void *Data) {
static int Playing=0;

	if (Playing==0) {
		myPlayer->StartPlayer();
		OnOff->SetState(1);
		OnOff->Draw(myWin);
		Playing=1;
	} else {
		myPlayer->StopPlayer();
		OnOff->SetState(0);
		OnOff->Draw(myWin);
		Playing=0;
	}
}

void SetUpButtons(HINSTANCE hInstance) {
	
	hPlay=LoadBitmap(hInstance,"PLAY");
	hStop=LoadBitmap(hInstance,"STOP");

	OnOff=new Object(10,10,hPlay,hInstance);
	OnOff->SetToggleBitmap(hStop);
//	OnOff->SetCallBackFunc(StartPlay,(void *)NULL);
	
}



//
// This function is an About dialog..
//

BOOL CALLBACK AboutDialog(HWND hdwnd,UINT message,
						 WPARAM wParam,LPARAM lparam) {
 
	switch (message) {
		 case WM_LBUTTONDOWN:
		 break;
	case WM_PAINT:
		{
			PAINTSTRUCT myPS;
			HDC hpDC = BeginPaint( hdwnd, &myPS );
			EndPaint( hdwnd, &myPS );
		}
		return(1);
	case WM_INITDIALOG:
		return (1);
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
			case IDOK:
				EndDialog(hdwnd,0);
				return (1);
			case IDCANCEL:
				EndDialog(hdwnd,0);
				break;
		}
	break;
	}
return (0);
}

