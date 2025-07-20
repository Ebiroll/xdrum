/*
 *  xdrum_imgui.cpp
 *   
 * ImGui port of the original Athena widget-based drum machine
 * 
 * This maintains the same functionality and variable names as the original
 * but uses Dear ImGui for the interface instead of X11/Athena widgets
 *
 */

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <ctype.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <chrono>
#include <thread>

#include "xdrum.h"
#include "drum.h"
#include "files.h"

#include <assert.h>

extern tDelay delay;

/* Global variables - same as original */
int DrumIndex = 0;
int PatternIndex = 0;
int SongPatternIndex = 0;
int M = 0;
int SongM = 0;
int buffIndex = 0;
int SongPlay = 0;
int BPM = 120;
unsigned long UpdateIntervall;
unsigned long RealUpdateIntervall;

struct timeval StartTime;
struct timezone DummyTz;
extern tPattern Pattern[];

pDrumPattern PattP;

/* ImGui specific globals */
GLFWwindow* window;
bool show_file_dialog = false;
bool show_pattern_dialog = false;
bool show_delay_dialog = false;
bool save_mode = false;
bool is_playing = false;
bool is_song_playing = false;

char speed_buf[7] = "120";
char limit_buf[LIMIT_LEN];
char PatternBuf[3] = "a";
char SongBuf[SONG_BUF_LENGTH] = "aaaa";
char song_name_buf[NAME_LENGTH] = "test.sng";

char *edit_buf;
int x1_old = 0, x2_old = 0;
header_data options;
int visible_sample;
DWORD start_visible;
int pid = -1;

/* Timer management for ImGui */
std::chrono::steady_clock::time_point last_update_time;
std::chrono::steady_clock::time_point song_update_time;
bool need_pattern_update = false;
bool need_song_update = false;

/* Function prototypes */
void RenderMainWindow();
void RenderFileDialog();
void RenderPatternDialog();
void RenderDelayDialog();
void UpdateTimers();

int main(int argc, char **argv)
{
    /* Initialize GLFW */
    if (!glfwInit())
        return -1;

    /* GL 3.0 + GLSL 130 */
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    /* Create window */
    window = glfwCreateWindow(800, 600, "XDrum/V1.5 - ImGui Port", NULL, NULL);
    if (window == NULL)
        return -1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    /* Setup ImGui */
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    /* Setup Platform/Renderer backends */
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    /* Initialize patterns and drums */
    PatternIndex = 'a'; // ASCII value for 'a'
    
    /* Initialize the sound card */
    MAIN(argc, argv);
    PattP = &Pattern[0].DrumPattern[0];

    /* Main loop */
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        /* Start ImGui frame */
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        /* Update timers for pattern/song playback */
        UpdateTimers();

        /* Render UI */
        RenderMainWindow();
        
        if (show_file_dialog)
            RenderFileDialog();
        
        if (show_pattern_dialog)
            RenderPatternDialog();
            
        if (show_delay_dialog)
            RenderDelayDialog();

        /* Rendering */
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    /* Cleanup */
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    close_alsa_device();
    return 0;
}

void RenderMainWindow()
{
    ImGui::Begin("XDrum Control Panel", nullptr, ImGuiWindowFlags_NoCollapse);

    /* Speed/BPM control */
    ImGui::Text("Tempo (BPM):");
    ImGui::SameLine();
    ImGui::InputText("##speed", speed_buf, sizeof(speed_buf));

    /* Pattern name display */
    ImGui::Text("Pattern:");
    ImGui::SameLine();
    if (ImGui::Button(PatternBuf)) {
        show_pattern_dialog = true;
    }

    /* File operations */
    if (ImGui::Button("Load...")) {
        show_file_dialog = true;
        save_mode = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Save...")) {
        show_file_dialog = true;
        save_mode = true;
    }

    /* Playback controls */
    if (ImGui::Button("Play Pattern") && !is_playing && !is_song_playing) {
        play_pattern(nullptr, nullptr, nullptr);
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Pattern") && is_playing) {
        stop_playing_pattern(nullptr, nullptr, nullptr);
    }

    if (ImGui::Button("Play Song") && !is_playing && !is_song_playing) {
        play_song(nullptr, nullptr, nullptr);
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Song") && is_song_playing) {
        stop_playing_song(nullptr, nullptr, nullptr);
    }

    /* Delay control */
    if (ImGui::Button("Delay...")) {
        show_delay_dialog = true;
    }

    /* Drum selection buttons */
    ImGui::Separator();
    ImGui::Text("Select Drum:");
    for (int i = 0; i < MAX_DRUMS; i++) {
        if (i > 0 && i % 4 != 0) ImGui::SameLine();
        
        char drum_label[32];
        if (Pattern[PatternIndex].DrumPattern[i].Drum && 
            Pattern[PatternIndex].DrumPattern[i].Drum->Name) {
            snprintf(drum_label, sizeof(drum_label), "%s", 
                    Pattern[PatternIndex].DrumPattern[i].Drum->Name);
        } else {
            snprintf(drum_label, sizeof(drum_label), "Drum %d", i);
        }
        
        if (ImGui::Button(drum_label, ImVec2(100, 0))) {
            ChangeDrum(nullptr, (XtPointer)i, nullptr);
        }
    }

    /* Beat pattern toggles */
    ImGui::Separator();
    ImGui::Text("Beat Pattern:");
    
    for (int i = 0; i < 16; i++) {
        if (i > 0) ImGui::SameLine();
        
        bool beat_on = Pattern[PatternIndex].DrumPattern[DrumIndex].beat[i] > 0;
        char beat_label[8];
        snprintf(beat_label, sizeof(beat_label), "%d", i % 4);
        
        if (ImGui::Checkbox(beat_label, &beat_on)) {
            Pattern[PatternIndex].DrumPattern[DrumIndex].beat[i] = beat_on ? 1 : 0;
        }
        
        if (i == 7) {
            ImGui::Text(""); // New line after first 8 beats
        }
    }

    /* Song editor */
    ImGui::Separator();
    ImGui::Text("Song:");
    ImGui::InputTextMultiline("##song", SongBuf, sizeof(SongBuf), 
                              ImVec2(-1, 100), 
                              is_song_playing ? ImGuiInputTextFlags_ReadOnly : 0);

    /* Keyboard input handling for pattern switching */
    if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive()) {
        ImGuiIO& io = ImGui::GetIO();
        for (int key = ImGuiKey_A; key <= ImGuiKey_Z; key++) {
            if (ImGui::IsKeyPressed(key)) {
                int keycode = key - ImGuiKey_A + 'a';
                
                if (io.KeyShift) {
                    /* Copy current pattern to new pattern */
                    CopyBeat(PatternIndex, keycode);
                }
                
                PatternIndex = keycode;
                PattP = &Pattern[PatternIndex].DrumPattern[DrumIndex];
                SetBeats(DrumIndex);
                PatternBuf[0] = (char)keycode;
                PatternBuf[1] = '\0';
            }
        }
    }

    ImGui::End();
}

void RenderFileDialog()
{
    ImGui::OpenPopup("File Dialog");
    
    bool open = true;
    if (ImGui::BeginPopupModal("File Dialog", &open)) {
        ImGui::Text(save_mode ? "Save as:" : "Load file:");
        
        ImGui::InputText("##filename", song_name_buf, sizeof(song_name_buf));
        
        if (ImGui::Button("OK")) {
            if (save_mode) {
                save_file();
            } else {
                load_file();
            }
            show_file_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            show_file_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    if (!open) {
        show_file_dialog = false;
    }
}

void RenderPatternDialog()
{
    ImGui::OpenPopup("Pattern Overview");
    
    bool open = true;
    if (ImGui::BeginPopupModal("Pattern Overview", &open)) {
        /* Display all patterns with drums and beats */
        for (int patt = 0; patt < MAX_PATTERNS; patt++) {
            char patt_char = (char)patt;
            if (patt_char < 'a' || patt_char > 'z') continue;
            
            bool has_content = false;
            for (int drum = 0; drum < MAX_DRUMS; drum++) {
                unsigned char bytes[2];
                if (FindBytes(&bytes[0], &bytes[1], patt, drum) > 0) {
                    has_content = true;
                    break;
                }
            }
            
            if (!has_content) continue;
            
            ImGui::Separator();
            char patt_label[16];
            snprintf(patt_label, sizeof(patt_label), "Pattern %c", patt_char);
            if (ImGui::Button(patt_label)) {
                set_active_pattern(nullptr, (XtPointer)patt, nullptr);
            }
            
            /* Show drums and beats for this pattern */
            for (int drum = 0; drum < MAX_DRUMS; drum++) {
                unsigned char bytes[2];
                if (FindBytes(&bytes[0], &bytes[1], patt, drum) > 0) {
                    ImGui::Text("  %s:", Pattern[patt].DrumPattern[drum].Drum->Name);
                    ImGui::SameLine();
                    
                    for (int beat = 0; beat < 16; beat++) {
                        bool beat_on = Pattern[patt].DrumPattern[drum].beat[beat] > 0;
                        char beat_id[32];
                        snprintf(beat_id, sizeof(beat_id), "##p%dd%db%d", patt, drum, beat);
                        
                        if (ImGui::Checkbox(beat_id, &beat_on)) {
                            Pattern[patt].DrumPattern[drum].beat[beat] = beat_on ? 1 : 0;
                        }
                        if (beat < 15) ImGui::SameLine();
                    }
                }
            }
        }
        
        if (ImGui::Button("OK")) {
            show_pattern_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    if (!open) {
        show_pattern_dialog = false;
    }
}

void RenderDelayDialog()
{
    ImGui::OpenPopup("Delay Settings");
    
    bool open = true;
    if (ImGui::BeginPopupModal("Delay Settings", &open)) {
        bool delay_enabled = delay.On == 1;
        if (ImGui::Checkbox("Delay ON", &delay_enabled)) {
            delay.On = delay_enabled ? 1 : 0;
        }
        
        ImGui::Text("Feedback:");
        ImGui::SliderFloat("##feedback", &delay.leftc, 0.0f, 1.0f);
        
        ImGui::Text("Time:");
        float delay_time = (float)delay.Delay16thBeats / (16.0f * 16.0f);
        if (ImGui::SliderFloat("##time", &delay_time, 0.0f, 1.0f)) {
            delay.Delay16thBeats = (unsigned int)(delay_time * 16 * 16);
        }
        
        if (ImGui::Button("OK")) {
            show_delay_dialog = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    if (!open) {
        show_delay_dialog = false;
    }
}

void UpdateTimers()
{
    if (!is_playing && !is_song_playing) return;
    
    auto now = std::chrono::steady_clock::now();
    
    /* Pattern playback timer */
    if (is_playing && need_pattern_update) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_update_time).count();
        
        if (elapsed >= UpdateIntervall) {
            PlayMeasure(nullptr, nullptr);
            last_update_time = now;
        }
    }
    
    /* Song playback timer */
    if (is_song_playing && need_song_update) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - song_update_time).count();
        
        if (elapsed >= RealUpdateIntervall) {
            UpdateSong(nullptr, nullptr);
            song_update_time = now;
        }
    }
}

/* Callbacks implementation - maintaining original function signatures */

void SetBeat(Widget w, XtPointer client_data, XtPointer call_data)
{
    int BeatNr = (int)client_data;

    if (Pattern[PatternIndex].DrumPattern[DrumIndex].beat[BeatNr] > 0) {
        Pattern[PatternIndex].DrumPattern[DrumIndex].beat[BeatNr] = 0;
    } else {
        Pattern[PatternIndex].DrumPattern[DrumIndex].beat[BeatNr] = 1;
    }
}

void SetBeats(int DrumNr)
{
    /* In ImGui version, the UI automatically updates from the data */
}

void ChangeDrum(Widget w, XtPointer client_data, XtPointer call_data)
{
    printf("Change drum to %d\n", (int)client_data);
    int DrumNr = (int)client_data;
    
    /* Bounds checking */
    if (DrumNr < 0 || DrumNr >= MAX_DRUMS) {
        fprintf(stderr, "Error: DrumNr %d out of bounds\n", DrumNr);
        return;
    }
    
    if (PatternIndex < 0 || PatternIndex >= MAX_PATTERNS) {
        fprintf(stderr, "Error: PatternIndex %d out of bounds\n", PatternIndex);
        return;
    }
    
    /* Check if drum pointer is valid */
    if (Pattern[PatternIndex].DrumPattern[DrumNr].Drum == NULL) {
        fprintf(stderr, "Error: Drum %d not initialized in pattern %d\n", DrumNr, PatternIndex);
        return;
    }
    
    /* Now safe to access the drum */
    tDrum *drum = Pattern[PatternIndex].DrumPattern[DrumNr].Drum;
    
    if (drum->Loaded != 1) {
        /* Check if filename exists */
        if (drum->Filename == NULL) {
            fprintf(stderr, "Error: No filename for drum %d\n", DrumNr);
            return;
        }
        
        if (load_sample(drum->SampleNum, drum->Filename, drum->pan, drum->Filetype) < 0) {
            fprintf(stderr, "Error: Failed to load sample for drum %d\n", DrumNr);
            return;
        }
        
        drum->Loaded = 1;
    }
    
    DrumIndex = DrumNr;
    PattP = &Pattern[PatternIndex].DrumPattern[DrumNr];
    assert(PattP != NULL);
    SetBeats(DrumNr);
}

void quit_application()
{
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void stop_playing_pattern(Widget w, XtPointer client_data, XtPointer call_data)
{
    is_playing = false;
    need_pattern_update = false;
    StopPlaying();
}

void stop_playing_song(Widget w, XtPointer client_data, XtPointer call_data)
{
    is_playing = false;
    is_song_playing = false;
    need_pattern_update = false;
    need_song_update = false;
    StopPlaying();
}

void UpdateSong(XtPointer client_data, XtIntervalId *Dummy)
{
    /* Update song buffer position indicator */
    if (SongBuf[buffIndex] == '\0') {
        stop_playing_song(nullptr, nullptr, nullptr);
    } else {
        need_song_update = true;
    }
}

void PlayMeasure(XtPointer client_data, XtIntervalId *Dummy)
{
    PlayPattern(PatternIndex, M);
    M++;
    need_pattern_update = true;
}

void PlaySongMeasure(XtPointer client_data, XtIntervalId *Dummy)
{
    char Test[2];
    
    if (SongM != M) {
        buffIndex++;
        if (SongBuf[buffIndex] == '\n') {
            buffIndex++;
        }
        Test[0] = SongBuf[buffIndex];
        Test[1] = '\0';
        
        /* Convert character to pattern index */
        SongPatternIndex = Test[0];
        SongM = M;
    }
    
    PlayPattern(SongPatternIndex, SongM);
    M++;
    need_pattern_update = true;
}

void play_pattern(Widget w, XtPointer client_data, XtPointer call_data)
{
    SongPlay = 0;

    sscanf(speed_buf, "%d", &BPM);
    if (BPM > 640) {
        BPM = 640;
    }

    if (BPM < 1) {
        BPM = 1;
    }
    
    starttimer();
    M = 0;
    PlayPattern(PatternIndex, 0);

    if (gettimeofday(&StartTime, &DummyTz) < 0) {
        fprintf(stderr, "Something went wrong with the time call");
    }

    UpdateIntervall = GetUpdateIntervall(PatternIndex);

    last_update_time = std::chrono::steady_clock::now();
    is_playing = true;
    need_pattern_update = true;
}

void play_song(Widget w, XtPointer client_data, XtPointer call_data)
{
    char Test[2];
    SongPlay = 1;

    SongM = 0;
    buffIndex = 0;
    Test[0] = SongBuf[buffIndex];
    Test[1] = '\0';
    SongPatternIndex = Test[0];
    
    sscanf(speed_buf, "%d", &BPM);
    if (BPM > 640) {
        BPM = 640;
    }

    if (BPM < 1) {
        BPM = 1;
    }
    
    starttimer();
    M = 0;
    PlayPattern(SongPatternIndex, 0);

    if (gettimeofday(&StartTime, &DummyTz) < 0) {
        fprintf(stderr, "Something went wrong with the time call..\n");
    }
  
    UpdateIntervall = GetUpdateIntervall(PatternIndex);
    RealUpdateIntervall = (unsigned long)(1000 * 240 / BPM);

    last_update_time = std::chrono::steady_clock::now();
    song_update_time = std::chrono::steady_clock::now();
    
    is_playing = true;
    is_song_playing = true;
    need_pattern_update = true;
    need_song_update = true;
}

void CopyBeat(int FromPattern, int ToPattern)
{
    int k, j;
    
    for (j = 0; j < MAX_DRUMS; j++) {
        for (k = 0; k < 16; k++) {
            Pattern[ToPattern].DrumPattern[j].beat[k] = 
                Pattern[FromPattern].DrumPattern[j].beat[k];
        }
    }
}

void load_file()
{
    fprintf(stderr, "Loading %s\n", song_name_buf);
    LoadFile(song_name_buf);
    SetBeats(DrumIndex);
}

void save_file()
{
    fprintf(stderr, "Saving %s\n", song_name_buf);
    SaveFile(song_name_buf);
}

void set_active_pattern(Widget w, XtPointer client_data, XtPointer call_data)
{
    PatternIndex = (int)client_data;
    PattP = &Pattern[PatternIndex].DrumPattern[DrumIndex];
    SetBeats(DrumIndex);
    PatternBuf[0] = (char)PatternIndex;
    PatternBuf[1] = '\0';
}

void set_feedback(Widget w, XtPointer client_data, XtPointer percent_ptr)
{
    delay.leftc = *(float *)percent_ptr;
}

void set_time(Widget w, XtPointer client_data, XtPointer percent_ptr)
{
    delay.Delay16thBeats = 16 * 16 * *(float *)percent_ptr;
}

void delay_on(Widget w, XtPointer client_data, XtPointer percent_ptr)
{
    if (delay.On == 1) {
        delay.On = 0;
    } else {
        delay.On = 1;
    }
}

/* Helper functions for string/pattern conversion */
char *PatternIndexToString(int PI)
{
    static char str[2];
    str[0] = (char)PI;
    str[1] = '\0';
    return str;
}

int StringToPatternIndex(char *string)
{
    return string[0];
}
