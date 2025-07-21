cmake_minimum_required(VERSION 3.14)
project(xdrum C CXX)

# ——— Options to mimic your Makefile targets ———
option(USE_GUS   "GUS version (default)"           OFF)
option(OTHER     "Generic /dev/dsp version"         OFF)
option(STEREO    "Stereo /dev/dsp version"          OFF)
option(USE_ALSA  "ALSA + ImGui modern version"      ON)

# Only one of the first three makes sense when not using ALSA:
if(NOT USE_ALSA)
  math(EXPR MODE_COUNT "${USE_GUS} + ${OTHER} + ${STEREO}")
  if(MODE_COUNT GREATER 1)
    message(FATAL_ERROR "When USE_ALSA=OFF, only one of USE_GUS, OTHER, STEREO may be ON.")
  elseif(MODE_COUNT EQUAL 0)
    message(FATAL_ERROR "When USE_ALSA=OFF, you must enable exactly one of USE_GUS, OTHER, STEREO.")
  endif()
endif()

# ——— Fetch ImGui only if ALSA/modern mode is on ———
if(USE_ALSA)
  include(FetchContent)
  FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.1               # ← use the v1.92.1 release
  )
  FetchContent_MakeAvailable(imgui)
endif()

# ——— Find system libs ———
find_package(X11    REQUIRED)
include_directories(${X11_INCLUDE_DIR})

if(USE_ALSA)
  find_package(OpenGL REQUIRED)
  find_package(glfw3   3.3 REQUIRED)
  find_library(ASOUND_LIB asound REQUIRED)

  set(COMMON_SRCS
  files.c
  )

else()
set(COMMON_SRCS
  files.c
  widget.c
)

endif()

# ——— Common sources, flags & libs ———


add_compile_definitions(
  DRUMS_ROOT_DIR=\"${CMAKE_SOURCE_DIR}\"
  $<$<BOOL:${STEREO}>:STEREO>
)

set(COMMON_LIBS
  Xaw
  Xmu
  Xt
  ${X11_LIBRARIES}
)

# ——— Mode‐specific wiring ———
if(USE_ALSA)
  # modern ImGui + ALSA
  list(APPEND COMMON_SRCS
    xdrum_imgui.cpp
    alsadrum.c
  )
  add_compile_definitions(ALSADRUM)
  #target_compile_definitions(xdrum  ALSADRUM)
  list(APPEND COMMON_LIBS
    ${ASOUND_LIB}
    OpenGL::GL
    glfw
  )

  # build ImGui core + backends as static libs
  add_library(imgui_core STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
  )
  target_include_directories(imgui_core PUBLIC ${imgui_SOURCE_DIR})

  add_library(imgui_impl_glfw STATIC
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  )
  target_include_directories(imgui_impl_glfw PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
  )
  target_link_libraries(imgui_impl_glfw PUBLIC glfw)

  add_library(imgui_impl_opengl3 STATIC
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
  )
  target_include_directories(imgui_impl_opengl3 PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
  )

else()
  # classic X11 targets
  list(APPEND COMMON_SRCS
    xdrum.c
    sbdrum.c
  )
  if(NOT USE_GUS)
    add_compile_definitions(NOGUS)
  endif()
endif()

# ——— Finally: the xdrum executable ———
add_executable(xdrum ${COMMON_SRCS})

if(USE_ALSA)
  target_link_libraries(xdrum
    PRIVATE
      imgui_core
      imgui_impl_glfw
      imgui_impl_opengl3
      ${COMMON_LIBS}
  )
else()
  target_link_libraries(xdrum PRIVATE ${COMMON_LIBS})
endif()

# ——— Usage instructions ———
message(STATUS "Build mode:")
if(USE_ALSA)
  message(STATUS "  ALSA + ImGui modern")
else()
#  if(USE_GUS) message(STATUS "  GUS")
#  if(OTHER)   message(STATUS "  /dev/dsp generic")
#  if(STEREO)  message(STATUS "  /dev/dsp stereo")
endif()

