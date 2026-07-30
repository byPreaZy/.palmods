# PalTrainerUltra — Makefile
# Build targets: PalTrainerUltra.exe + PalTrainerCore.dll + PalOffsetScanner.exe

CXX = g++
WINDRES = windres
CXXFLAGS = -std=c++17 -O2 -Wall -m64 -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE
CXXFLAGS_OVERLAY = -std=c++17 -O0 -Wall -m64 -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE -Isrc/overlay/imgui -Isrc/overlay/imgui/backends
LDFLAGS_DLL = -shared -static -static-libgcc -static-libstdc++ -lwinpthread -Wl,--subsystem,windows
LDFLAGS_SCANNER = -static -static-libgcc -static-libstdc++ -lwinpthread
LDFLAGS_OVERLAY = -static -static-libgcc -static-libstdc++ -lwinpthread -ld3d11 -ldxgi -ld3dcompiler -luser32 -lgdi32 -lshell32 -ldwmapi -lpsapi -mwindows -municode

TRAINER_SRC = src/trainer
OVERLAY_SRC = src/overlay
TOOLS_SRC = src/tools

IMGUI_SRC = $(OVERLAY_SRC)/imgui/imgui.cpp $(OVERLAY_SRC)/imgui/imgui_draw.cpp $(OVERLAY_SRC)/imgui/imgui_widgets.cpp $(OVERLAY_SRC)/imgui/imgui_tables.cpp $(OVERLAY_SRC)/imgui/backends/imgui_impl_win32.cpp $(OVERLAY_SRC)/imgui/backends/imgui_impl_dx11.cpp

all: PalTrainerUltra.exe PalTrainerCore.dll PalOffsetScanner.exe

# --- Trainer DLL ---
PalTrainerCore.dll: $(TRAINER_SRC)/core.cpp $(TRAINER_SRC)/cheat.cpp $(TRAINER_SRC)/cheat.hpp $(TRAINER_SRC)/engine.cpp $(TRAINER_SRC)/engine.hpp $(TRAINER_SRC)/sdk.hpp $(TRAINER_SRC)/types.hpp $(TRAINER_SRC)/offsets.h $(TRAINER_SRC)/types.h $(TRAINER_SRC)/logger.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS_DLL) -o $@ $(TRAINER_SRC)/core.cpp $(TRAINER_SRC)/cheat.cpp $(TRAINER_SRC)/engine.cpp

# --- Offset Scanner ---
PalOffsetScanner.exe: $(TOOLS_SRC)/offset_scanner.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS_SCANNER) -o $@ $(TOOLS_SRC)/offset_scanner.cpp

# --- Unified App (PalTrainerUltra.exe) ---
overlay.res: $(OVERLAY_SRC)/overlay.rc $(OVERLAY_SRC)/overlay.manifest $(OVERLAY_SRC)/palmods.ico
	$(WINDRES) -O coff -I$(OVERLAY_SRC) -i $(OVERLAY_SRC)/overlay.rc -o $@

IMGUI_OBJS = imgui.o imgui_draw.o imgui_widgets.o imgui_tables.o imgui_impl_win32.o imgui_impl_dx11.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

imgui.o: $(OVERLAY_SRC)/imgui/imgui.cpp
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

imgui_draw.o: $(OVERLAY_SRC)/imgui/imgui_draw.cpp
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

imgui_widgets.o: $(OVERLAY_SRC)/imgui/imgui_widgets.cpp
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

imgui_tables.o: $(OVERLAY_SRC)/imgui/imgui_tables.cpp
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

imgui_impl_win32.o: $(OVERLAY_SRC)/imgui/backends/imgui_impl_win32.cpp
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

imgui_impl_dx11.o: $(OVERLAY_SRC)/imgui/backends/imgui_impl_dx11.cpp
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

overlay.o: $(OVERLAY_SRC)/overlay.cpp $(OVERLAY_SRC)/overlay_minimap.inl
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $(OVERLAY_SRC)/overlay.cpp

PalTrainerUltra.exe: overlay.o $(IMGUI_OBJS) overlay.res
	$(CXX) $(CXXFLAGS_OVERLAY) -o $@ overlay.o $(IMGUI_OBJS) overlay.res $(LDFLAGS_OVERLAY)

clean:
	cmd /c "del /q PalTrainerUltra.exe PalTrainerCore.dll PalOffsetScanner.exe overlay.res *.o 2>nul"

.PHONY: all clean
