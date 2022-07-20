// 
// A better name for Object would have been ToggleObject
//
#include "object.h"
#include <windows.h>

Object::Object(int x,int y,HBITMAP hB,HANDLE hInstance) {
		
	this->x=x;
	this->y=y;
	sizex=20;
	sizey=20;
	hBit=hB;
	hAltBit=NULL;
	hDrawBit=hBit;
	Func=NULL;
	Data=NULL;
	this->hInstance=hInstance;
};


BOOL Object::HandleClick(int x,int y){
	if((this->x<x)&&(this->y<y)&&(x<this->x+sizex)&&(y<this->y+sizey)) {
		if (Func!=NULL) {
			(*Func)(Data);
		}
		if (hAltBit!=NULL) {
			if (hDrawBit==hAltBit) {
				hDrawBit=hBit;	
			} else {
				hDrawBit=hAltBit;	
			}

		return(TRUE);
		}
	}
	return(FALSE);
};

int Object::SetCallBackFunc(CallBackFn *fn,void *Val){
	Data=Val;
	Func=fn;
	return(1);
};


int Object::SetToggleBitmap(HBITMAP hB){	
	hAltBit=hB;	
return(1);
};


int Object::Draw(HWND hWnd) {

	HDC DC,memDC;

	DC=GetDC(hWnd);
	memDC=CreateCompatibleDC(DC);
	SelectObject(memDC,hDrawBit);
	BitBlt(DC,x,y,sizex,sizey,memDC,0,0,SRCCOPY);
	ReleaseDC(hWnd,DC);
	DeleteDC(memDC);

	return(1);
};

int Object::SetState(int i){
	
	if (hAltBit!=NULL) {
		if (i==0) {
			hDrawBit=hBit;	
		} else {
			hDrawBit=hAltBit;	
		}
	}

	return(1);
};

