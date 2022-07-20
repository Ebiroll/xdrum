/*
 * keyboard.h
 * 
 * Include file for the keyboard
 * 
 * $Date: 1996/04/08 18:40:33 $
 * 
 * $Revision: 0.1 $
 * 
 * 
 */ 

#ifndef _KEYBOARD_H
#define _KEYBOARD_H




class Area {
 public:
   Area(){};
   Area(int leftx,int rightx,int lowy,int highy){
      
      LeftX=leftx;
      RightX=rightx;
      LowY=lowy;
      HighY=highy;

   };
   
   int IsInArea(int x,int y){
      return(((x>LeftX)&&x<(RightX))&&((y>LowY)&&(y<HighY)));
   };
   
   int LeftX;
   int RightX;		   
   int LowY;
   int HighY;
   
};







class Key {
public:
  
  Key();
  int IsInArea(int x,int y);
//  int Draw(Octave *T);
//  int Clear(Octave *T);
  
  int IsPressed;
  Area *Area1;
  Area *Area2;
  Area *Area3;
};




//class Octave : public O_Widget {
//public:
//  Octave();
//  Octave(O_Widget *parent,SimpleCallback Func=NULL,void* data=NULL);
//  void SetDirector(void *data);  // To remember who owns this octave

//  virtual void Octave::SetSize(int x,int y);
  
//  int AddLine(int from_x,int from_y,int to_x,int to_y);
//  int DrawLine(int from_x,int from_y,int to_x,int to_y);
//  void Recalculate(void);
//  void DoExpose(void);
  
//  void half(int i,int keynum,int Height,int Width);
//  void nmumd(int i,int keynum,int Height,int Width);
//  void mumd(int i,int keynum,int Height,int Width);
//  void munmd(int i,int keynum,int Height,int Width);
  // Polygons could be drawn wit Xdrawlines
       

//       Key Keys[14];
  
//  int FromX[700];
//  int FromY[700];
//  int ToX[700];
//  int ToY[700];
//  int PI;	
//  int Width;
//  int Height;
  
  
//  GC myGC;
  
//  int LastKey;
//  int IsPressed;
//  int NoteNum;
//  int BaseNote;
  
//  void *Director;    // Every octave here has an owner!!

  // This data doesnt really belong here
//  int PatternNum;
//  int SampleNum;
//  int Pan;
//  int Vol;
  
   
//  int Lastx;           // Last x-click
// int Lasty;           // Lasy y-click
//  int LastMouse;       // Last Left,Middle,Rigtht,LeftUp,MiddleUp or Right Up
//  int LastModifier;    // Shift,Ctrl or Alt(Meta)
//};



#endif
