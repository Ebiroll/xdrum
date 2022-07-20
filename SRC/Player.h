#ifndef _PLAYER_H
#define _PLAYER_H

#define MAX_VOICE    32


class Player
{

public:

	int LoadSample (int samplenum,char *name,int pan,int Type);
	void  ParseDrums(char *FileName);


	int StartPlayer(void);
	int StopPlayer(void);

	
	int InitPlayer(HWND hWnd);
	int CleanUpPlayer(void);

	void enableTB();
	void disableTB();

	static Player* instance() {

		if (mInstance==NULL)
		{
			mInstance= new Player();
		}
		return mInstance;
	}

	int getPlayPos();

	// Adds a voice for playing

	int voiceOn(short *data,float pan,float vol) 
	{
		int j=0;

		while ( j<MAX_VOICE)
		{
			if (!Voices[j].isPlaying()) 
				break;
			j++;
		}	

	}

	class Voice  {
	public:
		
		Voice(void)
		{
			count=-1;
			pointer=NULL;
			leftc=1.0;
			rightc=1.0;
		};

		bool isPlaying()
		{
			return (count>0);
			
		};

		int voice_on(short *data,float pan,float vol) {

		}

	public:

		short *pointer;
		int count;
		float leftc;
		float rightc;
	};


private:
	Voice Voices[MAX_VOICE];

	Player(){};
	static Player *mInstance;


};

#endif