# PalTrainerUltra — Makefile racine
# Délègue la compilation aux sous-dossiers src/

CXX = g++
WINDRES = windres
CXXFLAGS = -std=c++17 -O2 -Wall -m64 -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE
CXXFLAGS_OVERLAY = -std=c++17 -O0 -Wall -m64 -DWIN32_LEAN_AND_MEAN -DUNICODE -D_UNICODE -Isrc/overlay/imgui -Isrc/overlay/imgui/backends
LDFLAGS_DLL = -shared -static -static-libgcc -static-libstdc++ -lwinpthread -Wl,--subsystem,windows
LDFLAGS_EXE = -static -static-libgcc -static-libstdc++ -lwinpthread -municode
LDFLAGS_SCANNER = -static -static-libgcc -static-libstdc++ -lwinpthread
LDFLAGS_OVERLAY = -static -static-libgcc -static-libstdc++ -lwinpthread -ld3d11 -ldxgi -ld3dcompiler -luser32 -lgdi32 -lshell32 -ldwmapi -lpsapi -mwindows -municode

TRAINER_SRC = src/trainer
OVERLAY_SRC = src/overlay
TOOLS_SRC = src/tools

IMGUI_SRC = $(OVERLAY_SRC)/imgui/imgui.cpp $(OVERLAY_SRC)/imgui/imgui_draw.cpp $(OVERLAY_SRC)/imgui/imgui_widgets.cpp $(OVERLAY_SRC)/imgui/imgui_tables.cpp $(OVERLAY_SRC)/imgui/backends/imgui_impl_win32.cpp $(OVERLAY_SRC)/imgui/backends/imgui_impl_dx11.cpp

all: PalTrainerCore.dll PalTrainerInjector.exe PalOffsetScanner.exe PalTrainerOverlay.exe PalTrainerMiniMap.exe PalTrainerLauncher.exe

# --- Trainer DLL ---
PalTrainerCore.dll: $(TRAINER_SRC)/core.cpp $(TRAINER_SRC)/cheat.cpp $(TRAINER_SRC)/cheat.hpp $(TRAINER_SRC)/engine.cpp $(TRAINER_SRC)/engine.hpp $(TRAINER_SRC)/sdk.hpp $(TRAINER_SRC)/types.hpp $(TRAINER_SRC)/offsets.h $(TRAINER_SRC)/types.h $(TRAINER_SRC)/logger.hpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS_DLL) -o $@ $(TRAINER_SRC)/core.cpp $(TRAINER_SRC)/cheat.cpp $(TRAINER_SRC)/engine.cpp

# --- Injector ---
injector.res: $(TRAINER_SRC)/injector.rc
	$(WINDRES) -O coff -i $(TRAINER_SRC)/injector.rc -o $@

PalTrainerInjector.exe: $(TRAINER_SRC)/injector.cpp $(TRAINER_SRC)/logger.hpp injector.res
	$(CXX) $(CXXFLAGS) $(LDFLAGS_EXE) -o $@ $(TRAINER_SRC)/injector.cpp injector.res

# --- Offset Scanner ---
PalOffsetScanner.exe: $(TOOLS_SRC)/offset_scanner.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS_SCANNER) -o $@ $(TOOLS_SRC)/offset_scanner.cpp

# --- Overlay ---
overlay.res: $(OVERLAY_SRC)/overlay.rc $(OVERLAY_SRC)/overlay.manifest $(OVERLAY_SRC)/palmods.ico
	$(WINDRES) -O coff -I$(OVERLAY_SRC) -i $(OVERLAY_SRC)/overlay.rc -o $@

OVERLAY_OBJS = overlay.o imgui.o imgui_draw.o imgui_widgets.o imgui_tables.o imgui_impl_win32.o imgui_impl_dx11.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

overlay.o: $(OVERLAY_SRC)/overlay.cpp overlay.res
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

PalTrainerOverlay.exe: $(OVERLAY_OBJS) overlay.res
	$(CXX) $(CXXFLAGS_OVERLAY) -o $@ $(OVERLAY_OBJS) overlay.res $(LDFLAGS_OVERLAY)

# --- Minimap ---
minimap.o: $(OVERLAY_SRC)/minimap.cpp $(OVERLAY_SRC)/overlay.cpp $(OVERLAY_SRC)/overlay_minimap.inl overlay.res
	$(CXX) $(CXXFLAGS_OVERLAY) -c -o $@ $<

PalTrainerMiniMap.exe: minimap.o $(filter-out overlay.o,$(OVERLAY_OBJS)) overlay.res
	$(CXX) $(CXXFLAGS_OVERLAY) -o $@ minimap.o $(filter-out overlay.o,$(OVERLAY_OBJS)) overlay.res $(LDFLAGS_OVERLAY)

# --- Launcher ---
LAUNCHER_SRC = src/launcher
launcher.res: $(LAUNCHER_SRC)/launcher.rc $(OVERLAY_SRC)/palmods.ico
	$(WINDRES) -O coff -I$(OVERLAY_SRC) -i $(LAUNCHER_SRC)/launcher.rc -o $@

launcher.o: $(LAUNCHER_SRC)/launcher.cpp
	$(CXX) $(CXXFLAGS_OVERLAY) -Isrc/overlay -c -o $@ $<

PalTrainerLauncher.exe: launcher.o imgui.o imgui_draw.o imgui_widgets.o imgui_tables.o imgui_impl_win32.o imgui_impl_dx11.o launcher.res
	$(CXX) $(CXXFLAGS_OVERLAY) -o $@ launcher.o imgui.o imgui_draw.o imgui_widgets.o imgui_tables.o imgui_impl_win32.o imgui_impl_dx11.o launcher.res $(LDFLAGS_OVERLAY)

clean:
	rm -f PalTrainerCore.dll PalTrainerInjector.exe PalOffsetScanner.exe PalTrainerOverlay.exe PalTrainerMiniMap.exe PalTrainerLauncher.exe overlay.res launcher.res injector.res *.o

.PHONY: all clean
