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

// Add these global variables for thread control
static pthread_t audio_thread;
static pthread_mutex_t audio_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile bool audio_thread_running = false;
static volatile bool audio_thread_should_stop = false;

// Pattern control - simplified
static volatile int active_patt = 0;  // 0 means no pattern playing (silence)
static volatile int active_measure = 0;
static volatile bool pattern_loop = false;

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

extern int BPM;
extern int M;

void reset_voices() {
    for (int i = 0; i < MAX_VOICE; i++) {
        voices[i].on = 0;
        voices[i].pointer = NULL;
        voices[i].count = -1;
        voices[i].length = 0;
    }
}

// Continuously running audio thread
static void* audio_thread_func(void* arg) {
    int local_beat = 0;
    int local_measure = 0;
    int sample_len;
    int samples_until_next_beat = 0;
    
    printf("Audio thread started - continuous mode\n");
    
    while (!audio_thread_should_stop) {
        // Calculate samples per beat
        sample_len = sample_rate * 60 / BPM / 4;  // 1/16th note
        
        // Check if we need to trigger new drums
        if (samples_until_next_beat <= 0) {
            pthread_mutex_lock(&audio_mutex);
            int current_patt = active_patt;
            bool loop = pattern_loop;
            pthread_mutex_unlock(&audio_mutex);
            
            if (current_patt > 0) {  // Pattern is playing
                // Trigger drums for this beat
                for (int j = 0; j < MAX_DRUMS; j++) {
                    if (Pattern[current_patt - 1].DrumPattern[j].Drum != NULL) {
                        if (Pattern[current_patt - 1].DrumPattern[j].beat[local_beat] > 0) {
                            voice_on(Pattern[current_patt - 1].DrumPattern[j].Drum->SampleNum);
                            printf("Playing drum %d in pattern %d, beat %d\n", 
                                   Pattern[current_patt - 1].DrumPattern[j].Drum->SampleNum, 
                                   current_patt - 1, local_beat);
                        }
                    }
                }
                
                // Advance beat
                local_beat++;
                if (local_beat >= 16) {
                    local_beat = 0;
                    local_measure++;
                    
                    // Check if we should loop or stop
                    if (!loop) {
                        pthread_mutex_lock(&audio_mutex);
                        active_patt = 0;  // Stop playing
                        pthread_mutex_unlock(&audio_mutex);
                        printf("Pattern finished, stopping\n");
                    }
                }
            } else {
                // Reset beat counter when not playing
                local_beat = 0;
                local_measure = 0;
            }
            
            samples_until_next_beat = sample_len;
        }
        
        // Generate audio for a small chunk
        int chunk_size = 512;  // Process in small chunks
        if (chunk_size > samples_until_next_beat && samples_until_next_beat > 0) {
            chunk_size = samples_until_next_beat;
        }
        
        // Clear temporary buffers
        memset(tmpstorel, 0, chunk_size * sizeof(long));
        memset(tmpstorer, 0, chunk_size * sizeof(long));
        
        // Mix active voices
        for (int voi = 0; voi < MAX_VOICE; voi++) {
            if (voices[voi].count > -1) {
                int samples_remaining = (voices[voi].length / 2) - voices[voi].count;
                if (samples_remaining <= 0) {
                    voices[voi].count = -1;
                    voices[voi].on = 0;
                    continue;
                }
                
                int samples_to_process = (samples_remaining < chunk_size) ? samples_remaining : chunk_size;
                
                short *p = voices[voi].pointer;
                for (int j = 0; j < samples_to_process; j++) {
                    tmpstorel[j] += (long)(p[j] * voices[voi].pan / 127.0);
                    tmpstorer[j] += (long)(p[j] * (1.0 - voices[voi].pan / 127.0));
                }
                
                voices[voi].pointer += samples_to_process;
                voices[voi].count += samples_to_process;
                
                if (voices[voi].count >= voices[voi].length / 2) {
                    voices[voi].count = -1;
                    voices[voi].on = 0;
                }
            }
        }
        
        // Apply delay effect if enabled
        if (delay.On == 1) {
            int samples_per_16th = (sample_rate * 60) / (BPM * 4);
            int delay_samples = delay.Delay16thBeats * samples_per_16th;
            
            if (delay_samples >= MAX_DELAY) {
                delay_samples = MAX_DELAY - 1;
            }
            
            for (int j = 0; j < chunk_size; j++) {
                int read_pos = (delay_pointer + j - delay_samples) % MAX_DELAY;
                if (read_pos < 0) {
                    read_pos += MAX_DELAY;
                }
                
                tmpstorel[j] += (long)(delay_data[read_pos] * delay.leftc);
                tmpstorer[j] += (long)(delay_data[read_pos] * delay.rightc);
            }
        }
        
        // Convert to 16-bit stereo and output
        for (int j = 0; j < chunk_size; j++) {
            // Clip left channel
            long v = tmpstorel[j];
            if (v > 32767) v = 32767;
            if (v < -32768) v = -32768;
            short left_sample = (short)v;
            
            // Clip right channel
            v = tmpstorer[j];
            if (v > 32767) v = 32767;
            if (v < -32768) v = -32768;
            short right_sample = (short)v;
            
            // Write STEREO output
            data[block_counter] = (unsigned char)(left_sample & 0xFF);
            data[block_counter + 1] = (unsigned char)((left_sample >> 8) & 0xFF);
            data[block_counter + 2] = (unsigned char)(right_sample & 0xFF);
            data[block_counter + 3] = (unsigned char)((right_sample >> 8) & 0xFF);
            
            // Update delay buffer
            if (delay.On == 1) {
                delay_data[delay_pointer] = (short)((left_sample + right_sample) / 2);
                delay_pointer = (delay_pointer + 1) % MAX_DELAY;
            }
            
            block_counter += 4;
            left_of_block -= 4;
            
            if (left_of_block < 4) {
                write_audio_buffer(block_counter);
                left_of_block = BUFFER_SIZE;
                block_counter = 0;
            }
        }
        
        samples_until_next_beat -= chunk_size;
        
        // Small sleep to prevent CPU spinning
        usleep(100);  // 0.1ms sleep
    }
    
    printf("Audio thread stopped\n");
    return NULL;
}

// Initialize the audio thread
int init_audio_thread(void) {
    if (audio_thread_running) {
        return 0; // Already running
    }
    
    reset_voices();
    audio_thread_should_stop = false;
    active_patt = 0;  // Start with no pattern playing
    
    if (pthread_create(&audio_thread, NULL, audio_thread_func, NULL) != 0) {
        fprintf(stderr, "Failed to create audio thread\n");
        return -1;
    }
    
    audio_thread_running = true;
    return 0;
}

void starttimer(void) {
    // Not needed with continuous thread
}

// Shutdown the audio thread
void shutdown_audio_thread(void) {
    if (!audio_thread_running) {
        return;
    }
    
    printf("shutdown_audio_thread...\n");
    audio_thread_should_stop = true;
    
    pthread_join(audio_thread, NULL);
    audio_thread_running = false;
}

// Start playing a pattern
void PlayPatternThreaded(int pattern, int measure, bool loop) {
    printf("Starting pattern %d, measure %d, loop: %d\n", pattern, measure, loop);
    
    pthread_mutex_lock(&audio_mutex);
    active_patt = pattern + 1;  // Use 1-based indexing (0 means stopped)
    active_measure = measure;
    pattern_loop = loop;
    pthread_mutex_unlock(&audio_mutex);
    
    // Clear delay buffer when starting new pattern
    memset(delay_data, 0, MAX_DELAY * sizeof(short));
}

// Stop the currently playing pattern
void StopPattern(void) {
    pthread_mutex_lock(&audio_mutex);
    active_patt = 0;  // This will cause silence to be written
    pthread_mutex_unlock(&audio_mutex);
    
    printf("Pattern stopped\n");
}

// Check if a pattern is currently playing
bool IsPatternPlaying(void) {
    bool playing;
    pthread_mutex_lock(&audio_mutex);
    playing = (active_patt > 0);
    pthread_mutex_unlock(&audio_mutex);
    return playing;
}

// Get current beat position (thread-safe)
int GetCurrentBeat(void) {
    // This would need to be tracked in the audio thread
    // For now, return 0
    return 0;
}

int StopPlaying(void) {
    StopPattern();
    return 0;
}

void PlayPattern(int Patt, int Measure) {
    PlayPatternThreaded(Patt, Measure, false);
}

int EmptyCard(void) {
    int i;
    
    for(i = 0; i < MAX_DRUMS; i++) {
        Drum[i].Loaded = 0;
    }
    /* TODO fix the memory leakage */
    return 0;
}

// This function is no longer used - pattern playing is handled in the thread
static void play_pattern(int patt, int measure) {
    // Not used anymore
}

// This is now simplified - just stops the pattern
int stop_playing(void) {
    StopPattern();
    return 0;
}

unsigned long GetUpdateIntervall(int PattI) {
    return (unsigned long)(800 * 60 / BPM);
}

/* Open ALSA device for playback */
static int open_alsa_device(void) {
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

    snd_pcm_uframes_t period_frames = 1024 * 4;
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
void close_alsa_device(void) {
    shutdown_audio_thread();
    
    if (pcm_handle) {
        snd_pcm_drain(pcm_handle);
        snd_pcm_close(pcm_handle);
        pcm_handle = NULL;
    }
}

static int xrun_recovery(snd_pcm_t *handle, int err) {
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
int write_audio_buffer(int length_bytes) {
    if (!pcm_handle) return 0;
    if (audio_thread_should_stop) {
        return 0;
    }
    
    // Check if we should write silence
    pthread_mutex_lock(&audio_mutex);
    int current_patt = active_patt;
    pthread_mutex_unlock(&audio_mutex);
    
    // If no pattern is playing and no voices are active, write silence
    if (current_patt == 0) {
        int active_voices = 0;
        for (int i = 0; i < MAX_VOICE; i++) {
            if (voices[i].count > -1) {
                active_voices++;
                break;
            }
        }
        
        if (active_voices == 0) {
            // Write silence
            memset(data, 0, length_bytes);
        }
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
int load_sample(int samplenum, const char *name, int pan, int intel_format) {
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
static void init_pattern(void) {
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
char *add_root(char *name) {
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
static void parse_drums(const char *filename) {
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

void validate_delay_params(void) {
    // Ensure delay time is reasonable
    int max_delay_16ths = (MAX_DELAY * BPM * 4) / (sample_rate * 60);
    if (delay.Delay16thBeats > max_delay_16ths) {
        delay.Delay16thBeats = max_delay_16ths;
        printf("Warning: Delay time clamped to %d 16th beats\n", max_delay_16ths);
    }
    
    // Ensure feedback is in valid range (0.0 to 0.95 to avoid runaway feedback)
    if (delay.leftc < 0.0) delay.leftc = 0.0;
    if (delay.leftc > 0.95) delay.leftc = 0.95;
    if (delay.rightc < 0.0) delay.rightc = 0.0;
    if (delay.rightc > 0.95) delay.rightc = 0.95;
}

/* Start playing a voice */
int voice_on(int drum_index) {
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

/* This function is no longer used - audio generation is in the thread */
int add_one_beat(int frames) {
    // Not used anymore
    return 0;
}

// Implementation:
int init_audio_system(void) {
    if (pcm_handle != NULL) {
        return 0; // Already initialized
    }
    return open_alsa_device();
}

// Modified MAIN function with thread initialization
int MAIN(int argc, char *argv[]) {
    // Initialize ALSA
    if (init_audio_system() < 0) {
        fprintf(stderr, "Warning: Failed to initialize audio system\n");
        return -1;
    }

    // Initialize delay effect
    memset(delay_data, 0, MAX_DELAY * sizeof(short));
    delay_pointer = 0;
    delay.Delay16thBeats = 4;
    delay.leftc = 0.3;
    delay.rightc = 0.3;
    delay.pointer = &delay_data[MAX_DELAY / 2];
    delay.On = 0;

    // Initialize patterns and drums
    init_pattern();

    // Parse drums configuration
    parse_drums("DRUMS");

    // Initialize audio thread - it will run continuously
    if (init_audio_thread() < 0) {
        fprintf(stderr, "Failed to initialize audio thread\n");
        close_alsa_device();
        return -1;
    }

    reset_voices();

    // The audio thread is now running continuously
    // You can control it with:
    // PlayPatternThreaded(0, 0, true);  // Start playing pattern 0, looping
    // StopPattern();  // Stop playing (will write silence)
    // IsPatternPlaying();  // Check if playing

    // Your ImGui main loop would go here
    // The thread keeps running, writing silence when active_patt == 0

    return 0;
}