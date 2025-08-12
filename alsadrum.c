// Add these includes at the top of your file
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include "drum.h"
#include <string.h>
#include <assert.h>
#include <alsa/asoundlib.h>

#include "gdrum.h"
#include "drum.h"

int verbose = 1; // Global variable for verbosity

/* Configuration */
#define MAX_VOICE       32
#define BUFFER_SIZE     65536 * 2 
#define MAX_DELAY       200000
// Maximum sample length for loading
#define SLEN            265535
//#define MAX_DRUMS       64
//#define MAX_PATTERNS    32

// Add these global variables for thread control
static pthread_t audio_thread;
static pthread_mutex_t audio_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t audio_cond = PTHREAD_COND_INITIALIZER;
static volatile bool audio_thread_running = false;
static volatile bool audio_thread_should_stop = false;
static volatile bool pattern_playing = false;

#ifndef DRUMS_ROOT_DIR
#define DRUMS_ROOT_DIR  "."
#endif


// globals
static volatile bool quitting = false;

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
static unsigned int num_channels = 2;
static snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
int write_audio_buffer(int len);
extern int beat;

/* Audio buffer management */
static int left_of_block = BUFFER_SIZE;
static int block_counter = 0;
static unsigned char data[BUFFER_SIZE];
static long tmpstorel[SLEN];
static long tmpstorer[SLEN];

/* Delay effect */
static short delay_data[MAX_DELAY];
static int delay_pointer = 0;

/* Voice structure */
typedef struct {
    short *pointer;
    int count;
    int length;
    int on;
    int pan; // Panning for the voice
} Voice_t;

static Voice_t voices[MAX_VOICE];
static void play_pattern(int patt, int measure);

/* Global pattern and drum data */
tDelay delay;
tDrum         Drum[MAX_DRUMS];
tPattern      Pattern [MAX_PATTERNS];

/* Voice structure */
typedef struct {
    int pattern;
    int measure;
    bool loop;  // Whether to loop the pattern
} PatternRequest;


void reset_voices() {
    for (int i = 0; i < MAX_VOICE; i++) {
        voices[i].on = 0;
        voices[i].pointer = NULL;
        voices[i].count = -1;
        voices[i].length = 0;
    }
}
static PatternRequest current_request;
static volatile bool new_request = false;
extern int BPM;
extern int M;

// Audio thread function
static void* audio_thread_func(void* arg) {
    PatternRequest local_request;
    printf("Audio thread started\n");
    
    while (!audio_thread_should_stop || quitting) {
        // Wait for a pattern play request
        printf("Audio thread waiting for request...\n");
        pthread_mutex_lock(&audio_mutex);
        while (!new_request && !audio_thread_should_stop) {
            printf("cond_wait...\n");
            pthread_cond_wait(&audio_cond, &audio_mutex);
        }
        
        printf("Audio thread woke up\n");
        if (audio_thread_should_stop) {
            pthread_mutex_unlock(&audio_mutex);
            break;
        }
        printf("Audio thread received request to play pattern %d, measure %d, loop: %d\n",
               current_request.pattern, current_request.measure, current_request.loop);
        
        // Copy the request
        local_request = current_request;
        new_request = false;
        pattern_playing = true;
        pthread_mutex_unlock(&audio_mutex);
        
        // Play the pattern
        do {
            // Do two for test
            M=0;
            local_request.measure = 0;
            printf("Playing pattern %d, measure %d, loop: %d\n",
                   local_request.pattern, local_request.measure, local_request.loop);
            play_pattern(local_request.pattern, local_request.measure);
            play_pattern(local_request.pattern, local_request.measure);
            
            // Check if we should stop
            printf("Checking if we should continue playing...\n");
            pthread_mutex_lock(&audio_mutex);
            //bool should_continue = local_request.loop && pattern_playing && !audio_thread_should_stop;
            bool should_continue = false; // For testing, always stop after one play
            pthread_mutex_unlock(&audio_mutex);
            if (!should_continue) {
                printf("Stopping pattern playback\n");
            }
            
            if (!should_continue) break;
            
        } while (true);
        
        // Pattern finished
        pthread_mutex_lock(&audio_mutex);
        pattern_playing = false;
        pthread_mutex_unlock(&audio_mutex);
    }
    
    return NULL;
}

// Initialize the audio thread
int init_audio_thread(void) {
    if (audio_thread_running) {
        return 0; // Already running
    }
    reset_voices();
    audio_thread_should_stop = false;
    
    if (pthread_create(&audio_thread, NULL, audio_thread_func, NULL) != 0) {
        fprintf(stderr, "Failed to create audio thread\n");
        return -1;
    }
    
    audio_thread_running = true;
    return 0;
}

void starttimer(void) {


}
// Shutdown the audio thread
void shutdown_audio_thread(void) {
    if (!audio_thread_running) {
        return;
    }
    printf("shutdown_audio_thread...\n");
    // Print audio_mutex state for debugging
    printf("audio_mutex state: %d\n", pthread_mutex_trylock(&audio_mutex));

    pthread_mutex_lock(&audio_mutex);
    audio_thread_should_stop = true;
    pattern_playing = false;
    pthread_cond_signal(&audio_cond);
    pthread_mutex_unlock(&audio_mutex);
    
    pthread_join(audio_thread, NULL);
    audio_thread_running = false;
}

// Thread-safe version of PlayPattern
void PlayPatternThreaded(int pattern, int measure, bool loop) {
    //printf("Requesting to play pattern %d, measure %d, loop: %d\n", pattern, measure, loop);
    pthread_mutex_lock(&audio_mutex);
    //printf("Lock acquired, setting request...\n");
    current_request.pattern = pattern;
    current_request.measure = measure;
    current_request.loop = loop;
    new_request = true;
    pthread_cond_signal(&audio_cond);
    //printf("Signal sent, unlocking mutex...\n");
    pthread_mutex_unlock(&audio_mutex);
}

// Stop the currently playing pattern
void StopPattern(void) {
    quitting = true;
    pthread_mutex_lock(&audio_mutex);
    pattern_playing = false;
    pthread_mutex_unlock(&audio_mutex);
    
    // Also call the existing stop function to finish audio
    stop_playing();
}

// Check if a pattern is currently playing
bool IsPatternPlaying(void) {
    bool playing;
    pthread_mutex_lock(&audio_mutex);
    playing = pattern_playing;
    pthread_mutex_unlock(&audio_mutex);
    return playing;
}

// Get current beat position (thread-safe)
int GetCurrentBeat(void) {
    int current_beat;
    pthread_mutex_lock(&audio_mutex);
    // OLAS TODO!! current_beat = beat;
    pthread_mutex_unlock(&audio_mutex);
    return current_beat;
}



int StopPlaying(void) {
   quitting = true;
   return stop_playing();
}
void PlayPattern(int Patt, int Measure) {
   exit(0); // This function is not used in the new implementation
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
    // 44100 samples per second * 60 seconds per minute / BPM is 
    // 1 beat = 44100 / 4 = 11025 samples
    double tpb = (double)(1500 / (double)BPM); // Ticks per Beat
    double tum = (double)measure * (24000 / (double)BPM); // Ticks per Measure
    sample_len = 44100 *num_channels  *60 / BPM;

    for (int i = beat; i < 16; i++) {
        /* Trigger drums for this beat */
        for (int j = 0; j < MAX_DRUMS; j++) {
            if (Pattern[patt].DrumPattern[j].Drum != NULL) {
                if (Pattern[patt].DrumPattern[j].beat[i] > 0) {
                    voice_on(Pattern[patt].DrumPattern[j].Drum->SampleNum);
                    printf("Playing drum %d in pattern %d, beat %d\n", 
                           Pattern[patt].DrumPattern[j].Drum->SampleNum, patt, i);
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
        //M++;
        patt = next_patt;
    }

    remember_patt = patt;
}


/* Stop playing and finish remaining audio */
int stop_playing(void)
{
    int max_j = 0;
    int crossed_limit = 0;

    memset(tmpstorel, 0, (BUFFER_SIZE + 1) * sizeof(long));
    memset(tmpstorer, 0, (BUFFER_SIZE + 1) * sizeof(long));

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
                    int leftc=*voices[voi].pointer;
                    int rightc=*voices[voi].pointer;
                    tmpstorel[j] += (long)((float)leftc * (float)(voices[voi].pan / 127.0));
                    tmpstorer[j] += (long)((float)rightc * (float)(1.0 - voices[voi].pan / 127.0));
                    voices[voi].pointer++;
                    voices[voi].count++;
                }
            }
        }
    }

    /* Convert and output remaining samples */
    for (int j = 0; j < max_j; j++) {
        short sample_value;
        if (tmpstorel[j] > 32767) {
            sample_value = 32767;
        } else if (tmpstorel[j] < -32767) {
            sample_value = -32768;
        } else {
            sample_value = (short)tmpstorel [j];
        }


        data[block_counter] = (unsigned char)(sample_value & 0xFF);
        data[block_counter + 1] = (unsigned char)((sample_value >> 8) & 0xFF);

        left_of_block -= 2;
        block_counter += 2;

        if (left_of_block < 1) {
            crossed_limit = 1;
            write_audio_buffer(block_counter);
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

    write_audio_buffer(block_counter);
    beat = 0;

    /* Clear delay buffer */
    memset(delay_data, 0, MAX_DELAY * sizeof(short));

    return crossed_limit;
}


unsigned long GetUpdateIntervall(int PattI)  {
  return (unsigned long)(1000 * 240 / BPM);
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
    err = snd_pcm_hw_params_set_channels(pcm_handle, hw_params, num_channels);
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

   #if 0 
    // Set buffer size (in frames)
    snd_pcm_uframes_t buffer_frames = BUFFER_SIZE * 4 / (num_channels * 2);
    err = snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hw_params, &buffer_frames);
    if (err < 0) {
        fprintf(stderr, "Cannot set buffer size: %s\n", snd_strerror(err));
        return -1;
    }

    /* Set period size */
    frames = BUFFER_SIZE / (num_channels * 2); /* 2 bytes per sample for S16 */
    err = snd_pcm_hw_params_set_period_size_near(pcm_handle, hw_params, &frames, 0);
    if (err < 0) {
        fprintf(stderr, "Cannot set period size: %s\n", snd_strerror(err));
        return -1;
    }
#endif
    snd_pcm_uframes_t period_frames = 1024;
    snd_pcm_hw_params_set_period_size_near(pcm_handle, hw_params, &period_frames, 0);

    snd_pcm_uframes_t buffer_frames = period_frames * 4; // 4 periods
    snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hw_params, &buffer_frames);

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
void close_alsa_device(void)
{
    quitting = true;
    shutdown_audio_thread();
    // Wait for audio thread to finish
    if (audio_thread_running) {
        pthread_join(audio_thread, NULL);
    }
    if (pcm_handle) {
        snd_pcm_drain(pcm_handle);
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
    }
}
static int xrun_recovery(snd_pcm_t *handle, int err)
{
    if (verbose)
        printf("stream recovery\n");
    if (err == -EPIPE) {    /* under-run */
        err = snd_pcm_prepare(handle);
        if (err < 0)
            printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
        return 0;
    } else if (err == -ESTRPIPE) {
        while ((err = snd_pcm_resume(handle)) == -EAGAIN)
            sleep(1);   /* wait until the suspend flag is released */
        if (err < 0) {
            err = snd_pcm_prepare(handle);
            if (err < 0)
                printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
        }
        return 0;
    }
    return err;
}

/* Write audio buffer to ALSA */
int write_audio_buffer(int length_bytes)
{
    if (!pcm_handle) return 0;
    if (audio_thread_should_stop) {
        // avoid fighting with drain/close; pretend success
        return 0;
    }
    int frames_to_write = length_bytes / (num_channels * 2); // 2 bytes/sample
    int frame_size_bytes = num_channels * 2;
    unsigned char *ptr = data;
    while (frames_to_write > 0) {
        snd_pcm_sframes_t n = snd_pcm_writei(pcm_handle, ptr, frames_to_write);
        if (n == -EPIPE) {
            fprintf(stderr, "ALSA underrun\n");
            xrun_recovery(pcm_handle, n);
            continue;
        } else if (n == -EAGAIN) {
            continue;
        } else if (n < 0) {
            fprintf(stderr, "ALSA write error: %s\n", snd_strerror(n));
            xrun_recovery(pcm_handle, n);
            return -1;
        }
        ptr += n * frame_size_bytes;
        frames_to_write -= n;
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
    printf("Parsing drums file: %s\n", filepath);

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
int voice_on(int drum_index)
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
    voices[j].pan = Drum[drum_index].pan;

    return j;
}

/* Add one beat worth of audio data */
int add_one_beat(int frames)
{
    int crossed_limit = 0;

    // tmpstore holds mono mix in 32-bit to avoid overflow, length = frames
    memset(tmpstorel, 0, frames * sizeof(long));
    memset(tmpstorer, 0, frames * sizeof(long));


    // Mix voices (length and count are in samples)
    for (int voi = 0; voi < MAX_VOICE; ++voi) {
        if (voices[voi].count >= 0) {
            int sound_len = voices[voi].length - voices[voi].count;
            if (sound_len <= 0) { voices[voi].count = -1; continue; }
            int n = (sound_len < frames) ? sound_len : frames;

            short *p = voices[voi].pointer;
            for (int j = 0; j < n; ++j) {
                tmpstorel[j] += (long)p[j]* voices[voi].pan / 127.0; // Apply panning
                tmpstorer[j] += (long)p[j] * (1.0 - voices[voi].pan / 127.0); // Right channel
            }

            voices[voi].pointer += n;
            voices[voi].count   += n;
            if (n < frames) {
                // sample ended this beat
                voices[voi].count = -1;
            }
        }
    }

    // Delay tap (optional; keep simple and safe)
    if (delay.On == 1) {
        printf("Adding delay effect\n");
        int delay_offset = (delay.Delay16thBeats * frames) / 16;
        int dp = delay_pointer - delay_offset;
        if (dp < 0) dp += MAX_DELAY;
        short *dptr = &delay_data[dp]; // local pointer for this beat

        for (int j = 0; j < frames; ++j) {
            tmpstorel[j] += (long)((float)dptr[0] * delay.leftc);
            tmpstorer[j] += (long)((float)dptr[0] * delay.rightc);
            dptr++;
            if (dptr >= &delay_data[MAX_DELAY]) dptr = &delay_data[0];
        }
    }

    // Convert to 16-bit, write L+R, update delay ring
    for (int j = 0; j < frames; ++j) {
        long v = tmpstorel[j];
        if (v >  32767) v =  32767;
        if (v < -32768) v = -32768;
        short l = (short)v;
        v = tmpstorer[j];
        if (v >  32767) v =  32767;
        if (v < -32768) v = -32768;
        short r = (short)v;

        // write stereo: L then R (duplicate mono)

        data[block_counter + 0] = (unsigned char)(l & 0xFF);
        data[block_counter + 1] = (unsigned char)((l >> 8) & 0xFF);
        data[block_counter + 2] = (unsigned char)(r & 0xFF);
        data[block_counter + 3] = (unsigned char)((r >> 8) & 0xFF);

        // update delay ring with the (mono) sample
        delay_data[delay_pointer] = l;
        delay_pointer = (delay_pointer + 1) % MAX_DELAY;

        block_counter += 4;    // 4 bytes per frame (stereo 16-bit)
        left_of_block -= 4;

        if (left_of_block < 4) {
            crossed_limit = 1;
            write_audio_buffer(block_counter);  // bytes
            block_counter = 0;
            left_of_block = BUFFER_SIZE;
        }
    }

    return crossed_limit;
}

// Implementation:
int init_audio_system(void) 
{
    if (pcm_handle != NULL) {
        return 0; // Already initialized
    }
    return open_alsa_device();
}


// Modified MAIN function with thread initialization
int MAIN(int argc, char *argv[])
{
    // Initialize ALSA
    if (init_audio_system() < 0) {
        fprintf(stderr, "Warning: Failed to initialize audio system\n");
        return -1;
    }

    // Initialize delay effect
    #if 1
    memset(delay_data, 0, MAX_DELAY * sizeof(short));
    delay_pointer = 0;
    delay.Delay16thBeats = 230;
    delay.leftc = 0.5;
    delay.rightc = 0.5;
    delay.pointer = &delay_data[MAX_DELAY / 2];
    #endif
    delay.On = 0;

    // Initialize patterns and drums
    init_pattern();

    // Parse drums configuration
    parse_drums("DRUMS");

    // Initialize audio thread
    if (init_audio_thread() < 0) {
        fprintf(stderr, "Failed to initialize audio thread\n");
        close_alsa_device();
        return -1;
    }

    reset_voices();

    // Your ImGui main loop would go here
    // Example usage:
    // PlayPatternThreaded(0, 0, true);  // Play pattern 0, looping
    // ...
    // StopPattern();  // Stop playing

    // Cleanup
    //shutdown_audio_thread();
    // stop_playing();
    // close_alsa_device();

    /* Free allocated memory */
    #if 0
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
    #endif

    return 0;
}
