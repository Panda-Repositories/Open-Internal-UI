#pragma once
#include <Windows.h>
#pragma comment(lib, "urlmon.lib")

#include <urlmon.h>
#include <sstream>
#include <fstream>


namespace Utils {
	void GetFile(const char* dllName, const char* fileName, char* buffer, int bfSize);
	std::string DownloadStringFromUrl(std::string url);
	std::string IntegerToString(DWORD fuck);
	std::string ReadFile(char file_name[MAX_PATH]);
	void WriteFile(char file_name[MAX_PATH], std::string buffer);
}