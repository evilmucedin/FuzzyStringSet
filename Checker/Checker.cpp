#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "..\Common\FileIO.h"
#include "..\Common\Types.h"
#include "..\Common\SetParameters.h"
#include "..\Common\Timer.h"

#include "windows.h"

class THashChecker
{
private:
	HANDLE m_File;
	HANDLE m_Map;
	ui32 m_Size;
	LPVOID m_View;
	const TBucket* m_Buckets;
	SYSTEM_INFO m_SI;
	char* m_buffer;

public:
	THashChecker(const std::string& filename)
	{
		m_File = CreateFileA(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
		if (INVALID_HANDLE_VALUE == m_File)
		{
			throw std::exception("Bad CreateFile");
		}
		m_Size = GetFileSize(m_File, 0);
		m_Map = CreateFileMapping(m_File, NULL, PAGE_READONLY, 0, m_Size, NULL);
		if (!m_Map)
		{
			throw std::exception("Bad CreateFileMapping");
		}
		m_View = MapViewOfFile(m_Map, FILE_MAP_READ, 0, 0, (NBUCKETS + 1)*sizeof(TBucket));
		if (!m_View)
		{
			throw std::exception("Bad MapViewOfFile");
		}
		m_Buckets = static_cast<const TBucket*>(m_View);
		GetSystemInfo(&m_SI);
		m_buffer = new char[1 << 20];
	}
		
	bool Has(ui64 hash) throw()
	{
		const size_t iBucket = static_cast<size_t>((hash*NBUCKETS)/NHASHES);
		
		const size_t offset = (NBUCKETS + 1)*sizeof(TBucket) + m_Buckets[iBucket].m_Offset*sizeof(ui16);
		const size_t iPage = offset / m_SI.dwAllocationGranularity;
		const size_t shift = offset % m_SI.dwAllocationGranularity;
		const size_t blockSize = m_Buckets[iBucket + 1].m_Offset - m_Buckets[iBucket].m_Offset;

		const size_t address = iPage*m_SI.dwAllocationGranularity;
		
		/*
		LARGE_INTEGER liOffset;
		memset(&liOffset, 0, sizeof(liOffset));
		liOffset.LowPart = offset;
		if (!SetFilePointerEx(m_File, liOffset, 0, FILE_BEGIN))
		{
			throw std::exception("Bad SetFilePointer");			
		}
		if (!ReadFile(m_File, m_buffer, blockSize*sizeof(ui16), 0, 0))
		{
			throw std::exception("Bad ReadFile");			
		}
		const ui16* data = reinterpret_cast<ui16*>(m_buffer);
		*/
		const void* view = MapViewOfFile(m_Map, FILE_MAP_READ, 0, address, shift + blockSize*sizeof(ui16));
		if (!view)
		{
			throw std::exception("Bad MapViewOfFile");
		}
		const ui16* data = reinterpret_cast<const ui16*>((const char*)view + shift);

		const ui16* stop = data + blockSize*sizeof(ui16);
		const ui64 hashStart = (iBucket*NHASHES)/NBUCKETS;
		ui64 now = hashStart;
		do 
		{
			now += *data;
			++data;
		}
		while ( (data < stop) && (now < hash) );

		UnmapViewOfFile(view);

		return now == hash;
	}
	
	~THashChecker()
	{
		delete[] m_buffer;
		UnmapViewOfFile(m_View);
		CloseHandle(m_Map);
		CloseHandle(m_File);
	}
};

static void TestInput()
{
	TLineReader fInput("..\\surls");
	char* buffer;
	size_t urlLen;
	THashChecker hashChecker("..\\Indexer\\_bin");
	size_t iLine = 0;
	clock_t begin = clock();
	while (fInput.NextLine(&buffer, &urlLen))
	{
		++iLine;
		if ( (iLine % 100000) == 0 )
		{
			fprintf(stderr, "%d %d\n", static_cast<int>(iLine), static_cast<int>((CLOCKS_PER_SEC*iLine)/(clock() - begin)));
		}
		const ui64 hash = CheckerHash(buffer, urlLen);
		const bool has = hashChecker.Has(hash);
		if (!has)
		{
			printf("%d\n", static_cast<int>(has));
		}
	}
}

static void TestRandom()
{
	size_t countFP = 0;
	THashChecker hashChecker("c:\\Users\\denisra\\Programming\\FuzzyStringSet\\url_compressed.bin");
	static const size_t NTESTS = 1000000;
	clock_t begin = clock();
	size_t iLine = 0;
	for (size_t i = 0; i < NTESTS; ++i)
	{
		if ((i % 100000) == 0 && i)
		{
			printf("%d %d %f %d\n", i, countFP, static_cast<float>(countFP)*100.f/i, static_cast<int>((CLOCKS_PER_SEC*iLine)/(clock() - begin)));
		}
		++iLine;
		size_t len = 5 + (rand() % 20);
		char buffer[1024];
		for (size_t i = 0; i < len; ++i)
		{
			buffer[i] = (rand() % 250) + 1;
		}
		const ui64 hash = CheckerHash(buffer, len);
		if (hashChecker.Has(hash))
		{
			++countFP;
		}
	}
	printf("%d %f\n", countFP, static_cast<float>(countFP)*100.f/NTESTS);
}

int main(int argc, char* argv[])
{
	// TestInput();
	// TestRandom();
	// return 0;
	
	TTimer tCheck("check");

	if (argc < 4)
	{
		fprintf(stderr, "bad arguments\n");
		return 1;
	}

	const char* binIn = argv[1];
	const char* urlsIn = argv[2];
	const char* out = argv[3];

	TUrlReader reader(urlsIn);
	THashChecker checker(binIn);

	char* url;
	size_t len;
	TFileWriter fOut(out);

	while (reader.NextUrl(&url, &len))
	{
		/*
		for (size_t i = 0; i < len; ++i)
		{
			printf("%c", url[i]);
		}
		printf("\n");
		*/

		const ui64 hash = CheckerHash(url, len);
		const bool has = checker.Has(hash);
		fwrite(url, len, 1, fOut.m_file);
		if (has)
		{
			fwrite("\t1\r\n", 4, 1, fOut.m_file);
		}
		else
		{
			fwrite("\t0\r\n", 4, 1, fOut.m_file);
		}
	}

	return 0;
}