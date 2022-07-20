#ifndef _OBJECT_H
#define _OBJECT_H
#include <windows.h>


typedef  void (CallBackFn) (void *);

class Object {
public:
	Object(int x,int y,HBITMAP hB,HANDLE hInstance);
	BOOL HandleClick(int x,int y);
	int SetCallBackFunc(CallBackFn *fn,void *Val);
	int SetToggleBitmap(HBITMAP hB);
	int Draw(HWND hWnd);
	int SetState(int i);
private:
	int x,y;
	int sizex,sizey;
	HBITMAP hBit;
	HBITMAP hAltBit;
	HBITMAP hDrawBit;
	CallBackFn *Func;
	void *Data;
	HANDLE hInstance;
};


#endif