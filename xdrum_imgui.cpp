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
#include <cmath>

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <chrono>
#include <thread>

extern "C" {
#include "xdrum.h"
#include "drum.h"
#include "files.h"
}

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
char song_name_buf[NAME_LEN] = "test.sng";

char *edit_buf;
int x1_old = 0, x2_old = 0;
header_data options;
int visible_sample;
DWORD start_visible;
int pid = -1;

/* Context menu state */
bool show_drum_context_menu = false;
int context_menu_drum_idx = -1;
bool show_waveform_window = false;
int waveform_drum_idx = -1;

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
void RenderDrumContextMenu();
void RenderWaveformWindow();
void UpdateTimers();
void HandleKeyboardInput();
void set_active_pattern(Widget w, XtPointer client_data, XtPointer call_data);
void PlayMeasure(XtPointer client_data, XtIntervalId *Dummy);
void UpdateSong(XtPointer client_data, XtIntervalId *Dummy);

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
    window = glfwCreateWindow(900, 700, "XDrum/V1.5 - ImGui Port", NULL, NULL);
    if (window == NULL)
        return -1;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    /* Setup ImGui */
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard navigation

    ImGui::StyleColorsDark();

    /* Setup Platform/Renderer backends */
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    /* Initialize patterns and drums */
    PatternIndex = StringToPatternIndex("a");
    
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

        /* Handle keyboard input */
        HandleKeyboardInput();

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
            
        if (show_drum_context_menu)
            RenderDrumContextMenu();
            
        if (show_waveform_window)
            RenderWaveformWindow();

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

void HandleKeyboardInput()
{
    ImGuiIO& io = ImGui::GetIO();
    
    // Only process keys when no text input is active and main window is focused
    if (!io.WantCaptureKeyboard || ImGui::IsAnyItemActive()) {
        return;
    }
    
    // Check for pattern switching keys (A-Z)
    for (int i = 0; i < 26; i++) {
        ImGuiKey key = (ImGuiKey)(ImGuiKey_A + i);
        if (ImGui::IsKeyPressed(key)) {
            char pattern_char;
            
            if (io.KeyShift) {
                // Shift+letter = uppercase pattern
                pattern_char = 'A' + i;
                // Copy current pattern to new pattern
                CopyBeat(PatternIndex, pattern_char);
            } else {
                // letter = lowercase pattern
                pattern_char = 'a' + i;
            }
            
            // Switch to the pattern
            PatternIndex = pattern_char;
            PattP = &Pattern[PatternIndex].DrumPattern[DrumIndex];
            SetBeats(DrumIndex);
            sprintf(PatternBuf, "%c", pattern_char);
            
            printf("Switched to pattern %c\n", pattern_char);
            break;
        }
    }
}

void RenderMainWindow()
{
    ImGui::Begin("XDrum Control Panel", nullptr, ImGuiWindowFlags_NoCollapse);

    /* Pattern name display at top */
    ImGui::Text("Current Pattern:");
    ImGui::SameLine();
    if (ImGui::Button(PatternBuf, ImVec2(40, 0))) {
        show_pattern_dialog = true;
    }
    ImGui::SameLine();
    ImGui::Text("(Press A-Z to switch patterns, Shift+A-Z to copy)");

    ImGui::Separator();

    /* Beat patterns for all loaded drums in current pattern */
    ImGui::Text("Beat Patterns:");
    
    bool any_drum_shown = false;
    for (int drum_idx = 0; drum_idx < MAX_DRUMS; drum_idx++) {
        // Skip uninitialized drums
        if (Pattern[PatternIndex].DrumPattern[drum_idx].Drum == NULL) {
            continue;
        }
        
        // Skip drums with generic names (unloaded)
        char* drum_name = Pattern[PatternIndex].DrumPattern[drum_idx].Drum->Name;
        if (drum_name == NULL || strncmp(drum_name, "Drum ", 5) == 0) {
            continue;
        }
        
        // Check if this drum has any beats
        bool has_beats = false;
        for (int b = 0; b < 16; b++) {
            if (Pattern[PatternIndex].DrumPattern[drum_idx].beat[b] > 0) {
                has_beats = true;
                break;
            }
        }
        
        // Always show the currently selected drum
        if (drum_idx != DrumIndex && !has_beats) {
            continue;
        }
        
        any_drum_shown = true;
        
        // Drum name button with right-click context menu
        bool is_selected = (drum_idx == DrumIndex);
        if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        }
        
        if (ImGui::Button(drum_name, ImVec2(80, 0))) {
            ChangeDrum(nullptr, (XtPointer)(intptr_t)drum_idx, nullptr);
        }
        
        // Right-click context menu
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            context_menu_drum_idx = drum_idx;
            show_drum_context_menu = true;
            ImGui::OpenPopup("DrumContextMenu");
        }
        
        if (is_selected) {
            ImGui::PopStyleColor();
        }
        
        ImGui::SameLine();
        
        // Beat toggles for this drum
        for (int i = 0; i < 16; i++) {
            if (i == 8) {
                // New line for beats 8-15
                ImGui::Text("        "); // Indent
                ImGui::SameLine();
            }
            
            bool beat_on = Pattern[PatternIndex].DrumPattern[drum_idx].beat[i] > 0;
            char beat_id[32];
            snprintf(beat_id, sizeof(beat_id), "##d%db%d", drum_idx, i);
            
            // Color coding for quarter notes
            if (i % 4 == 0) {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.2f, 0.2f, 1.0f));
            }
            
            if (ImGui::Checkbox(beat_id, &beat_on)) {
                Pattern[PatternIndex].DrumPattern[drum_idx].beat[i] = beat_on ? 1 : 0;
            }
            
            if (i % 4 == 0) {
                ImGui::PopStyleColor();
            }
            
            if (i < 15 && i != 7) ImGui::SameLine();
        }
        
        // Beat numbers guide (only show for selected drum)
        if (drum_idx == DrumIndex) {
            ImGui::Text("        "); // Indent
            ImGui::SameLine();
            for (int i = 0; i < 16; i++) {
                if (i == 8) {
                    ImGui::Text("        ");
                    ImGui::SameLine();
                }
                ImGui::Text("%d", i % 4);
                if (i < 15 && i != 7) {
                    ImGui::SameLine();
                    ImGui::Dummy(ImVec2(14, 0)); // Spacing to align with checkboxes
                    ImGui::SameLine();
                }
            }
        }
    }
    
    if (!any_drum_shown) {
        ImGui::Text("No drums loaded in this pattern");
    }

    ImGui::Separator();

    /* Drum selection - only show loaded drums */
    ImGui::Text("Load/Select Drum:");
    int drums_per_row = 0;
    for (int i = 0; i < MAX_DRUMS; i++) {
        if (Pattern[PatternIndex].DrumPattern[i].Drum == NULL) {
            continue;
        }
        
        char* drum_name = Pattern[PatternIndex].DrumPattern[i].Drum->Name;
        if (drum_name == NULL) {
            continue;
        }
        
        // Show all drums here (even with generic names) for loading
        if (drums_per_row > 0 && drums_per_row % 4 == 0) {
            drums_per_row = 0; // New row
        }
        
        if (drums_per_row > 0) ImGui::SameLine();
        
        if (ImGui::Button(drum_name, ImVec2(100, 0))) {
            ChangeDrum(nullptr, (XtPointer)(intptr_t)i, nullptr);
        }
        
        // Right-click context menu
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            context_menu_drum_idx = i;
            show_drum_context_menu = true;
            ImGui::OpenPopup("DrumContextMenu");
        }
        
        drums_per_row++;
    }

    ImGui::Separator();

    /* Speed/BPM control */
    ImGui::Text("Tempo (BPM):");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(60);
    ImGui::InputText("##speed", speed_buf, sizeof(speed_buf));

    /* Playback controls */
    ImGui::Separator();
    if (ImGui::Button("Play Pattern") && !is_playing && !is_song_playing) {
        play_pattern(nullptr, nullptr, nullptr);
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Pattern") && is_playing) {
        stop_playing_pattern(nullptr, nullptr, nullptr);
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Play Song") && !is_playing && !is_song_playing) {
        play_song(nullptr, nullptr, nullptr);
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop Song") && is_song_playing) {
        stop_playing_song(nullptr, nullptr, nullptr);
    }

    /* File operations */
    ImGui::Separator();
    if (ImGui::Button("Load...")) {
        show_file_dialog = true;
        save_mode = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Save...")) {
        show_file_dialog = true;
        save_mode = true;
    }
    ImGui::SameLine();
    
    /* Delay control */
    if (ImGui::Button("Delay...")) {
        show_delay_dialog = true;
    }

    /* Song editor */
    ImGui::Separator();
    ImGui::Text("Song:");
    ImGui::InputTextMultiline("##song", SongBuf, sizeof(SongBuf), 
                              ImVec2(-1, 100), 
                              is_song_playing ? ImGuiInputTextFlags_ReadOnly : 0);

    ImGui::End();
}

void RenderDrumContextMenu()
{
    if (ImGui::BeginPopup("DrumContextMenu")) {
        if (context_menu_drum_idx >= 0 && context_menu_drum_idx < MAX_DRUMS) {
            tDrum* drum = Pattern[PatternIndex].DrumPattern[context_menu_drum_idx].Drum;
            
            if (drum != NULL) {
                ImGui::Text("Drum: %s", drum->Name ? drum->Name : "Unknown");
                ImGui::Separator();
                
                // Pan control
                ImGui::Text("Pan:");
                float pan = (float)drum->pan / 127.0f * 2.0f - 1.0f; // Convert 0-127 to -1.0 to 1.0
                if (ImGui::SliderFloat("##pan", &pan, -1.0f, 1.0f, "%.2f")) {
                    drum->pan = (int)((pan + 1.0f) / 2.0f * 127.0f); // Convert back to 0-127
                }
                
                ImGui::Separator();
                
                // Show waveform
                if (ImGui::MenuItem("Show Waveform")) {
                    waveform_drum_idx = context_menu_drum_idx;
                    show_waveform_window = true;
                }
                
                // Load new sample
                if (ImGui::MenuItem("Load Sample...")) {
                    // Could trigger a file dialog for loading new samples
                    printf("Load sample for drum %d\n", context_menu_drum_idx);
                }
                
                ImGui::Separator();
                
                // Volume control (if available)
                ImGui::Text("Volume:");
                static float volume = 1.0f;
                ImGui::SliderFloat("##volume", &volume, 0.0f, 2.0f, "%.2f");
            }
        }
        
        if (!ImGui::IsPopupOpen("DrumContextMenu")) {
            show_drum_context_menu = false;
        }
        
        ImGui::EndPopup();
    } else {
        show_drum_context_menu = false;
    }
}

void RenderWaveformWindow()
{
    if (!show_waveform_window) return;
    
    ImGui::Begin("Waveform Viewer", &show_waveform_window);
    
    if (waveform_drum_idx >= 0 && waveform_drum_idx < MAX_DRUMS) {
        tDrum* drum = Pattern[PatternIndex].DrumPattern[waveform_drum_idx].Drum;
        
        if (drum != NULL) {
            ImGui::Text("Waveform for: %s", drum->Name ? drum->Name : "Unknown");
            ImGui::Text("Sample Number: %d", drum->SampleNum);
            ImGui::Text("Filename: %s", drum->Filename ? drum->Filename : "None");
            ImGui::Text("Loaded: %s", drum->Loaded ? "Yes" : "No");
            
            // Simple waveform visualization placeholder
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = ImVec2(400, 100);
            
            draw_list->AddRectFilled(canvas_pos, 
                                   ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                                   IM_COL32(50, 50, 50, 255));
            
            draw_list->AddRect(canvas_pos, 
                             ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                             IM_COL32(255, 255, 255, 255));
            
            // Draw a simple sine wave as placeholder
            for (int i = 0; i < canvas_size.x - 1; i++) {
                float x1 = canvas_pos.x + i;
                float x2 = canvas_pos.x + i + 1;
                float y1 = canvas_pos.y + canvas_size.y / 2 + sin(i * 0.1f) * 30;
                float y2 = canvas_pos.y + canvas_size.y / 2 + sin((i + 1) * 0.1f) * 30;
                
                draw_list->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(0, 255, 0, 255));
            }
            
            ImGui::Dummy(canvas_size);
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
    if (ImGui::BeginPopupModal("Pattern Overview", &open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Pattern Overview - Click patterns to select, edit beats directly");
        ImGui::Separator();
        
        /* Display all patterns with drums and beats */
        for (int patt = 'a'; patt <= 'z'; patt++) {
            bool has_content = false;
            for (int drum = 0; drum < MAX_DRUMS; drum++) {
                if (Pattern[patt].DrumPattern[drum].Drum != NULL) {
                    for (int beat = 0; beat < 16; beat++) {
                        if (Pattern[patt].DrumPattern[drum].beat[beat] > 0) {
                            has_content = true;
                            break;
                        }
                    }
                    if (has_content) break;
                }
            }
            
            if (!has_content && patt != PatternIndex) continue;
            
            ImGui::Separator();
            
            // Pattern header with selection button
            char patt_label[32];
            snprintf(patt_label, sizeof(patt_label), "Pattern %c%s", 
                    (char)patt, (patt == PatternIndex) ? " (Current)" : "");
            
            bool is_current = (patt == PatternIndex);
            if (is_current) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
            }
            
            if (ImGui::Button(patt_label, ImVec2(150, 0))) {
                set_active_pattern(nullptr, (XtPointer)(intptr_t)patt, nullptr);
            }
            
            if (is_current) {
                ImGui::PopStyleColor();
            }
            
            /* Show drums and beats for this pattern - EDITABLE */
            for (int drum = 0; drum < MAX_DRUMS; drum++) {
                if (Pattern[patt].DrumPattern[drum].Drum == NULL) continue;
                
                char* drum_name = Pattern[patt].DrumPattern[drum].Drum->Name;
                if (drum_name == NULL || strncmp(drum_name, "Drum ", 5) == 0) continue;
                
                bool has_beats = false;
                for (int beat = 0; beat < 16; beat++) {
                    if (Pattern[patt].DrumPattern[drum].beat[beat] > 0) {
                        has_beats = true;
                        break;
                    }
                }
                
                if (!has_beats && patt != PatternIndex) continue;
                
                ImGui::Text("  %s:", drum_name);
                ImGui::SameLine();
                
                // Editable beat toggles
                for (int beat = 0; beat < 16; beat++) {
                    bool beat_on = Pattern[patt].DrumPattern[drum].beat[beat] > 0;
                    char beat_id[32];
                    snprintf(beat_id, sizeof(beat_id), "##p%dd%db%d", patt, drum, beat);
                    
                    // Color coding for quarter notes
                    if (beat % 4 == 0) {
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.4f, 0.2f, 0.2f, 1.0f));
                    }
                    
                    if (ImGui::Checkbox(beat_id, &beat_on)) {
                        Pattern[patt].DrumPattern[drum].beat[beat] = beat_on ? 1 : 0;
                    }
                    
                    if (beat % 4 == 0) {
                        ImGui::PopStyleColor();
                    }
                    
                    if (beat == 7) {
                        // Start new line for beats 8-15
                        ImGui::Text("        "); // Indent
                        ImGui::SameLine();
                    } else if (beat < 15) {
                        ImGui::SameLine();
                    }
                }
            }
        }
        
        ImGui::Separator();
        if (ImGui::Button("Close")) {
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
    int BeatNr = (int)(intptr_t)client_data;

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
    printf("Change drum to %d\n", (int)(intptr_t)client_data);
    int DrumNr = (int)(intptr_t)client_data;
    
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
    PatternIndex = (int)(intptr_t)client_data;
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
    if (PI >= 'a' && PI <= 'z') {
        str[0] = (char)PI;
    } else if (PI >= 'A' && PI <= 'Z') {
        str[0] = (char)PI;
    } else {
        // For backwards compatibility with X11 keycodes
        // Assuming lowercase letters were mapped to sequential keycodes
        if (PI >= 0 && PI < 26) {
            str[0] = 'a' + PI;
        } else if (PI >= 26 && PI < 52) {
            str[0] = 'A' + (PI - 26);
        } else {
            str[0] = '?'; // Invalid pattern
        }
    }
    str[1] = '\0';
    return str;
}

int StringToPatternIndex(char *string)
{
    if (string && string[0]) {
        return (int)string[0];
    }
    return 'a'; // Default to pattern 'a'
}
