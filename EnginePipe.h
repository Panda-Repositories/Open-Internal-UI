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


#include <windows.h>
#include <iostream>
#include <shlobj.h>
#pragma comment(lib, "shell32.lib")

namespace Parallel
{

	void Execute(std::string script, std::string pipename)
	{
        const char* convert = script.c_str();

        HANDLE hPipe;
        DWORD  cbRead, cbToWrite, cbWritten, dwMode;
        DWORD dwWritten;
        char Buffer[1024];

        std::string PipeNamed = "\\\\.\\pipe\\" + pipename;
        hPipe = CreateFile(TEXT(PipeNamed.c_str()),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);
        if (hPipe != INVALID_HANDLE_VALUE)
        {
            //WriteFile(hPipe,
            //    Buffer, //How do I put all the data into a buffer to send over to the client?
            //    sizeof(Buffer),   // = length of string + terminating '\0' !!!
            //    &dwWritten,
            //    NULL);

            WriteFile(
                hPipe,                  // pipe handle 
                convert,             // message 
                sizeof(Buffer),              // message length 
                &cbWritten,             // bytes written 
                NULL);                  // not overlapped 


            CloseHandle(hPipe);
        }

	}


}