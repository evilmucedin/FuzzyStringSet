#include <cstdio>
#include <windows.h>
#include <time.h>

int main()
{
    STARTUPINFOW siStartupInfo; 
    PROCESS_INFORMATION piProcessInfo; 
    memset(&siStartupInfo, 0, sizeof(siStartupInfo)); 
    memset(&piProcessInfo, 0, sizeof(piProcessInfo)); 
    siStartupInfo.cb = sizeof(siStartupInfo);
    const clock_t cBefore = clock();
    if (CreateProcess(L"dummy.exe", L"", 0, 0, false, CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo, &piProcessInfo))
    {
        const DWORD dwExitCode = WaitForSingleObject(piProcessInfo.hProcess, INFINITE);
        fprintf(stderr, "CreateProcess %d %f\n", (int)dwExitCode, static_cast<float>(clock() - cBefore)/CLK_TCK);
    }
    else
    {
        fprintf(stderr, "!CreateProcess\n");
    }
    return 0;
}