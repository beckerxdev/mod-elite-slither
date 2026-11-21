// STUB apenas para checar sintaxe do nosso código (NÃO é o Dear ImGui real).
#pragma once
#include <cstddef>
typedef unsigned int ImU32; typedef int ImGuiCond; typedef int ImGuiWindowFlags; typedef int ImGuiCol;
#define IM_COL32(r, g, b, a) ((ImU32)(((a) << 24) | ((b) << 16) | ((g) << 8) | (r)))
#define IMGUI_CHECKVERSION()
struct ImVec2 { float x, y; ImVec2() : x(0), y(0) {} ImVec2(float a, float b) : x(a), y(b) {} };
struct ImVec4 { float x, y, z, w; ImVec4() : x(0), y(0), z(0), w(0) {} ImVec4(float a, float b, float c, float d) : x(a), y(b), z(c), w(d) {} };
enum { ImGuiCond_Always = 1, ImGuiWindowFlags_NoDecoration = 1, ImGuiWindowFlags_NoMove = 2, ImGuiWindowFlags_NoSavedSettings = 4,
       ImGuiCol_WindowBg = 0, ImGuiCol_FrameBg, ImGuiCol_Button, ImGuiCol_ButtonHovered, ImGuiCol_ButtonActive, ImGuiCol_SliderGrab,
       ImGuiCol_CheckMark, ImGuiCol_Header, ImGuiCol_Tab, ImGuiCol_TabSelected, ImGuiCol_COUNT };
struct ImDrawList {
    void AddCircleFilled(ImVec2, float, ImU32, int = 0); void AddCircle(ImVec2, float, ImU32, int = 0, float = 1.f);
    void AddLine(ImVec2, ImVec2, ImU32, float = 1.f); void AddText(ImVec2, ImU32, const char*);
    void AddRectFilled(ImVec2, ImVec2, ImU32, float = 0.f); void AddTriangleFilled(ImVec2, ImVec2, ImVec2, ImU32);
};
struct ImDrawData; struct ImGuiStyle { float WindowRounding, FrameRounding, GrabRounding, TabRounding, FontScaleMain; ImVec2 FramePadding, ItemSpacing, WindowPadding; ImVec4 Colors[ImGuiCol_COUNT]; };
struct ImGuiIO { ImVec2 DisplaySize; const char* IniFilename; bool WantTextInput, WantCaptureMouse; };
namespace ImGui {
ImGuiIO& GetIO(); ImGuiStyle& GetStyle(); void CreateContext(); void DestroyContext(); void StyleColorsDark(); void NewFrame(); void Render(); ImDrawData* GetDrawData();
ImDrawList* GetBackgroundDrawList(); ImDrawList* GetForegroundDrawList(); ImDrawList* GetWindowDrawList();
void SetNextWindowPos(ImVec2, ImGuiCond = 0, ImVec2 = ImVec2()); void SetNextWindowSize(ImVec2); bool Begin(const char*, bool* = nullptr, ImGuiWindowFlags = 0); void End();
void Text(const char*, ...); void TextColored(ImVec4, const char*, ...); void TextDisabled(const char*, ...);
void SameLine(float = 0, float = -1); void Separator(); void NewLine(); void Dummy(ImVec2); ImVec2 GetCursorScreenPos();
bool Button(const char*, ImVec2 = ImVec2()); bool Checkbox(const char*, bool*); bool SliderFloat(const char*, float*, float, float, const char* = "%.3f");
bool SliderInt(const char*, int*, int, int); bool ColorEdit3(const char*, float*); bool ColorEdit4(const char*, float*);
bool Combo(const char*, int*, const char* const[], int); bool InputTextWithHint(const char*, const char*, char*, size_t);
bool BeginTabBar(const char*); void EndTabBar(); bool BeginTabItem(const char*); void EndTabItem(); void SetNextItemWidth(float);
bool IsAnyItemActive(); ImVec2 CalcTextSize(const char*); float GetTextLineHeightWithSpacing();
}
