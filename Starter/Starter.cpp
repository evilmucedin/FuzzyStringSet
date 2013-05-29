#include <cstdio>
#include <windows.h>
#include <time.h>
#include <psapi.h>

int main()
{
    STARTUPINFOW siStartupInfo; 
    PROCESS_INFORMATION piProcessInfo; 
    memset(&siStartupInfo, 0, sizeof(siStartupInfo)); 
    memset(&piProcessInfo, 0, sizeof(piProcessInfo)); 
    siStartupInfo.cb = sizeof(siStartupInfo);
    const clock_t cBefore = clock();
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	printf("Granularity: %d\n", static_cast<int>(si.dwAllocationGranularity));
    if (CreateProcess(L"check.exe", L"check.exe url_compressed.bin c:\\Temp\\Competition\\small_url_batch_in.tsv small_url_batch_res.tsv", 0, 0, false, CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo, &piProcessInfo))
    {
        const DWORD dwExitCode = WaitForSingleObject(piProcessInfo.hProcess, INFINITE);
		PROCESS_MEMORY_COUNTERS memoryCounters;
		memset(&memoryCounters, 0, sizeof(memoryCounters));
		GetProcessMemoryInfo(piProcessInfo.hProcess, &memoryCounters, sizeof(memoryCounters));
		fprintf(stderr, "CreateProcess %d %f %f\n", (int)dwExitCode, static_cast<float>(clock() - cBefore)/CLK_TCK, static_cast<float>(memoryCounters.PeakWorkingSetSize)/1024/1024);
    }
    else
    {
        fprintf(stderr, "!CreateProcess\n");
    }
    return 0;
}