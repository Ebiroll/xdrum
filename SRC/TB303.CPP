/*
 * TB303 simulator
 *
 * Lars Hamre, lars@kswaves.com
 */

#include <windows.h>
#include <commctrl.h>
#include <math.h>
#include "tb303.h"

// #include "resource.h"






/*** Global data ******/

static struct TB		*CurrSettings;
static struct SamInfo	*CurrSample;

extern char Note[];

extern char NoteLength[];

extern char Slide[];

/*
 * 0 = no accent
 * 1 = accent on
 *
 * Accent is not used yet, but increases volume and resonance
 */

char Accent[16] =
{
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
};


#define OFS(a) struct_offset(struct TB, a)

//static struct APControl Controls[] =
//{
//	ASLIDER(-24,  24,  0, 1, IDC_TUNE, IDC_EDITTUNE,           OFS(Tune)),
//	ASLIDER(  0, 100,  0, 1, IDC_CUTOFF, IDC_EDITCUTOFF,       OFS(Freq)),
//	ASLIDER(  0, 100, 25, 1, IDC_RESONANCE, IDC_EDITRESONANCE, OFS(Reso)),
//	ASLIDER(  0, 100, 95, 1, IDC_ENVMOD, IDC_EDITENVMOD,       OFS(EnvMod)),
//	ASLIDER(  0, 100, 56, 1, IDC_DECAY, IDC_EDITDECAY,         OFS(Decay)),
//	ASLIDER(  0, 100,  0, 1, IDC_ACCENT, IDC_EDITACCENT,       OFS(Accent)),
//
//	ASLIDER( 30, 300,133, 1, IDC_TEMPO, IDC_EDITTEMPO,         OFS(Tempo)),
//	ASLIDER(  0, 100, 70, 1, IDC_VOLUME, IDC_EDITVOLUME,       OFS(Volume)),
//
//	ACHECK(IDC_SQUARE, 0, OFS(square)),
//	ACHECK(IDC_BANDPASS, 0, OFS(bpf)),
//	ACHECK(IDC_NONLINEAR, 0, OFS(nonlin)),
//	ACHECK(IDC_FILTERSAM, 0, OFS(filtersam)),
//
//	0
//};



/*
BOOL CALC_TB303(int keys)
{
	struct SamInfo *si;
	struct TB *tb;

//	Expand_PLUGIN_CalcSettings(si, tb, keys);
//	ASLIDER(-24,  24,  0, 1, IDC_TUNE, IDC_EDITTUNE,           OFS(Tune)),
//	ASLIDER(  0, 100,  0, 1, IDC_CUTOFF, IDC_EDITCUTOFF,       OFS(Freq)),
//	ASLIDER(  0, 100, 25, 1, IDC_RESONANCE, IDC_EDITRESONANCE, OFS(Reso)),
//	ASLIDER(  0, 100, 95, 1, IDC_ENVMOD, IDC_EDITENVMOD,       OFS(EnvMod)),
//	ASLIDER(  0, 100, 56, 1, IDC_DECAY, IDC_EDITDECAY,         OFS(Decay)),
//	ASLIDER(  0, 100,  0, 1, IDC_ACCENT, IDC_EDITACCENT,       OFS(Accent)),
//
//	ASLIDER( 30, 300,133, 1, IDC_TEMPO, IDC_EDITTEMPO,         OFS(Tempo)),
//	ASLIDER(  0, 100, 70, 1, IDC_VOLUME, IDC_EDITVOLUME,
	tb->tune   = (Controls[0].val) / 12.0;
	tb->freq   = Controls[1].val / 100.0;
	tb->reso   = Controls[2].val / 100.0;
	tb->envmod = (Controls[3].val / 100.0); // * (1.0 - tb->freq);
	tb->decay  = 1.0 - pow(10.0, -2 -4 * (Controls[4].val / 100.0));
	tb->accent = Controls[5].val / 100.0;

	tb->notelength = (int)((44100 * 0.125 * 120) / Controls[6].val);
	tb->volume     = Controls[7].val / 100.0;

	tb->hpffreq    = 0.92;
	tb->hpfq       = 1.00;;
	tb->sqrrise    = 0.55;
	tb->sqrfall    = 0.45;
	tb->sawrise    = 0.01;

	return TRUE;
}
*/

#define NOTELEN 4950


void FX_TB303(short *from, short *to, int len, struct TB *tb)
{
	static int tot=0, out2, notenum=0, noteofs;
	static double out;

	static double saw = 0, vco = 1.1;
	static double vcophase=0;
	static double vcofreq = 1, lastvcofreq=0, vcoadd, vcofreq2=0;

	static double hig=0, mid=0, low=0, freq = 0;
	static double hig2=0, mid2=0, low2=0;
	static double feedbk, flevel=0.01;

	static double amp=0, env=0, elow=0, aenv=0, inp;
	static int emode = 2;

	static double reso, trigger;

	while (len--)
	{
		noteofs = tot % tb->notelength;

		// Envelope trigger signal, TC: 0.94 ms

		if (NoteLength[notenum] == 1)
		{
			if (noteofs == 0)
			{
				emode = 1;
				trigger = 1;
			}
			else if (noteofs == 50)
				trigger = 0;
		}

		// VCF / Accent envelope, TC: up 0.1 ms, down 200 - 2500 ms

		if (trigger)
			env = env + (trigger - env) * 0.5;
		else
			env *= tb->decay;

		// VCA envelope, TC: lowpass 1.55 ms, down 1500 ms

		// Trig envelope release
		if (noteofs >= tb->notelength*11/20)
			if (NoteLength[(notenum+1)%16] != 2)
				emode = 3;

		if (trigger)
		{
			elow = elow + (trigger - elow) * 0.5;
			aenv = aenv + (elow - aenv) * 0.1;
		}
		else
		{
			if (emode == 3)
				aenv = aenv * 0.994;
			else
				aenv = aenv * 0.99997;
		}

		// Update VCO frequency

		if (noteofs == 0)
		{
			vcofreq = Note[notenum] / 12.0;
		}

		if (Slide[notenum])
			vcofreq2 = vcofreq2 + (vcofreq - vcofreq2) * 0.001;
		else
			vcofreq2 = vcofreq;

		if (vcofreq2 != lastvcofreq)
			vcoadd = 0.05 * pow(2.0, vcofreq2 - 2.34 + tb->tune);

		lastvcofreq = vcofreq2;

/*
 *
 * VCO
 *
 */
		// If we are in reset mode, charge capacitor C33
		// until we reacharge above the maximum level.
		// I don't know what level, but let's say 12V - 0.7V = 11.3V

		if (vco > 5.5)
		{
			// Charge the VCO capacitor. This is probably not
			// 100% accurate, but I don't want to model saturation
			// of the transistors. The discharnge should NOT be
			// disabled when resetting, but I will not complicate
			// things at this point.

			saw = saw + (vco - saw) * tb->sqrrise;

			// If we hit maximum, go to discharge mode.

			if (saw >= 11.3)
				vco = 5.333;
		}
		else
		{
			// We are falling down and generating something that
			// looks somewhat like a sawtooth, but it's not linear.

			saw = saw + (vco - saw) * tb->sqrfall * vcoadd;

			// When we hit the lower trigger level, go to reset mode.
			// This happens at about 5.5V in the TB303. Discharge level
			// is at 5.333V.

			if (saw <= 5.5)
				vco = 12.0;
		}
													  
		if (tb->square)
		{
		//	if (saw > 7.0)
		//		sqr = sqr + (saw - sqrvco2) * tb->sqrrise; // 0.95
		//	else
		//		vco2 = vco2 + (vco - vco2) * tb->sqrfall; // 0.90
		}

		// Simulate the highpass filter with C17 removing the DC
		// part of the VCO waveform.
		{
			static double delta=0;
			delta += saw;
			delta = delta * (0.97 + 0.03 * tb->hpffreq); // 0.980;
			inp   = delta;
			//delta -= out; // this is a bug, but sounds really funky!
			delta -= saw;
		}

		if (tb->filtersam)
			inp = *from++ / 32768.0;


//---------- VCF ----------

		// Initial cutoff freq + envelope amount

		freq = tb->freq + tb->envmod * env;
		if (freq > 1.0) freq = 1.0;
		freq = freq * freq * freq * freq;

		// Moog-style 4 pole VCF
		{
			static double o0,o1,o2,o3,o4;
			static double a1,a2,a3,a4;
			static double i1,i2,i3,i4;

			reso = (1 - freq);
			reso = (1 + 3.5*reso*reso);
//			reso = (1 + 10*reso*reso);
			reso = reso * tb->reso;

			freq = freq * 2.0 - 1.0;

			reso = reso * o4;

			if (tb->nonlin)
			{
				double lim = 10.0 * tb->sawrise;
				reso *= 2.0;
				if (reso >  lim) reso =  lim + sqrt(reso - lim);
				if (reso < -lim) reso = -lim - sqrt(lim - reso);
			}
			else
			{
				if (reso > 5.0) reso = 5.0;
				else if (reso < -5.0) reso = -5.0;
			}
			

			o0 = inp - reso;

			a1 = (o0 - a1) * freq + i1;
			i1 =  o0;
			o1 = (o0 + a1) * 0.5;

			a2 = (o1 - a2) * freq + i2;
			i2 =  o1;
			o2 = (o1 + a2) * 0.5;

			a3 = (o2 - a3) * freq + i3;
			i3 =  o2;
			o3 = (o2 + a3) * 0.5;

//			o4 = o3;
			a4 = (o3 - a4) * freq + i4;
			i4 =  o3;
			o4 = (o3 + a4) * 0.5;

			low = o4;
		}

//---------- VCA ----------

		// Capacitor charge attack...
		//amp = amp + (1.1 - amp) * 0.01;

		switch (emode)
		{
			// First attack state
			case 0:
				// The TB303 attack looks more like this
				amp = amp*1.07 + 0.007;
				if (amp >= 0.6)
				{
					amp = 0.6;
					emode = 1;
				}
				break;

			// Secondary slow attack (bloom)
			case 1:
			//	amp = amp + (1.01 - amp) * 0.0042 * tb->decay;
				amp = amp + (1.01 - amp) * 0.42 * tb->decay;
				if (amp >= 1.0)
				{
					amp = 1.0;
					emode = 2;
				}
				break;

			// Decay state - tweak as you like
			case 2:
				amp = amp * tb->decay;  //0.99997;
				break;

			// Release state
			case 3:
				amp = amp * 0.994;
				//amp = amp * tb->decay;
				break;
		}

		// Output is lowpass (try low or low2) multiplied by amplitude.

		out = low * aenv;
/*
		if (tb->bpf)
			out = mid2 * amp;
		else
			out = hig2 * amp;
*/
		// Add a tiny click at the beginning of each note

		if (noteofs == 0)
			out += 0.020;
		else if (noteofs == 1)
			out += 0.020 * 0.7;

		// Final  output volume

		out *= 6000 * tb->volume;

//		out = 32767 * aenv;

		// Highpass to simulate output coupling capacitor
#if 1
		{
			static double delta=0;
			delta += out;
			delta = delta * (0.97 + 0.03 * tb->hpffreq); // 0.980;
			out2 = (int)delta;
			delta -= out;
		}
#else
		out2 = (int)out; // (int)(vco2*16384.0);
#endif

//-------- Output ----------

//		out2 = (int)(low * 32768.0);

		// Clip to min/max values
//		Clip16bit(out2);
		if (out2>32767) out2=32767;
		if (out2<-32767) out2=-32767;

		*to++ = out2;

		tot++;

//------------------------

		// At the end of a note, start another one
		if (noteofs == tb->notelength-1)
		{
			notenum++;
			if (notenum >= 16)
				notenum = 0;
		}
	}
}


/*** TB303 effect ********************************/

#if 0
/*
ULONG DoTB303(PLUGIN *plug, ULONG method, KEYS keys)
{
	struct SamInfo *si;
	void *tb;
	short *from, *to;
	ULONG len;
	ULONG result = 0;

	switch (method)
	{
	case Do_PLUGIN_GetName:
		return (ULONG) "TB303";

	case Do_PLUGIN_GetDescription:
		return (ULONG) "Simulates a Roland TB303. The overhyped and silvergray resonant techo-bass box.";

	// Check if format is supported
	case MTD_PLUGIN_IsFormatSupported:
		{
			SAMINFO *si;
			Expand_PLUGIN_IsFormatSupported(si, keys);
			if (VAL_SAMPLE_Bits(si) != 16)
				result |= FORMATERR_BITS;
			if (VAL_SAMPLE_Stereo(si) == TRUE)
				result |= FORMATERR_CHANNELS;
		}
		break;


	// Create a settings struct.
	case Do_PLUGIN_CreateSettings:
		result = (ULONG) GlobalAlloc(GMEM_FIXED, sizeof(struct TB));
	//	if (result)
	//		TB_GUIToSettings((struct TB *) result);
		break;

	case MTD_PLUGIN_ShowSettings:
		{
			HWND	ParentWin;	// Parent window
			Expand_PLUGIN_ShowSettings(CurrSample, CurrSettings, ParentWin, keys);

			result = DialogBox(plug->PluginLib->Instance,
							   MAKEINTRESOURCE(DIALOG_TB303),
							   ParentWin,
							   (DLGPROC)TB303WinProc);

//			TB_GUIToSettings(tb);
		}
		break;

	case MTD_PLUGIN_CalcSettings:
		result = CALC_TB303(keys);
		break;

	// Initialize things before processing
	case MTD_PLUGIN_InitProcess:
	{
		struct SamInfo *si;
		struct TB *tb;
		ULONG offset, length;

		Expand_PLUGIN_InitProcess(si, tb, offset, length, keys);

		result = TRUE;
	}
	break;


	// Process a block of data
	case MTD_PLUGIN_ProcessBlock:
		Expand_PLUGIN_ProcessBlock(si, tb, from, to, len, keys);

		if (VAL_SAMPLE_Bits(si) == 16 && !VAL_SAMPLE_Stereo(si))
		{
			FX_TB303(from, to, len, (TB *)tb);
			result = TRUE;
		}
		break;

	default:
		result = DoSuperMethodA(plug, method, keys);
		break;
	}
	return result;
}

*/
#endif