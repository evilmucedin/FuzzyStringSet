#include <cstdio>

#include <vector>
#include <string>
#include <algorithm>

#include "..\Common\Types.h"
#include "..\Common\CityHash.h"
#include "..\Common\Timer.h"
#include "..\Common\SetParameters.h"
#include "..\Common\SuperFastHash.h"

using namespace std;

typedef vector<string> TStringVector;
typedef vector<ui64> THashes;

// static const bool TEST_MODE = true;
static const bool TEST_MODE = false;

static const size_t N = 101000000;

int main()
{
	THashes hashes;
	hashes.reserve(N);
	{
		TTimer tRead("Read");
		FILE* fIn = fopen("..\\surls", "r");
		static const size_t BUFFER_LEN = 65536;
		char buffer[BUFFER_LEN];
		int iLine = 0;
		ui64 prevHash = 0;
		while (fgets(buffer, BUFFER_LEN, fIn))
		{
			if (TEST_MODE && (iLine >= 1000000))
			{
				break;
			}
			++iLine;
			if ((iLine % 100000) == 0)
			{
				printf("%d\n", iLine);
			}
			size_t urlLen = strlen(buffer);
			if (urlLen > 32000)
			{
				fprintf(stderr, "Warning: long URL\n");
			}
			if (urlLen)
			{
				while (urlLen && (buffer[urlLen - 1] == '\n'))
				{
					--urlLen;
				}
				if (urlLen)
				{
					const ui64 hash = CityHash64(buffer, urlLen) & MASK;
					hashes.push_back(hash);
					if (prevHash == hash)
					{
						fprintf(stderr, "Warning: same URL '%llu' '%llu' '%s'\n", prevHash, hash, buffer);			
					}
					prevHash = hash;
				}
			}
		}
		printf("%d lines handled\n", iLine);
	}

	{
		TTimer tSort("Sort");
		std::sort(hashes.begin(), hashes.end());
	}

	{
		TTimer tStat("Stat");
		size_t bigDiffs = 0;
		size_t veryBigDiffs = 0;
		size_t collisions = 0;
		size_t sizeEstimation = 0;
		for (size_t i = 1; i < hashes.size(); ++i)
		{
			const ui64 diff = hashes[i] - hashes[i - 1];
			if (diff >= 32768)
			{
				++veryBigDiffs;
			}
			else if (diff >= 128)
			{
				++bigDiffs;
			}
			else if (diff == 0)
			{
				++collisions;
			}

			if (diff >= 128)
			{
				sizeEstimation += 2;
			}
			else
			{
				sizeEstimation += 1;
			}
		}

		printf("Collisions: %d\n", static_cast<int>(collisions));
		printf("Collisions ratio: %f\n", static_cast<float>(collisions)/hashes.size());
		printf("Very big diffs: %d\n", static_cast<int>(veryBigDiffs));
		printf("Big diffs: %d\n", static_cast<int>(bigDiffs));
		printf("Avg. diff: %f\n", static_cast<float>(MASK)/hashes.size());
		printf("Diffs: %d\n", static_cast<int>(hashes.size()));
		printf("Size: %d\n", static_cast<int>(sizeEstimation/1024/1024));
	}

	return 0;
}