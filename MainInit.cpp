#include <Windows.h>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <typeinfo>
#include <windows.h>
#include <strsafe.h>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <iterator>
#include <fstream>
#include <intrin.h>
#include <Tlhelp32.h>
#include <CommCtrl.h>
#include <Wininet.h>
#include <Psapi.h>
#include <thread>
#include <ctime>
#include <vector>
#include <stdio.h>
#include <WtsApi32.h>
#include "InternalUI.hpp"


#include <curl.h>

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib, "D3dcompiler.lib")

using namespace std;


int Main() {
	InitInternalUI();
	return 0;
}


BOOL APIENTRY DllMain(HMODULE Module, DWORD Reason, void* Reserved) {
	if (Reason == DLL_PROCESS_ATTACH) {
		CreateThread(NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(Main), NULL, NULL, NULL);
	}
	return TRUE;
}