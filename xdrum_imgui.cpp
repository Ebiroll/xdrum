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
int beat;

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
    window = glfwCreateWindow(1200, 700, "XDrum/V1.5 - ImGui Port", NULL, NULL);
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
        
        // Render the drum context menu if needed
        RenderDrumContextMenu();
        
        if (show_file_dialog)
            RenderFileDialog();
        
        if (show_pattern_dialog)
            RenderPatternDialog();
            
        if (show_delay_dialog)
            RenderDelayDialog();
            
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
            char pattern_char = 'a' + i;  // Always use lowercase for patterns
            
            if (io.KeyShift) {
                // Shift+letter = copy current pattern to that letter, then switch to it
                CopyBeat(PatternIndex, pattern_char);
                printf("Copied pattern %c to pattern %c\n", (char)PatternIndex, pattern_char);
            }
            
            // Switch to the pattern (always lowercase)
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

        // Push a unique ID for this drum row to prevent ID conflicts
        ImGui::PushID(drum_idx);
        
        // Drum name button with right-click context menu
        bool is_selected = (drum_idx == DrumIndex);
        if (is_selected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        }
        char button_label[64];
        snprintf(button_label, sizeof(button_label), "%s##drum%d", drum_name, drum_idx);
        
        if (ImGui::Button(button_label, ImVec2(80, 0))) {
            // Check if Shift is held for context menu
            if (ImGui::GetIO().KeyShift) {
                context_menu_drum_idx = drum_idx;
                show_drum_context_menu = true;
                printf("Opening context menu for drum %d\n", drum_idx);
            } else {
                ChangeDrum(nullptr, (XtPointer)(intptr_t)drum_idx, nullptr);
            }
        }
        
        // Right-click context menu (backup method)
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            context_menu_drum_idx = drum_idx;
            show_drum_context_menu = true;
            printf("Right-click context menu for drum %d\n", drum_idx);
        }
        
        // Tooltip to show Shift+click hint
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Click to select, Shift+Click for options");
        }
        
        if (is_selected) {
            ImGui::PopStyleColor();
        }
        
        ImGui::SameLine();
        
        // Beat toggles for this drum - ALL ON ONE LINE
        for (int i = 0; i < 16; i++) {
            bool beat_on = Pattern[PatternIndex].DrumPattern[drum_idx].beat[i] > 0;
            char beat_id[32];
            snprintf(beat_id, sizeof(beat_id), "##beat%d", i);
            
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
            
            // Keep everything on the same line
            if (i < 15) ImGui::SameLine();
        }
        
        // Beat numbers guide (only show for selected drum)
        if (drum_idx == DrumIndex) {
            // Indent to align with the checkboxes
            ImGui::Dummy(ImVec2(88.0f, 0.0f));
            ImGui::SameLine();
            
            for (int i = 0; i < 16; i++) {
                // Center the number under the checkbox
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 7);  // Offset to center number
                ImGui::Text("%d", i % 4);
                if (i < 15) {
                    ImGui::SameLine();
                }
            }
        }
        
        // Pop the ID for this drum row
        ImGui::PopID();
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

        // Push ID here as well, since buttons may have the same name in different patterns
        ImGui::PushID(i);
        if (ImGui::Button(drum_name, ImVec2(100, 0))) {
            // Check if Shift is held for context menu
            if (ImGui::GetIO().KeyShift) {
                context_menu_drum_idx = i;
                show_drum_context_menu = true;
                printf("Opening context menu for drum %d\n", i);
            } else {
                ChangeDrum(nullptr, (XtPointer)(intptr_t)i, nullptr);
            }
        }
        
        // Right-click context menu (backup method)
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            context_menu_drum_idx = i;
            show_drum_context_menu = true;
            printf("Right-click context menu for drum %d\n", i);
        }
        ImGui::PopID();
        
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
        play_pattern_ui(nullptr, nullptr, nullptr);
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
    if (show_drum_context_menu) {
        ImGui::OpenPopup("DrumContextMenu");
        show_drum_context_menu = false;  // Reset flag after opening
    }
    
    if (ImGui::BeginPopupModal("DrumContextMenu", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
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
                if (ImGui::Button("Show Waveform")) {
                    waveform_drum_idx = context_menu_drum_idx;
                    show_waveform_window = true;
                    ImGui::CloseCurrentPopup();
                }
                
                // Load new sample
                if (ImGui::Button("Load Sample...")) {
                    // Could trigger a file dialog for loading new samples
                    printf("Load sample for drum %d\n", context_menu_drum_idx);
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::Separator();
                
                // Volume control (if available)
                ImGui::Text("Volume:");
                static float volume = 1.0f;
                ImGui::SliderFloat("##volume", &volume, 0.0f, 2.0f, "%.2f");
                
                ImGui::Separator();
                
                if (ImGui::Button("Close")) {
                    ImGui::CloseCurrentPopup();
                }
            }
        }
        
        ImGui::EndPopup();
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
            
            // Get the actual sample data if loaded
            if (drum->Loaded && drum->sample != NULL && drum->length > 0) {
                ImGui::Text("Sample Length: %d samples", drum->length);
                ImGui::Text("Pan: %d", drum->pan);
                ImGui::Text("Volume: %d", drum->vol);
                
                // Find the actual content bounds (skip silence at the end)
                int effective_end = drum->length;
                int effective_start = 0;
                float threshold = 0.001f; // Silence threshold
                
                // Find where the sample effectively ends (skip trailing silence)
                for (int i = drum->length - 1; i >= drum->length * 0.1f; i--) {
                    float val = fabs((float)drum->sample[i] / 32768.0f);
                    if (val > threshold) {
                        effective_end = i + (drum->length / 100); // Add small padding
                        if (effective_end > drum->length) effective_end = drum->length;
                        break;
                    }
                }
                
                // Find where the sample effectively starts (skip leading silence)
                for (int i = 0; i < drum->length * 0.1f; i++) {
                    float val = fabs((float)drum->sample[i] / 32768.0f);
                    if (val > threshold) {
                        effective_start = i - 100; // Small padding before
                        if (effective_start < 0) effective_start = 0;
                        break;
                    }
                }
                
                // Zoom controls
                static float zoom_start = 0.0f;
                static float zoom_end = 1.0f;
                static bool auto_fit = true;
                
                ImGui::Checkbox("Auto-fit waveform", &auto_fit);
                
                if (auto_fit) {
                    zoom_start = (float)effective_start / drum->length;
                    zoom_end = (float)effective_end / drum->length;
                    if (zoom_end - zoom_start < 0.05f) { // Minimum zoom range
                        zoom_end = zoom_start + 0.05f;
                        if (zoom_end > 1.0f) {
                            zoom_end = 1.0f;
                            zoom_start = 0.95f;
                        }
                    }
                } else {
                    ImGui::Text("Manual Zoom:");
                    if (ImGui::SliderFloat("Start", &zoom_start, 0.0f, zoom_end - 0.01f, "%.3f")) {
                        if (zoom_start >= zoom_end) zoom_start = zoom_end - 0.01f;
                    }
                    if (ImGui::SliderFloat("End", &zoom_end, zoom_start + 0.01f, 1.0f, "%.3f")) {
                        if (zoom_end <= zoom_start) zoom_end = zoom_start + 0.01f;
                    }
                    if (ImGui::Button("Reset Zoom")) {
                        zoom_start = 0.0f;
                        zoom_end = 1.0f;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Fit to Content")) {
                        auto_fit = true;
                    }
                }
                
                // Calculate the displayed range
                int display_start = (int)(zoom_start * drum->length);
                int display_end = (int)(zoom_end * drum->length);
                int display_length = display_end - display_start;
                
                // Waveform visualization with actual data
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
                ImVec2 canvas_size = ImVec2(ImGui::GetContentRegionAvail().x - 10, 200); // Use available width
                
                // Background
                draw_list->AddRectFilled(canvas_pos, 
                                       ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                                       IM_COL32(30, 30, 30, 255));
                
                // Border
                draw_list->AddRect(canvas_pos, 
                                 ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                                 IM_COL32(255, 255, 255, 255));
                
                // Center line
                float center_y = canvas_pos.y + canvas_size.y / 2;
                draw_list->AddLine(ImVec2(canvas_pos.x, center_y), 
                                  ImVec2(canvas_pos.x + canvas_size.x, center_y), 
                                  IM_COL32(128, 128, 128, 128));
                
                // Grid lines for better visualization
                for (int i = 1; i < 4; i++) {
                    float y = canvas_pos.y + (canvas_size.y * i) / 4;
                    draw_list->AddLine(ImVec2(canvas_pos.x, y), 
                                      ImVec2(canvas_pos.x + canvas_size.x, y), 
                                      IM_COL32(64, 64, 64, 64));
                }
                
                // Vertical grid lines (time markers)
                int num_time_lines = 8;
                for (int i = 1; i < num_time_lines; i++) {
                    float x = canvas_pos.x + (canvas_size.x * i) / num_time_lines;
                    draw_list->AddLine(ImVec2(x, canvas_pos.y), 
                                      ImVec2(x, canvas_pos.y + canvas_size.y), 
                                      IM_COL32(64, 64, 64, 64));
                }
                
                // Calculate downsampling ratio for the zoomed range
                int samples_per_pixel = display_length / (int)canvas_size.x;
                if (samples_per_pixel < 1) samples_per_pixel = 1;
                
                // Window size for sliding average (adjust for smoothness)
                int window_size = samples_per_pixel;
                if (window_size < 2) window_size = 2;
                if (window_size > 100) window_size = 100;
                
                // Track min/max for better visualization
                float global_max = 0.0f;
                for (int i = display_start; i < display_end && i < drum->length; i++) {
                    float val = fabs((float)drum->sample[i] / 32768.0f);
                    if (val > global_max) global_max = val;
                }
                if (global_max < 0.1f) global_max = 0.1f; // Prevent division by zero
                
                // Draw waveform
                float prev_x = canvas_pos.x;
                float prev_y = center_y;
                
                for (int x = 0; x < canvas_size.x; x++) {
                    int sample_idx = display_start + (x * display_length) / canvas_size.x;
                    int sample_end_idx = sample_idx + window_size;
                    if (sample_end_idx > display_end) sample_end_idx = display_end;
                    if (sample_end_idx > drum->length) sample_end_idx = drum->length;
                    
                    // Calculate min/max for this window (for better peak visibility)
                    float min_val = 0.0f;
                    float max_val = 0.0f;
                    float avg_val = 0.0f;
                    int count = 0;
                    
                    for (int i = sample_idx; i < sample_end_idx && i < drum->length; i++) {
                        // Convert 16-bit signed sample to float (-1.0 to 1.0)
                        float val = (float)drum->sample[i] / 32768.0f;
                        
                        avg_val += val;
                        if (val < min_val) min_val = val;
                        if (val > max_val) max_val = val;
                        count++;
                    }
                    
                    if (count > 0) {
                        avg_val /= count;
                    }
                    
                    // Use appropriate value based on zoom level
                    float display_val;
                    if (samples_per_pixel > 50) {
                        // Very zoomed out - show envelope
                        display_val = (fabs(max_val) > fabs(min_val)) ? max_val : min_val;
                    } else if (samples_per_pixel > 10) {
                        // Medium zoom - blend peak and average
                        float peak = (fabs(max_val) > fabs(min_val)) ? max_val : min_val;
                        display_val = peak * 0.7f + avg_val * 0.3f;
                    } else {
                        // Zoomed in - show actual samples
                        display_val = avg_val;
                    }
                    
                    // Normalize and scale
                    display_val = display_val / global_max;
                    
                    // Scale and position
                    float curr_x = canvas_pos.x + x;
                    float curr_y = center_y - (display_val * canvas_size.y * 0.45f); // Scale to 90% of height
                    
                    // Draw line from previous point
                    if (x > 0) {
                        draw_list->AddLine(ImVec2(prev_x, prev_y), 
                                         ImVec2(curr_x, curr_y), 
                                         IM_COL32(0, 255, 0, 255), 1.5f);
                    }
                    
                    // Draw min/max envelope for dense samples
                    if (samples_per_pixel > 5 && min_val != max_val) {
                        float min_y = center_y - (min_val / global_max * canvas_size.y * 0.45f);
                        float max_y = center_y - (max_val / global_max * canvas_size.y * 0.45f);
                        draw_list->AddLine(ImVec2(curr_x, min_y), 
                                         ImVec2(curr_x, max_y), 
                                         IM_COL32(0, 128, 0, 128), 0.5f);
                    }
                    
                    prev_x = curr_x;
                    prev_y = curr_y;
                }
                
                ImGui::Dummy(canvas_size);
                
                // Display info about the visible range
                float visible_start_time = (float)display_start / 44100.0f;
                float visible_end_time = (float)display_end / 44100.0f;
                ImGui::Text("Visible range: %.3f - %.3f seconds (%.1f%% of sample)", 
                           visible_start_time, visible_end_time, (zoom_end - zoom_start) * 100.0f);
                ImGui::Text("Total length: %.3f seconds", (float)drum->length / 44100.0f);
                
                // Simple amplitude analysis for the visible range
                float rms = 0.0f;
                int analysis_count = 0;
                for (int i = display_start; i < display_end && i < drum->length; i++) {
                    float val = (float)drum->sample[i] / 32768.0f;
                    rms += val * val;
                    analysis_count++;
                }
                if (analysis_count > 0) {
                    rms = sqrt(rms / analysis_count);
                    ImGui::Text("Visible RMS Level: %.3f (%.1f dB)", rms, 20.0f * log10(rms + 0.00001f));
                }
                
            } else {
                ImGui::Text("Sample data not available");
                
                // Fallback visualization
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
                ImVec2 canvas_size = ImVec2(400, 100);
                
                draw_list->AddRectFilled(canvas_pos, 
                                       ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                                       IM_COL32(50, 50, 50, 255));
                
                draw_list->AddRect(canvas_pos, 
                                 ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), 
                                 IM_COL32(255, 255, 255, 255));
                
                // Center line
                float center_y = canvas_pos.y + canvas_size.y / 2;
                draw_list->AddLine(ImVec2(canvas_pos.x, center_y), 
                                  ImVec2(canvas_pos.x + canvas_size.x, center_y), 
                                  IM_COL32(128, 128, 128, 255));
                
                ImGui::Dummy(canvas_size);
                ImGui::Text("No waveform data to display");
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
            
            /* Show drums and beats for this pattern - EDITABLE, ALL ON ONE LINE */
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
                
                // Editable beat toggles - ALL ON ONE LINE
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
                    
                    // Keep all beats on same line
                    if (beat < 15) {
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

           // printf("Elapsed time since last update: %ld ms , interval %ld ms\n", elapsed, UpdateIntervall);
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
    close_alsa_device();
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
    PlayPatternThreaded(PatternIndex, M, false);
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
    
    PlayPatternThreaded(SongPatternIndex, SongM,false);
    M++;
    need_pattern_update = true;
}

void play_pattern_ui(Widget w, XtPointer client_data, XtPointer call_data)
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
    PlayPatternThreaded(PatternIndex, 0, true);

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
    PlayPatternThreaded(SongPatternIndex, 0, false);

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