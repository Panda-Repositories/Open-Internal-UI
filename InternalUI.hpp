const char* remotes = " ";

#pragma region imgui

#include <iostream>
#include <fstream>


#include <Windows.h>
#include <fstream>
#include <d3d11.h>
#include <D3D11Shader.h>
#include <D3Dcompiler.h>
#include "ImGUI\imgui.h"
#include "ImGUI\imgui_impl_dx11.h"
#include "MinHook.h"
#include "ImGUI/TextEditor.h"
#include "Utils.hpp"

#include <windows.h>
#include <string>
#include <iostream>
#include "ConsoleColors.hpp"

#include "EnginePipe.h"
#include "Global.h"

using namespace std;

ID3D11Device* D3D11Device;
ID3D11DeviceContext* D3D11Context;
ID3D11Texture2D* D3D11TextureTarget;
ID3D11RenderTargetView* D3D11RenderView;
IDXGISwapChain* D3D11SwapChain;


typedef HRESULT(__stdcall* HookPresent) (IDXGISwapChain* D3D11SwapChain, UINT SyncInterval, UINT Flags);
HookPresent PresentD3DHooks;


DWORD_PTR* D3D11SwapChain_VTABLE;
DWORD_PTR* D3D11Device_VTABLE;
DWORD_PTR* D3D11Context_VTABLE;

UINT StartingSlot;
D3D11_SHADER_RESOURCE_VIEW_DESC  D3D11ViewDescription;
ID3D11ShaderResourceView* D3D11ShaderResourceView;




std::string GetExeFileName()
{
	char buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	return std::string(buffer);
}

std::string GetExePath()
{
	std::string f = GetExeFileName();
	return f.substr(0, f.find_last_of("\\/"));
}

long ReturnRGBValue(int iR, int iG, int iB, int iA) {

	return ((iA * 256 + iR) * 256 + iG) * 256 + iB;
}

#define RGB_GET(iR, iG, iB, iA) (ReturnRGBValue(iR, iG, iB, iA))





LRESULT CALLBACK DXGIMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return DefWindowProc(hwnd, uMsg, wParam, lParam); }



ImGuiWindowFlags InternalWindowFlags;

WNDPROC OldWindowProc;
bool UICurrentlyShown = true;
bool IsBeingResized = false;

extern IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);




LRESULT CALLBACK InternalWindowProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) {
		return true;
	}

	switch (msg) {
	case WM_SIZE: {
		if (wParam != SIZE_MINIMIZED) {
			ImGui_ImplDX11_InvalidateDeviceObjects();
			D3D11RenderView->Release();
			D3D11SwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
			IsBeingResized = true;
			ImGui_ImplDX11_CreateDeviceObjects();
			return CallWindowProc(OldWindowProc, hwnd, msg, wParam, lParam);
		}
	}
	case WM_SYSCOMMAND: {
		if ((wParam & 0xfff0) == SC_KEYMENU) {
			return 0;
		}
		break;
	}
	case WM_DESTROY: {
		PostQuitMessage(0);
		return 0;
	}
	}

	if (UICurrentlyShown) {
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	else {
		SetCursor(NULL);
		return CallWindowProc(OldWindowProc, hwnd, msg, wParam, lParam);
	}
}


void ExecuteScript(std::string script)
{
	Parallel::Execute(script, "WeAreDevsPublicAPI_Lua"); //WeAreDevsAPI's
	Parallel::Execute(script, "krnlpipe"); //KRNL Pipe
}

bool FirstInit = true;

TextEditor Editor;

double RandInt(int min, int max)
{
	return min + (max - min) * (rand() / RAND_MAX);
}

ImVec4 GetStyleColorVec4new(ImGuiCol idx)
{

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4 c = style.Colors[idx];

	return c;
}
struct RemoteSpy
{
	char                  InputBuf[256];
	ImVector<char*>       Items;

	ImVector<char*>       History;
	int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
	ImGuiTextFilter       Filter;
	bool                  AutoScroll;
	bool                  ScrollToBottom;

	RemoteSpy()
	{
		ClearLog();
		memset(InputBuf, 0, sizeof(InputBuf));
		HistoryPos = -1;
		AutoScroll = true;
		ScrollToBottom = true;

	}
	~RemoteSpy()
	{
		ClearLog();
		for (int i = 0; i < History.Size; i++)
			free(History[i]);
	}

	// Portable helpers
	static int   Stricmp(const char* str1, const char* str2) { int d; while ((d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; } return d; }
	static int   Strnicmp(const char* str1, const char* str2, int n) { int d = 0; while (n > 0 && (d = toupper(*str2) - toupper(*str1)) == 0 && *str1) { str1++; str2++; n--; } return d; }
	static char* Strdup(const char* str) { size_t len = strlen(str) + 1; void* buf = malloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)str, len); }
	static void  Strtrim(char* str) { char* str_end = str + strlen(str); while (str_end > str&& str_end[-1] == ' ') str_end--; *str_end = 0; }

	void    ClearLog()
	{
		for (int i = 0; i < Items.Size; i++)
			free(Items[i]);
		Items.clear();
		ScrollToBottom = true;
	}

	void    AddLog(const char* fmt, ...) IM_FMTARGS(2)
	{
		// FIXME-OPT
		char buf[1024];
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
		buf[IM_ARRAYSIZE(buf) - 1] = 0;
		va_end(args);
		Items.push_back(Strdup(buf));
		if (AutoScroll)
			ScrollToBottom = true;
	}

	void    Draw(const char* title, bool* p_open)
	{
		ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);
		if (!ImGui::Begin(title, p_open))
		{
			ImGui::End();
			return;
		}

		// As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar. So e.g. IsItemHovered() will return true when hovering the title bar.
		// Here we create a context menu only available from the title bar.
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Close Console"))
				*p_open = false;
			ImGui::EndPopup();
		}





		if (ImGui::SmallButton("Clear")) { ClearLog(); } ImGui::SameLine();
		bool copy_to_clipboard = ImGui::SmallButton("Copy"); ImGui::SameLine();
		if (ImGui::SmallButton("Scroll to bottom")) ScrollToBottom = true;
		//static float t = 0.0f; if (ImGui::GetTime() - t > 0.02f) { t = ImGui::GetTime(); AddLog("Spam %f", t); }

		ImGui::Separator();

		// Options menu
		if (ImGui::BeginPopup("Options"))
		{
			if (ImGui::Checkbox("Auto-scroll", &AutoScroll))
				if (AutoScroll)
					ScrollToBottom = true;
			ImGui::EndPopup();
		}

		// Options, Filter
		if (ImGui::Button("Options"))
			ImGui::OpenPopup("Options");
		ImGui::SameLine();
		Filter.Draw("Filter (\"Remote name\") (\"Argument\")", 180);
		ImGui::Separator();

		const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Clear")) ClearLog();
			ImGui::EndPopup();
		}

		// Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text you can use ImGui::TextUnformatted(log.begin(), log.end());
		// NB- if you have thousands of entries this approach may be too inefficient and may require user-side clipping to only process visible items.
		// You can seek and display only the lines that are visible using the ImGuiListClipper helper, if your elements are evenly spaced and you have cheap random access to the elements.
		// To use the clipper we could replace the 'for (int i = 0; i < Items.Size; i++)' loop with:
		//     ImGuiListClipper clipper(Items.Size);
		//     while (clipper.Step())
		//         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
		// However, note that you can not use this code as is if a filter is active because it breaks the 'cheap random-access' property. We would need random-access on the post-filtered list.
		// A typical application wanting coarse clipping and filtering may want to pre-compute an array of indices that passed the filtering test, recomputing this array when user changes the filter,
		// and appending newly elements as they are inserted. This is left as a task to the user until we can manage to improve this example code!
		// If your items are of variable size you may want to implement code similar to what ImGuiListClipper does. Or split your data into fixed height items to allow random-seeking into your list.
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		if (copy_to_clipboard)
			ImGui::LogToClipboard();
		for (int i = 0; i < Items.Size; i++)
		{
			const char* item = Items[i];
			if (!Filter.PassFilter(item))
				continue;

			// Normally you would store more information in your item (e.g. make Items[] an array of structure, store color/type etc.)
			bool pop_color = false;
			if (strstr(item, "[error]")) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f)); pop_color = true; }
			else if (strncmp(item, "# ", 2) == 0) { ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f)); pop_color = true; }
			ImGui::TextUnformatted(item);
			if (pop_color)
				ImGui::PopStyleColor();
		}
		if (copy_to_clipboard)
			ImGui::LogFinish();
		if (ScrollToBottom)
			ImGui::SetScrollHereY(1.0f);
		ScrollToBottom = false;
		ImGui::PopStyleVar();
		ImGui::EndChild();
		ImGui::Separator();

		if (ImGui::Button("Start Remotespy!"))
		{
			ExecuteScript(Utils::DownloadStringFromUrl("https://pastebin.com/raw/S1pBUnDT").c_str());
		}






		ImGui::End();



	}
};
static int overlaycorner = 2;
static void Overlay(bool* p_open)
{
	const float DISTANCE = 10.0f;

	ImGuiIO& io = ImGui::GetIO();
	if (overlaycorner != -1)
	{
		ImVec2 window_pos = ImVec2((overlaycorner & 1) ? io.DisplaySize.x - DISTANCE : DISTANCE, (overlaycorner & 2) ? io.DisplaySize.y - DISTANCE : DISTANCE);
		ImVec2 window_pos_pivot = ImVec2((overlaycorner & 1) ? 1.0f : 0.0f, (overlaycorner & 2) ? 1.0f : 0.0f);
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
	}
	ImGui::SetNextWindowBgAlpha(0.0f); // Transparent background

	ImGui::Begin(" ", p_open, (overlaycorner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground);




	ImGui::TextColored(ImColor(0, 0, 0), "C");
	ImGui::SameLine();
	ImGui::TextColored(ImColor(255, 255, 255), "O");
	ImGui::SameLine();
	ImGui::TextColored(ImColor(0, 0, 0), "C");
	ImGui::SameLine();
	ImGui::TextColored(ImColor(255, 255, 255), "A");
	ImGui::SameLine();
	ImGui::TextColored(ImColor(0, 0, 0), "I");
	ImGui::SameLine();
	ImGui::TextColored(ImColor(255, 255, 255), "N");
	ImGui::SameLine();
	ImGui::TextColored(ImColor(0, 0, 0), "E");







	ImGui::End();
}


static RemoteSpy remotspyimgui;
HRESULT CALLBACK OnHookPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
	if (FirstInit) {

		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&D3D11Device))) {
			pSwapChain->GetDevice(__uuidof(D3D11Device), (void**)&D3D11Device);
			D3D11Device->GetImmediateContext(&D3D11Context);
		}
		else
		{
			std::cout << ired << "Internal UI Init Failed: D3D Device Hook Failed!";
			throw std::exception("Internal UI Init Failure!");
		}
		if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&D3D11TextureTarget))) {
			D3D11Device->CreateRenderTargetView(D3D11TextureTarget, NULL, &D3D11RenderView);
			D3D11TextureTarget->Release();
		}
		else
		{
			std::cout << ired << "Internal UI Init Failed: Render Target View Faiiled!";
			throw std::exception("Internal UI Init Failure!");
		}

		ImGui_ImplDX11_Init(FindWindowExW(NULL, NULL, NULL, L"Roblox"), D3D11Device, D3D11Context); //Modified ImGUI Implementation for Roblox (Retards Make Bypassing FindWindowA Easy As Fuck)
		ImGui_ImplDX11_CreateDeviceObjects();

		FirstInit = false;
	}


	if (IsBeingResized) {
		if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&D3D11TextureTarget))) {
			D3D11Device->CreateRenderTargetView(D3D11TextureTarget, NULL, &D3D11RenderView);
			D3D11TextureTarget->Release();
			std::cout << igreen << ("Internal UI Resize ReInit Success!");
			IsBeingResized = false;
		}
	}
	D3D11Context->OMSetRenderTargets(1, &D3D11RenderView, NULL);
	if ((GetAsyncKeyState(VK_HOME) & 1)) {
		UICurrentlyShown = !UICurrentlyShown;
	}


	if (UICurrentlyShown) {

		ImGuiStyle* style = &ImGui::GetStyle();
#define hovercolor ImColor(143, 152, 246);//on hover 170, 8, 79
#define normalcolor ImColor(88, 101, 242);//main color 217, 8, 79
		style->WindowPadding = ImVec2(6, 4);
		style->WindowRounding = 0.0f;
		style->FramePadding = ImVec2(5, 2);
		style->FrameRounding = 3.0f;
		style->ItemSpacing = ImVec2(7, 7);
		style->ItemInnerSpacing = ImVec2(1, 1);
		style->TouchExtraPadding = ImVec2(0, 0);
		style->IndentSpacing = 6.0f;
		style->ScrollbarSize = 12.0f;
		style->ScrollbarRounding = 16.0f;
		style->GrabMinSize = 20.0f;
		style->GrabRounding = 2.0f;
		style->WindowTitleAlign.x = 0.50f;
		style->Colors[ImGuiCol_Border] = ImVec4(0.539f, 0.479f, 0.255f, 0.162f);
		style->FrameBorderSize = 0.0f;
		style->ItemSpacing = ImVec2(7, 4);
		style->WindowBorderSize = 1.0f;
		style->Colors[ImGuiCol_ScrollbarGrab] = normalcolor;
		//style->Colors[ImGuiCol_ScrollbarGrabHovered] = hovercolor;
		style->Colors[ImGuiCol_ScrollbarGrabActive] = normalcolor;
		style->Colors[ImGuiCol_Tab] = normalcolor;
		style->Colors[ImGuiCol_TabActive] = normalcolor;
		style->Colors[ImGuiCol_TabHovered] = hovercolor;

		style->Colors[ImGuiCol_ResizeGrip] = GetStyleColorVec4new(ImGuiCol_WindowBg);
		style->Colors[ImGuiCol_ResizeGripActive] = GetStyleColorVec4new(ImGuiCol_WindowBg);
		style->Colors[ImGuiCol_ResizeGripHovered] = GetStyleColorVec4new(ImGuiCol_WindowBg);

		//style->Colors[ImGuiCol_ChildWindowBg] = normalcolor;
		style->Colors[ImGuiCol_Border] = normalcolor;
		style->Colors[ImGuiCol_BorderShadow] = normalcolor;
		style->Colors[ImGuiCol_TitleBgActive] = normalcolor;
		style->Colors[ImGuiCol_MenuBarBg] = normalcolor;
		style->Colors[ImGuiCol_CheckMark] = normalcolor;
		style->Colors[ImGuiCol_SliderGrab] = normalcolor;
		style->Colors[ImGuiCol_Button] = normalcolor;
		style->Colors[ImGuiCol_ButtonHovered] = hovercolor;
		style->Colors[ImGuiCol_HeaderHovered] = hovercolor;
		style->Colors[ImGuiCol_HeaderActive] = normalcolor;

		ImGui_ImplDX11_NewFrame();



		const char* c = GlobalBalls::Internal_UI_Title.c_str();
		ImGui::Begin(c);
		ImGui::SetWindowSize(ImVec2(Window_Size_X, Window_Size_Y));



		ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;


		if (ImGui::BeginTabBar("Executor", tab_bar_flags))
		{
			if (ImGui::BeginTabItem("Execution"))
			{

				Editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
				Editor.Render("Script Editor", ImVec2(575, 259), false);

				if (ImGui::Button("Execute", ImVec2(70, 20))) {
					ExecuteScript(Editor.GetText());
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear", ImVec2(70, 20))) {
					Editor.SetText("");
				}

				ImGui::SameLine();
				if (ImGui::Button("Open", ImVec2(70, 20))) {
					char file_name[MAX_PATH];

					OPENFILENAMEA ofn;
					ZeroMemory(&file_name, sizeof(file_name));
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.lpstrFilter = "Script (*.txt;*.lua)|*.txt;*.lua|All files (*.*)|*.*";
					ofn.lpstrFile = file_name;
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrTitle = "Select a File";
					ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

					std::string Buffer;

					if (GetOpenFileNameA(&ofn)) {
						Buffer = Utils::ReadFile(file_name);
					}

					Editor.SetText(Buffer);
				}

				ImGui::SameLine();
				if (ImGui::Button("Save", ImVec2(70, 20))) {
					char file_name[MAX_PATH];

					OPENFILENAMEA ofn;
					ZeroMemory(&file_name, sizeof(file_name));
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.lpstrFilter = "Script (*.txt;*.lua)|*.txt;*.lua|All files (*.*)|*.*";
					ofn.lpstrFile = file_name;
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrTitle = "Save File";
					ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

					if (GetSaveFileNameA(&ofn)) {
						std::string buffer = Editor.GetText();
						Utils::WriteFile(file_name, buffer);
					}
				}

				//ImGui::SameLine();
				//if (ImGui::Button("Attach", ImVec2(70, 20))) 
				//{
				//	std::cout << igreen << (GetExePath());
				//}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}


	ImGui::GetIO().MouseDrawCursor = UICurrentlyShown;

	return PresentD3DHooks(pSwapChain, SyncInterval, Flags);
}


const int MultiSampleCount = 1;


int InitInternalUI() {

	//Sloppeys FindWindowA Bypass



	HMODULE hDXGIDLL = 0;
	do {
		hDXGIDLL = GetModuleHandleA("dxgi.dll");
		Sleep(500);
	} while (!hDXGIDLL);

	WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DXGIMsgProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
	RegisterClassExA(&wc);
	HWND hWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);

	D3D_FEATURE_LEVEL requestedLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
	D3D_FEATURE_LEVEL obtainedLevel;
	ID3D11Device* d3dDevice = nullptr;
	ID3D11DeviceContext* d3dContext = nullptr;

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;


	UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };


	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &D3D11SwapChain, &D3D11Device, &featureLevel, &D3D11Context))) {
		std::cout << ired << "Internal UI Init Failed: D3D Device Creation And Chain Swap Failed!";
		throw std::exception("Internal UI Init Failure!");
	}


	OldWindowProc = (WNDPROC)SetWindowLongPtr(FindWindowExW(NULL, NULL, NULL, L"Roblox"), GWL_WNDPROC, (LONG)InternalWindowProcHandler);

	ImGui::CreateContext();

	ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;


	InternalWindowFlags |= ImGuiWindowFlags_NoResize;

	style->WindowRounding = 0;
	style->WindowTitleAlign = ImVec2(0.01, 0.5);
	//style->Alpha = 0.9;
	style->GrabRounding = 1;
	style->GrabMinSize = 20;
	style->FrameRounding = 0;


	D3D11SwapChain_VTABLE = (DWORD_PTR*)D3D11SwapChain;
	D3D11SwapChain_VTABLE = (DWORD_PTR*)D3D11SwapChain_VTABLE[0];

	D3D11Context_VTABLE = (DWORD_PTR*)D3D11Context;
	D3D11Context_VTABLE = (DWORD_PTR*)D3D11Context_VTABLE[0];

	D3D11Device_VTABLE = (DWORD_PTR*)D3D11Device;
	D3D11Device_VTABLE = (DWORD_PTR*)D3D11Device_VTABLE[0];


	if (MH_Initialize() != MH_OK) {
		std::cout << ired << "Internal UI Init Failed: MinHook Init Failed!";
		throw std::exception("Internal UI Init Failure!");
	}

	if (MH_CreateHook((DWORD_PTR*)D3D11SwapChain_VTABLE[8], OnHookPresent, reinterpret_cast<void**>(&PresentD3DHooks)) != MH_OK) {
		std::cout << ired << "Internal UI Init Failed: Hook Creation Failed!";
		throw std::exception("Internal UI Init Failure!");
	}

	if (MH_EnableHook((DWORD_PTR*)D3D11SwapChain_VTABLE[8]) != MH_OK) {
		std::cout << ired << "Internal UI Init Failed: Hook Enable Failed!";
		throw std::exception("Internal UI Init Failure!");
	}

	//D3D11Device->Release();
	//D3D11Context->Release();
	//D3D11SwapChain->Release();
	return 0;
}
#pragma endregion

