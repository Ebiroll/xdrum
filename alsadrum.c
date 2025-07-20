/*
 * alsadrum.c - Drum player for Linux using ALSA
 * (C) Olof Astrand, 2025
 * 
 * Modernized version using ALSA instead of OSS /dev/dsp
 * Cleaned up code structure and removed GUS-specific code
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include <alsa/asoundlib.h>

#include "gdrum.h"
#include "drum.h"

/* Configuration */
#define MAX_VOICE       32
#define BUFFER_SIZE     16384
#define MAX_DELAY       200000
#define SLEN            265535
#define MAX_DRUMS       64
#define MAX_PATTERNS    32

#ifndef DRUMS_ROOT_DIR
#define DRUMS_ROOT_DIR  "."
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#ifdef DEBUG
#define debug(f, x) fprintf(stderr, f, x)
#else
#define debug(f, x) {}
#endif

/* Global variables */
static snd_pcm_t *pcm_handle = NULL;
static snd_pcm_hw_params_t *hw_params = NULL;
static unsigned int sample_rate = 44100;
static unsigned int channels = 1;
static snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;

/* Audio buffer management */
static int left_of_block = BUFFER_SIZE;
static int block_counter = 0;
static unsigned char data[BUFFER_SIZE];
static long tmpstore[SLEN];

/* Delay effect */
static short delay_data[MAX_DELAY];
static int delay_pointer = 0;

/* Voice structure */
typedef struct {
    short *pointer;
    int count;
    int length;
    int on;
} Voice_t;

static Voice_t voices[MAX_VOICE];

/* Global pattern and drum data */
tDelay delay;
tDrum         Drum[MAX_DRUMS];
tPattern      Pattern [MAX_PATTERNS];


/* Timing variables */
static int beat = 0;
static int pattern_index = 0;
extern int BPM;
extern int M;

/* Function prototypes */
static int open_alsa_device(void);
static void close_alsa_device(void);
int load_sample(int samplenum, const char *name, int pan, int intel_format);
static void play_drum(int sample);
static void parse_drums(const char *filename);
static void init_pattern(void);
static void play_pattern(int patt, int measure);
static int voice_on(int drum_index);
static int add_one_beat(int length);
static int write_audio_buffer(void);
char *add_root(char *name);
static int stop_playing(void);

void starttimer(void) {

}


int StopPlaying(void) {
   return stop_playing();
}
void PlayPattern(int Patt, int Measure) {
   play_pattern(Patt, Measure);
}

int EmptyCard (void)
{
   int i;
   
   for(i=0;i<MAX_DRUMS;i++)  {
      Drum[i].Loaded=0;
   }
/* TODO fix the memory leakage */
      
   
}


/* Open ALSA device for playback */
static int open_alsa_device(void)
{
    int err;
    unsigned int actual_rate;
    snd_pcm_uframes_t frames;

    /* Open PCM device for playback */
    err = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        fprintf(stderr, "Cannot open audio device: %s\n", snd_strerror(err));
        return -1;
    }

    /* Allocate hardware parameters object */
    snd_pcm_hw_params_alloca(&hw_params);

    /* Fill it with default values */
    err = snd_pcm_hw_params_any(pcm_handle, hw_params);
    if (err < 0) {
        fprintf(stderr, "Cannot initialize hardware parameter structure: %s\n", snd_strerror(err));
        return -1;
    }

    /* Set access type */
    err = snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        fprintf(stderr, "Cannot set access type: %s\n", snd_strerror(err));
        return -1;
    }

    /* Set sample format */
    err = snd_pcm_hw_params_set_format(pcm_handle, hw_params, format);
    if (err < 0) {
        fprintf(stderr, "Cannot set sample format: %s\n", snd_strerror(err));
        return -1;
    }

    /* Set channel count */
    err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, channels);
    if (err < 0) {
        fprintf(stderr, "Cannot set channel count: %s\n", snd_strerror(err));
        return -1;
    }

    /* Set sample rate */
    actual_rate = sample_rate;
    err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &actual_rate, 0);
    if (err < 0) {
        fprintf(stderr, "Cannot set sample rate: %s\n", snd_strerror(err));
        return -1;
    }
    if (actual_rate != sample_rate) {
        fprintf(stderr, "The rate %u Hz is not supported, using %u Hz instead\n", 
                sample_rate, actual_rate);
        sample_rate = actual_rate;
    }

    /* Set period size */
    frames = BUFFER_SIZE / (channels * 2); /* 2 bytes per sample for S16 */
    err = snd_pcm_hw_params_set_period_size_near(pcm_handle, hw_params, &frames, 0);
    if (err < 0) {
        fprintf(stderr, "Cannot set period size: %s\n", snd_strerror(err));
        return -1;
    }

    /* Apply hardware parameters */
    err = snd_pcm_hw_params(pcm_handle, hw_params);
    if (err < 0) {
        fprintf(stderr, "Cannot set parameters: %s\n", snd_strerror(err));
        return -1;
    }

    /* Prepare PCM for use */
    err = snd_pcm_prepare(pcm_handle);
    if (err < 0) {
        fprintf(stderr, "Cannot prepare audio interface: %s\n", snd_strerror(err));
        return -1;
    }

    /* Initialize voices */
    for (int i = 0; i < MAX_VOICE; i++) {
        voices[i].count = -1;
    }

    return 0;
}

/* Close ALSA device */
static void close_alsa_device(void)
{
    if (pcm_handle) {
        snd_pcm_drain(pcm_handle);
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
    }
}

/* Write audio buffer to ALSA */
static int write_audio_buffer(void)
{
    snd_pcm_sframes_t frames_written;
    int frames_to_write = BUFFER_SIZE / (channels * 2); /* 2 bytes per sample */

    frames_written = snd_pcm_writei(pcm_handle, data, frames_to_write);
    
    if (frames_written < 0) {
        /* Handle underrun */
        if (frames_written == -EPIPE) {
            fprintf(stderr, "ALSA underrun occurred\n");
            snd_pcm_prepare(pcm_handle);
        } else {
            fprintf(stderr, "Write error: %s\n", snd_strerror(frames_written));
            return -1;
        }
    } else if (frames_written < frames_to_write) {
        fprintf(stderr, "Short write: wrote %ld of %d frames\n", 
                frames_written, frames_to_write);
    }

    return 0;
}

/* Load a sample file */
int load_sample(int samplenum, const char *name, int pan, int intel_format)
{
    int sample_fd;
    int len;
    char tmpdata[SLEN];
    char tmpdata2[SLEN];

    if ((sample_fd = open(name, O_RDONLY, 0)) == -1) {
        perror(name);
        return -1;
    }

    len = read(sample_fd, tmpdata, SLEN);
    
    assert(len > 0);
    assert(len < SLEN);

    Drum[samplenum].length = len;
    Drum[samplenum].sample = calloc(len + 4, sizeof(short));

    if (!intel_format) {
        /* Swap bytes for big-endian data */
        for (int j = 0; j < len - 1; j += 2) {
            tmpdata2[j + 1] = tmpdata[j];
            tmpdata2[j] = tmpdata[j + 1];
        }
        memcpy(Drum[samplenum].sample, tmpdata2, len);
    } else {
        memcpy(Drum[samplenum].sample, tmpdata, len);
    }

    Drum[samplenum].Loaded = 1;
    close(sample_fd);
    
    return samplenum;
}

/* Initialize pattern data */
static void init_pattern(void)
{
    /* Clear drum data */
    for (int i = 0; i < MAX_DRUMS; i++) {
        Drum[i].Filename = NULL;
        Drum[i].Loaded = 0;
    }

    /* Clear pattern data */
    for (int k = 0; k < MAX_PATTERNS; k++) {
        for (int j = 0; j < MAX_DRUMS; j++) {
            Pattern[k].DrumPattern[j].Drum = NULL;
            for (int i = 0; i < 16; i++) {
                Pattern[k].DrumPattern[j].beat[i] = 0;
            }
        }
    }
}

/* Add drums root directory to filename */
char *add_root(char *name)
{
    static char drum_name[512];
    char *env;

    if ((env = getenv("DRUMS_ROOT")) == NULL) {
        strcpy(drum_name, DRUMS_ROOT_DIR);
    } else {
        strcpy(drum_name, env);
    }

    strcat(drum_name, "/");
    strcat(drum_name, name);
    strcpy(name, drum_name);
    
    return name;
}

/* Parse drums configuration file */
static void parse_drums(const char *filename)
{
    FILE *drum_fd;
    int sn = 0;
    char buff[512];
    char name[512];
    char filepath[512];
    int vol, pan, col, intel, key;

    strcpy(filepath, filename);
    add_root(filepath);

    if ((drum_fd = fopen(filepath, "r")) == NULL) {
        perror(filepath);
        return;
    }

    while (fgets(buff, 512, drum_fd)) {
        if (sscanf(buff, "%s\t%s\t%d\t%d\t%d\t%d\t%d", 
                   name, filepath, &col, &pan, &vol, &intel, &key) == 7) {
            
            if (filepath[0] != '/') {
                add_root(filepath);
            }

            /* Setup drum data */
            Drum[sn].SampleNum = sn;
            Drum[sn].Name = strdup(name);
            Drum[sn].Filename = strdup(filepath);
            Drum[sn].col = col;
            Drum[sn].pan = pan;
            Drum[sn].vol = vol;
            Drum[sn].key = key;
            Drum[sn].Filetype = intel;

            /* Link drum to all patterns */
            for (int i = 0; i < MAX_PATTERNS; i++) {
                Pattern[i].DrumPattern[sn].Drum = &Drum[sn];
            }

            sn++;
        }
    }

    fclose(drum_fd);
}

/* Start playing a voice */
static int voice_on(int drum_index)
{
    int j = 0;

    /* Find free voice */
    while ((voices[j].count > -1) && (j < MAX_VOICE)) {
        j++;
    }

    if (j == MAX_VOICE) {
        fprintf(stderr, "No free voices available\n");
        return -1;
    }

    /* Load sample if not already loaded */
    if (Drum[drum_index].Loaded == 0) {
        if (load_sample(drum_index, Drum[drum_index].Filename, 
                       Drum[drum_index].pan, Drum[drum_index].Filetype) < 0) {
            return -1;
        }
    }

    voices[j].pointer = Drum[drum_index].sample;
    voices[j].count = 0;
    voices[j].length = Drum[drum_index].length;

    return j;
}

/* Add one beat worth of audio data */
static int add_one_beat(int length)
{
    int crossed_limit = 0;

    /* Clear temporary buffer */
    memset(tmpstore, 0, (length + 1) * sizeof(long));

    /* Mix all active voices */
    for (int voi = 0; voi < MAX_VOICE; voi++) {
        if (voices[voi].count > -1) {
            int sound_length = voices[voi].length - voices[voi].count;
            if (sound_length > length) {
                sound_length = length;
            }

            if (sound_length == 0) {
                voices[voi].count = -1;
            } else {
                for (int j = 0; j <= sound_length; j++) {
                    tmpstore[j] += *voices[voi].pointer;
                    voices[voi].pointer++;
                }
                voices[voi].pointer--;

                if (sound_length == length) {
                    voices[voi].count += sound_length;
                } else {
                    voices[voi].count = -1;
                }
            }
        }
    }

    /* Apply delay effect if enabled */
    if (delay.On == 1) {
        int delay_offset = (delay.Delay16thBeats / 16) * length;
        if (delay_pointer - delay_offset > 0) {
            delay.pointer = &delay_data[delay_pointer - delay_offset];
        }
    }

    /* Convert to output format and fill buffer */
    for (int j = 0; j <= length; j++) {
        /* Apply delay */
        if (delay.On == 1) {
            tmpstore[j] += (long)((float)delay.leftc * (float)(*delay.pointer));
            delay.pointer++;
            
            if (delay.pointer >= &delay_data[MAX_DELAY]) {
                delay.pointer = &delay_data[0];
            }
        }

        /* Clip and convert to 16-bit */
        short sample_value;
        if (tmpstore[j] > 32766) {
            sample_value = 32767;
        } else if (tmpstore[j] < -32767) {
            sample_value = -32768;
        } else {
            sample_value = (short)tmpstore[j];
        }

        /* Store in output buffer (little-endian) */
        data[block_counter] = (unsigned char)(sample_value & 0xFF);
        data[block_counter + 1] = (unsigned char)((sample_value >> 8) & 0xFF);

        /* Update delay buffer */
        delay_data[delay_pointer] = sample_value;
        delay_pointer = (delay_pointer + 1) % MAX_DELAY;

        left_of_block -= 2;
        block_counter += 2;

        /* Write buffer if full */
        if (left_of_block < 1) {
            crossed_limit = 1;
            write_audio_buffer();
            left_of_block = BUFFER_SIZE;
            block_counter = 0;
        }
    }

    return crossed_limit;
}

/* Play a pattern */
static void play_pattern(int patt, int measure)
{
    int sample_len;
    int crossed_limit, next_patt;
    static int remember_patt;

    if (beat > 0) {
        next_patt = patt;
        patt = remember_patt;
    }

    /* Calculate length of 1/4 beat in samples */
    sample_len = 660000 / BPM;

    for (int i = beat; i < 16; i++) {
        /* Trigger drums for this beat */
        for (int j = 0; j < MAX_DRUMS; j++) {
            if (Pattern[patt].DrumPattern[j].Drum != NULL) {
                if (Pattern[patt].DrumPattern[j].beat[i] > 0) {
                    voice_on(Pattern[patt].DrumPattern[j].Drum->SampleNum);
                }
            }
        }

        beat++;
        crossed_limit = add_one_beat(sample_len + 1);
        
        if (crossed_limit == 1) {
            break;
        }
    }

    if (beat > 15) {
        beat = 0;
        M++;
        patt = next_patt;
    }

    remember_patt = patt;
}

/* Stop playing and finish remaining audio */
static int stop_playing(void)
{
    int max_j = 0;
    int crossed_limit = 0;

    memset(tmpstore, 0, (BUFFER_SIZE + 1) * sizeof(long));

    /* Process remaining audio from active voices */
    for (int voi = 0; voi < MAX_VOICE; voi++) {
        if (voices[voi].count > -1) {
            for (int j = 0; j <= BUFFER_SIZE; j++) {
                if (voices[voi].count > voices[voi].length) {
                    if (j > max_j) {
                        max_j = j;
                    }
                    voices[voi].count = -1;
                    break;
                } else {
                    tmpstore[j] += *voices[voi].pointer;
                    voices[voi].pointer++;
                    voices[voi].count++;
                }
            }
        }
    }

    /* Convert and output remaining samples */
    for (int j = 0; j < max_j; j++) {
        short sample_value;
        if (tmpstore[j] > 32767) {
            sample_value = 32767;
        } else if (tmpstore[j] < -32767) {
            sample_value = -32768;
        } else {
            sample_value = (short)tmpstore[j];
        }

        data[block_counter] = (unsigned char)(sample_value & 0xFF);
        data[block_counter + 1] = (unsigned char)((sample_value >> 8) & 0xFF);

        left_of_block -= 2;
        block_counter += 2;

        if (left_of_block < 1) {
            crossed_limit = 1;
            write_audio_buffer();
            left_of_block = BUFFER_SIZE;
            block_counter = 0;
        }
    }

    /* Fill rest with silence */
    while (left_of_block > 0) {
        data[block_counter] = 0;
        data[block_counter + 1] = 0;
        left_of_block -= 2;
        block_counter += 2;
    }

    write_audio_buffer();
    beat = 0;

    /* Clear delay buffer */
    memset(delay_data, 0, MAX_DELAY * sizeof(short));

    return crossed_limit;
}

/* Calculate update interval */
unsigned long get_update_interval(int patt_index)
{
    return (unsigned long)(1000 * 240 / BPM);
}

unsigned long GetUpdateIntervall(int PattI)  {
   return (get_update_interval(PattI));
}


/* Main entry point */
int MAIN(int argc, char *argv[])
{
    /* Initialize ALSA */
    if (open_alsa_device() < 0) {
        fprintf(stderr, "Failed to initialize ALSA\n");
        return 1;
    }

    /* Initialize delay effect */
    memset(delay_data, 0, MAX_DELAY * sizeof(short));
    delay_pointer = 0;
    delay.Delay16thBeats = 230;
    delay.leftc = 0.5;
    delay.rightc = 0.5;
    delay.pointer = &delay_data[MAX_DELAY / 2];
    delay.On = 0; /* Disabled by default */

    /* Initialize patterns and drums */
    init_pattern();

    /* Parse drums configuration */
    parse_drums("DRUMS");

    /* Main playback loop would go here */
    /* This is just a skeleton - you'd need to add:
     * - Pattern loading
     * - Main timing loop
     * - User interface
     * - Pattern sequencing
     */

    /* Example: Play pattern 0 for 4 measures */
    for (int measure = 0; measure < 4; measure++) {
        play_pattern(0, measure);
    }

    /* Stop and cleanup */
    stop_playing();
    close_alsa_device();

    /* Free allocated memory */
    for (int i = 0; i < MAX_DRUMS; i++) {
        if (Drum[i].sample) {
            free(Drum[i].sample);
        }
        if (Drum[i].Name) {
            free(Drum[i].Name);
        }
        if (Drum[i].Filename) {
            free(Drum[i].Filename);
        }
    }

    return 0;
}