#include <cstdio>

#include <vector>
#include <string>
#include <algorithm>

#include "..\Common\Types.h"
#include "..\Common\CityHash.h"
#include "..\Common\Timer.h"
#include "..\Common\SetParameters.h"
#include "..\Common\SuperFastHash.h"
#include "..\Common\FileIO.h"

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
		TLineReader fIn("..\\surls");
		int iLine = 0;
		ui64 prevHash = 0;
		char* buffer;
		size_t urlLen;
		while (fIn.NextLine(&buffer, &urlLen))
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
			const ui64 hash = CityHash64(buffer, urlLen) & MASK;
			hashes.push_back(hash);
			if (prevHash == hash)
			{
				fprintf(stderr, "Warning: same URL '%llu' '%llu' '%s'\n", prevHash, hash, buffer);			
			}
			prevHash = hash;
		}
		printf("%d lines handled\n", iLine);
	}

	{
		TTimer tSort("Sort");
		std::sort(hashes.begin(), hashes.end());
	}

	{
		TTimer tStat("Stat");
		size_t diffs8 = 0;
		size_t diffs16 = 0;
		size_t collisions = 0;
		size_t sizeEstimation = 0;
		size_t sizeEstimation2 = 0;
		size_t sizeEstimation3 = 0;
		for (size_t i = 1; i < hashes.size(); ++i)
		{
			const ui64 diff = hashes[i] - hashes[i - 1];
			if (diff >= (1 << 16))
			{
                ++diffs16;
			}
			else if (diff >= 256)
			{
                ++diffs8;
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

            if (diff >= 254)
			{
				sizeEstimation2 += 3;
			}
			else
			{
				sizeEstimation2 += 1;
			}
			
            sizeEstimation3 += 2;
		}

		printf("Collisions: %d\n", static_cast<int>(collisions));
		printf("Collisions ratio %%: %f\n", 100.f*static_cast<float>(collisions)/hashes.size());
        printf("Collisions + Diffs16 ratio %%: %f\n", 100.f*static_cast<float>(collisions + diffs16)/hashes.size());
		printf("Diffs8: %d\n", static_cast<int>(diffs8));
		printf("Diffs16: %d\n", static_cast<int>(diffs16));
		printf("Avg. diff: %f\n", static_cast<float>(MASK)/hashes.size());
		printf("Diffs: %d\n", static_cast<int>(hashes.size()));
		printf("Size: %d\n", static_cast<int>(sizeEstimation/1024/1024));
		printf("Size2: %d\n", static_cast<int>(sizeEstimation2/1024/1024));
		printf("Size3: %d\n", static_cast<int>(sizeEstimation3/1024/1024));
	}

	{
		TTimer tStat("Write");
		FILE* fOut = fopen("bin", "wb");
		
		TBucket buckets[NBUCKETS + 1];
		memset(buckets, 0, sizeof(buckets));
		fwrite(buckets, sizeof(TBucket), NBUCKETS + 1, fOut);
		ui32 offset = 0;

		size_t iHash = 0;
		size_t minLen = 1000000;
		size_t maxLen = 0;
		for (ui64 i = 0; i < NBUCKETS; ++i)
		{
			if (0 == (i % 100))
			{
				printf("%d\n", i);
			}
			buckets[i].m_Offset = offset;

			const ui64 bucketBegin = (i*NHASHES) / NBUCKETS;
			const ui64 bucketEnd = ((i + 1)*NHASHES) / NBUCKETS;
			ui64 prev = bucketBegin;
			size_t len = 0;
			while ( (iHash < hashes.size()) && (hashes[iHash] < bucketEnd) )
			{
				ui32 diff = hashes[iHash] - prev;
				while (diff > 0)
				{
					const ui32 toWrite = std::min(static_cast<ui32>(diff), static_cast<ui32>((1 << 16) - 1));
					const ui16 toWrite16 = static_cast<ui16>(toWrite);
					fwrite(&toWrite16, 2, 1, fOut);
					diff -= toWrite;
					++offset;
					++len;
				}
				prev = hashes[iHash];
				++iHash;
			}
			minLen = min(minLen, len);
			maxLen = max(maxLen, len);
		}
		buckets[NBUCKETS].m_Offset = offset;

		fseek(fOut, 0, SEEK_SET);
		fwrite(buckets, sizeof(TBucket), NBUCKETS + 1, fOut);

		fclose(fOut);

		printf("minLen = %d\n", minLen);
		printf("maxLen = %d\n", maxLen);
	}

	return 0;
}