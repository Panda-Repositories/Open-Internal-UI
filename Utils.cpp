#include "Utils.hpp"


void Utils::GetFile(const char* dllName, const char* fileName, char* buffer, int bfSize) {
	GetModuleFileName(GetModuleHandle(dllName), buffer, bfSize);
	if (strlen(fileName) + strlen(buffer) < MAX_PATH) {
		char* pathEnd = strrchr(buffer, '\\');
		strcpy(pathEnd + 1, fileName);
	}
	else {
		*buffer = 0;
	}
}

std::string Utils::ReadFile(char file_name[MAX_PATH])
{
	std::ifstream file(file_name);
	std::string str;
	std::string file_contents;
	while (getline(file, str))
	{
		file_contents += str;
		file_contents.push_back('\n');
	}
	return file_contents;
}

void Utils::WriteFile(char file_name[MAX_PATH], std::string buffer)
{
	std::ofstream myfile;
	myfile.open(file_name);
	myfile << buffer;
	myfile.close();
}


std::string Utils::DownloadStringFromUrl(std::string url) {
	IStream* stream;
	HRESULT result = URLOpenBlockingStream(0, url.c_str(), &stream, 0, 0);
	if (result != 0)
	{
		return "";
	}
	char buffer[100];
	unsigned long bytesRead;
	std::stringstream ss;
	stream->Read(buffer, 100, &bytesRead);
	while (bytesRead > 0U)
	{
		ss.write(buffer, (long long)bytesRead);
		stream->Read(buffer, 100, &bytesRead);
	}
	stream->Release();
	return ss.str();
}

std::string Utils::IntegerToString(DWORD fuck) {

	std::stringstream stream;
	stream << fuck;
	return stream.str();
}