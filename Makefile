# PalTrainerUltra — Makefile
# Build targets: PalTrainerUltra.exe + PalTrainerCore.dll + PalOffsetScanner.exe

CXX = g++
WINDRES = windres
CXXFLAGS = -std=c++17 -O2 -Wall -m64 -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE
CXXFLAGS_OVERLAY = -std=c++17 -O0 -Wall -m64 -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE -Isrc/overlay/imgui -Isrc/overlay/imgui/backends
LDFLAGS_DLL = -shared -static -static-libgcc -static-libstdc++ -lwinpthread -Wl,--subsystem,windows
LDFLAGS_SCANNER = -static -static-libgcc -static-libstdc++ -lwinpthread
LDFLAGS_OVERLAY = -static -static-libgcc -static-libstdc++ -lwinpthread -ld3d11 -ldxgi -ld3dcompiler -luser32 -lgdi32 -lshell32 -ldwmapi -lpsapi -lwindowscodecs -lole32 -loleaut32 -mwindows -municode

TRAINER_SRC = src/trainer
OVERLAY_SRC = src/overlay
TOOLS_SRC = src/tools
BUILD_DIR = build

IMGUI_SRC = $(OVERLAY_SRC)/imgui/imgui.cpp $(OVERLAY_SRC)/imgui/imgui_draw.cpp $(OVERLAY_SRC)/imgui/imgui_widgets.cpp $(OVERLAY_SRC)/imgui/imgui_tables.cpp $(OVERLAY_SRC)/imgui/backends/imgui_impl_win32.cpp $(OVERLAY_SRC)/imgui/backends/imgui_impl_dx11.cpp

all: PalTrainerUltra.exe PalTrainerCore.dll PalOffsetScanner.exe

# --- Trainer DLL ---
PalTrainerCore.dll: $(TRAINER_SRC)/core.cpp $(TRAINER_SRC)/cheat.cpp $(TRAINER_SRC)/cheat.hpp $(TRAINER_SRC)/engine.cpp $(TRAINER_SRC)/engine.hpp $(TRAINER_SRC)/sdk.hpp $(TRAINER_SRC)/types.hpp $(TRAINER_SRC)/offsets.h $(TRAINER_SRC)/types.h $(TRAINER_SRC)/logger.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS_DLL) -o $@ $(TRAINER_SRC)/core.cpp $(TRAINER_SRC)/cheat.cpp $(TRAINER_SRC)/engine.cpp

# --- Offset Scanner ---
PalOffsetScanner.exe: $(TOOLS_SRC)/offset_scanner.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS_SCANNER) -o $@ $(TOOLS_SRC)/offset_scanner.cpp

# --- Unified App (PalTrainerUltra.exe) ---
IMGUI_OBJS = $(BUILD_DIR)/imgui.o $(BUILD_DIR)/imgui_draw.o $(BUILD_DIR)/imgui_widgets.o $(BUILD_DIR)/imgui_tables.o $(BUILD_DIR)/imgui_impl_win32.o $(BUILD_DIR)/imgui_impl_dx11.o

$(BUILD_DIR):
	cmd /c "if not exist build mkdir build"

$(BUILD_DIR)/overlay.res: $(OVERLAY_SRC)/overlay.rc $(OVERLAY_SRC)/overlay.manifest $(OVERLAY_SRC)/palmods.ico | $(BUILD_DIR)
	$(WINDRES) -O coff -I$(OVERLAY_SRC) -i $(OVERLAY_SRC)/overlay.rc -o $@

$(BUILD_DIR)/%.o: $(OVERLAY_SRC)/imgui/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

$(BUILD_DIR)/imgui.o: $(OVERLAY_SRC)/imgui/imgui.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

$(BUILD_DIR)/imgui_draw.o: $(OVERLAY_SRC)/imgui/imgui_draw.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

$(BUILD_DIR)/imgui_widgets.o: $(OVERLAY_SRC)/imgui/imgui_widgets.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

$(BUILD_DIR)/imgui_tables.o: $(OVERLAY_SRC)/imgui/imgui_tables.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

$(BUILD_DIR)/imgui_impl_win32.o: $(OVERLAY_SRC)/imgui/backends/imgui_impl_win32.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

$(BUILD_DIR)/imgui_impl_dx11.o: $(OVERLAY_SRC)/imgui/backends/imgui_impl_dx11.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

$(BUILD_DIR)/overlay.o: $(OVERLAY_SRC)/overlay.cpp $(OVERLAY_SRC)/overlay_minimap.inl | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $(OVERLAY_SRC)/overlay.cpp

PalTrainerUltra.exe: $(BUILD_DIR)/overlay.o $(IMGUI_OBJS) $(BUILD_DIR)/overlay.res
	$(CXX) $(CXXFLAGS_OVERLAY) -o $@ $(BUILD_DIR)/overlay.o $(IMGUI_OBJS) $(BUILD_DIR)/overlay.res $(LDFLAGS_OVERLAY)

clean:
	cmd /c "del /q PalTrainerUltra.exe PalTrainerCore.dll PalOffsetScanner.exe 2>nul"
	cmd /c "if exist build rmdir /s /q build"

.PHONY: all clean
