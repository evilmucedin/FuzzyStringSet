#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "..\Common\Timer.h"
#include "..\Common\Types.h"

int main() 
{
	static const ui64 SIZE = 1ULL << 32;
	static const char FILENAME[] = "test.bin"; 
	if (false)
	{
		TTimer t("Random fill");
		FILE* fTest = fopen(FILENAME, "wb");
		ui64 toWrite = 0;
		static const ui64 size64Mb = (1ULL << 26) - 1;
		while (toWrite < SIZE)
		{
			if ( (toWrite & size64Mb) == 0 )
			{
				printf("%lld\n", toWrite/1024/1024);
			}
			static const size_t BUFFER_SIZE = 1 << 16;
			char buffer[BUFFER_SIZE];
			for (size_t i = 0; i < BUFFER_SIZE; i += 2)
			{
				int rnd = rand();
				memcpy(buffer + i, &rnd, 2);
			}
			fwrite(buffer, BUFFER_SIZE, 1, fTest);
			toWrite += BUFFER_SIZE;
		}
		fclose(fTest);
	}
	
	{
		FILE* fTest = fopen(FILENAME, "rb");
		for (size_t blockSize = 0; blockSize < 20; ++blockSize)
		{
			const clock_t begin = clock();
			const size_t size = 1 << blockSize; 
			char* const buffer = new char[size];
			for (size_t i = 0; i < 100000; ++i)
			{
				const ui64 rnd = ((ui64)rand() << 32) + ((ui64)rand() << 16) + (ui64)rand(); 
				fseek(fTest, rnd, SEEK_SET);
				fread(buffer, size, 1, fTest);
			}
			printf("%d %d\n", size, static_cast<int>(clock() - begin));
			delete[] buffer;
		}
		fclose(fTest);
	}
	
	return 0;
}