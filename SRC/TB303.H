

struct TB
{
	int Tune, Freq, Reso, EnvMod, Decay, Accent;
	int Tempo, Volume, Distort;

	int square, bpf, nonlin, filtersam;

	double hpffreq, hpfq, sqrrise, sqrfall, sawrise;

	//-------------------

	double tune, freq, reso, envmod, decay, accent;
	double attack, volume;

	int notelength;
};

extern void FX_TB303(short *from, short *to, int len, struct TB *tb);
