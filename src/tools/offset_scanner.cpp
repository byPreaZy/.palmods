// PalOffsetScanner.exe
// Scanne l'EXE de Palworld (ou un processus en cours) pour les motifs AOB et écrit runtime_offsets.json.
// Compilation : x86_64-w64-mingw32-g++ -std=c++17 -O2 -o PalOffsetScanner.exe offset_scanner.cpp

#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cstring>

struct Pattern {
    const char* name;
    const char* pattern;
    int targetInstrLen; // nombre d'octets de l'instruction correspondante ; 0 = utiliser l'adresse de correspondance directement (début de fonction)
    const char* altPattern;  // pattern alternatif (UE5)
    int altTargetInstrLen;   // longueur instruction pour le pattern alternatif
};

static std::vector<uint8_t> parsePattern(const std::string& s, std::vector<bool>& mask) {
    std::vector<uint8_t> bytes;
    mask.clear();
    for (size_t i = 0; i < s.size();) {
        if (s[i] == ' ' || s[i] == '\t') { ++i; continue; }
        if (s[i] == '?') {
            bytes.push_back(0);
            mask.push_back(false);
            ++i;
            if (i < s.size() && s[i] == '?') ++i;
            continue;
        }
        std::string hex = s.substr(i, 2);
        bytes.push_back((uint8_t)std::stoul(hex, nullptr, 16));
        mask.push_back(true);
        i += 2;
    }
    return bytes;
}

static bool aobMatch(const uint8_t* data, const std::vector<uint8_t>& bytes, const std::vector<bool>& mask) {
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (mask[i] && data[i] != bytes[i]) return false;
    }
    return true;
}

static uint32_t readRel32(const uint8_t* p) {
    uint32_t v = 0;
    memcpy(&v, p, 4);
    return v;
}

static std::string toHex(uint64_t v) {
    char buf[32];
    snprintf(buf, sizeof(buf), "0x%llX", (unsigned long long)v);
    return buf;
}

int main(int argc, char* argv[]) {
    const char* path = (argc > 1) ? argv[1] : "Palworld-Win64-Shipping.exe";
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open " << path << "\n";
        return 1;
    }

    IMAGE_DOS_HEADER dos;
    file.read((char*)&dos, sizeof(dos));
    if (dos.e_magic != 0x5A4D) {
        std::cerr << "Not a PE file (bad DOS header)\n";
        return 1;
    }

    file.seekg(dos.e_lfanew, std::ios::beg);
    IMAGE_NT_HEADERS64 nt;
    file.read((char*)&nt, sizeof(nt));
    if (nt.Signature != 0x00004550) {
        std::cerr << "Not a PE file (bad NT header)\n";
        return 1;
    }

    uint64_t imageBase = nt.OptionalHeader.ImageBase;
    std::vector<IMAGE_SECTION_HEADER> sections(nt.FileHeader.NumberOfSections);
    file.read((char*)sections.data(), sizeof(IMAGE_SECTION_HEADER) * nt.FileHeader.NumberOfSections);

    // Trouver la section .text
    IMAGE_SECTION_HEADER textSec{};
    bool foundText = false;
    for (const auto& s : sections) {
        if (strncmp((const char*)s.Name, ".text", 5) == 0) {
            textSec = s;
            foundText = true;
            break;
        }
    }
    if (!foundText) {
        std::cerr << "No .text section found\n";
        return 1;
    }

    std::vector<uint8_t> text(textSec.SizeOfRawData);
    file.seekg(textSec.PointerToRawData, std::ios::beg);
    file.read((char*)text.data(), textSec.SizeOfRawData);

    // Motifs AOB — patterns UE4 (principal) + UE5 (fallback)
    // Palworld 1.0 utilise UE5; les patterns UE5 sont essayés si UE4 ne matche pas.
    std::vector<Pattern> patterns = {
        {"GWorld",
         "48 8B 1D ? ? ? ? 48 85 DB 74 ? 41 B0", 7,
         "48 8B 05 ? ? ? ? 48 3B C8 75", 7},
        {"GObject",
         "48 8B 05 ? ? ? ? 48 8B 0C C8 4C 8D 04 D1 EB 03", 7,
         "48 8B 05 ? ? ? ? 48 8B 0C C8 4C 8D 04 D1 EB 03", 7},
        {"FName",
         "48 8D 05 ? ? ? ? EB 13", 7,
         "48 8D 05 ? ? ? ? EB 13", 7},
        {"ProcessEvent",
         "40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC 10 01 00 00 48 8D", 0,
         "40 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24", 0},
        {"AppendString",
         "48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 20 80 3D 10 00 00 00 00", 0,
         "48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 20 80 3D 10 00 00 00 00", 0},
        {"Tick",
         "48 89 5C 24 ? 57 48 83 EC 60 48 8B F9 E8 ? ? ? ? 48 8B", 1,
         "48 89 5C 24 ? 57 48 83 EC 60 48 8B F9 E8 ? ? ? ? 48 8B", 1}
    };

    // Construire une map plate nom -> rva pour faciliter le chargement du core
    bool detectedUE5 = false;
    int ue5Count = 0, ue4Count = 0;
    std::vector<std::pair<std::string, std::string>> results;
    for (const auto& pat : patterns) {
        std::vector<bool> mask;
        std::vector<uint8_t> bytes = parsePattern(pat.pattern, mask);
        std::vector<size_t> matches;
        for (size_t i = 0; i + bytes.size() <= text.size(); ++i) {
            if (aobMatch(text.data() + i, bytes, mask)) matches.push_back(i);
        }

        // Try UE5 alt pattern if primary didn't match
        if (matches.empty() && pat.altPattern && pat.altPattern[0] != '\0') {
            std::vector<bool> altMask;
            std::vector<uint8_t> altBytes = parsePattern(pat.altPattern, altMask);
            for (size_t i = 0; i + altBytes.size() <= text.size(); ++i) {
                if (aobMatch(text.data() + i, altBytes, altMask)) matches.push_back(i);
            }
            if (!matches.empty()) {
                ue5Count++;
                std::cout << "[INFO] " << pat.name << ": matched UE5 pattern" << std::endl;
            }
        } else if (!matches.empty()) {
            ue4Count++;
        }

        uint64_t rva = 0;

        if (pat.name == std::string("Tick")) {
            if (matches.size() >= 2) {
                size_t idx = matches[1];
                rva = textSec.VirtualAddress + idx;
            } else if (!matches.empty()) {
                rva = textSec.VirtualAddress + matches[0];
            }
        } else if (!matches.empty()) {
            size_t idx = matches[0];
            uint64_t instrAddr = imageBase + textSec.VirtualAddress + idx;
            int instrLen = pat.targetInstrLen;
            // Use alt instruction length if we matched via alt pattern and primary had no matches
            if (ue5Count > ue4Count && pat.altTargetInstrLen != pat.targetInstrLen) {
                instrLen = pat.altTargetInstrLen;
            }
            if (instrLen == 7) {
                uint32_t rel = readRel32(text.data() + idx + 3);
                uint64_t target = instrAddr + 7 + rel;
                rva = target - imageBase;
            } else if (instrLen == 0) {
                rva = instrAddr - imageBase;
            }
        }

        std::string key = pat.name;
        key += "Rva";
        results.emplace_back(key, toHex(rva));
    }

    detectedUE5 = (ue5Count > ue4Count);
    std::string engineVer = detectedUE5 ? "UE5" : "UE4";

    std::ofstream out("runtime_offsets.json");
    out << "{\n";
    out << "  \"imageBase\": \"" << toHex(imageBase) << "\",\n";
    out << "  \"engineVersion\": \"" << engineVer << "\",\n";
    out << "  \"timestamp\": " << nt.FileHeader.TimeDateStamp << ",\n";
    out << "  \"path\": \"" << path << "\",\n";
    bool first = true;
    for (const auto& kv : results) {
        if (!first) out << ",\n";
        first = false;
        out << "  \"" << kv.first << "\": \"" << kv.second << "\"";
    }
    out << "\n}\n";
    out.close();

    std::cout << "Wrote runtime_offsets.json (Engine: " << engineVer << ")" << std::endl;
    return 0;
}
