#include <cstdio>
#include <cstdlib>

#include "..\Common\FileIO.h"
#include "..\Common\Types.h"
#include "..\Common\SetParameters.h"
#include "..\Common\CityHash.h"

#include "windows.h"

class THashCheker
{
private:
	HANDLE m_File;
	HANDLE m_Map;
	ui32 m_Size;
	LPVOID m_View;
	const TBucket* m_Buckets;
	const ui16* m_Data;

public:
	THashCheker(const std::wstring& filename)
	{
		m_File = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
		m_Size = GetFileSize(m_File, 0);
		m_Map = CreateFileMapping(m_File, NULL, PAGE_READONLY, 0, m_Size, NULL);
		m_View = MapViewOfFile(m_Map, FILE_MAP_READ, 0, 0, m_Size);
		m_Buckets = static_cast<const TBucket*>(m_View);
		m_Data = reinterpret_cast<const ui16*>(m_Buckets + NBUCKETS + 1);
	}
		
	bool Has(ui64 hash)
	{
		const size_t iBucket = (hash*NBUCKETS)/NHASHES;
		const size_t stop = m_Buckets[iBucket + 1].m_Offset;
		const ui64 hashStart = (iBucket*NHASHES)/NBUCKETS;
		ui64 now = hashStart;
		size_t i = m_Buckets[iBucket].m_Offset;
		do 
		{
			now += m_Data[i];
			++i;
		}
		while ( (i < stop) && (now < hash) );
		return now == hash;
	}
	
	~THashCheker()
	{
		UnmapViewOfFile(m_View);
		CloseHandle(m_Map);
		CloseHandle(m_File);
	}
};

void TestInput()
{
	TLineReader fInput("..\\surls");
	char* buffer;
	size_t urlLen;
	THashCheker hashChecker(L"..\\Indexer\\_bin");
	size_t iLine = 0;
	while (fInput.NextLine(&buffer, &urlLen))
	{
		++iLine;
		if ( (iLine % 100000) == 0 )
		{
			fprintf(stderr, "%d\n", static_cast<int>(iLine));
		}
		const ui64 hash = CityHash64(buffer, urlLen) & MASK;
		printf("%d\n", static_cast<int>(hashChecker.Has(hash)));
	}
}

void TestRandom()
{
	size_t countFP = 0;
	THashCheker hashChecker(L"..\\Indexer\\_bin");
	static const size_t NTESTS = 1000000;
	for (size_t i = 0; i < NTESTS; ++i)
	{
		if ((i % 100000) == 0 && i)
		{
			printf("%d %d %f\n", i, countFP, static_cast<float>(countFP)*100.f/i);
		}
		size_t len = 5 + (rand() % 20);
		char buffer[1024];
		for (size_t i = 0; i < len; ++i)
		{
			buffer[i] = (rand() % 250) + 1;
		}
		const ui64 hash = CityHash64(buffer, len) & MASK;
		if (hashChecker.Has(hash))
		{
			++countFP;
		}
	}
	printf("%d %f\n", countFP, static_cast<float>(countFP)*100.f/NTESTS);
}

int main()
{
	TestRandom();
	TestInput();
	return 0;
}