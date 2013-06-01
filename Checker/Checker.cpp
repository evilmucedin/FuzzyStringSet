#include <cstdio>
#include <cstdlib>

#include "..\Common\FileIO.h"
#include "..\Common\Types.h"
#include "..\Common\SetParameters.h"
#ifndef NDEBUG
#	include "..\Common\Timer.h"
#endif

#include <windows.h>

class THashChecker
{
private:
	HANDLE m_File;
	HANDLE m_Map;
	ui32 m_Size;
	LPVOID m_View;
	const TBucket* m_Buckets;
	const ui16* m_Data; 

public:
	THashChecker(const char* filename)
	{
		m_File = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
#ifndef NDEBUG		
		if (INVALID_HANDLE_VALUE == m_File)
		{
			throw std::exception("Bad CreateFile");
		}
#endif
		m_Size = GetFileSize(m_File, 0);
		m_Map = CreateFileMapping(m_File, NULL, PAGE_READONLY, 0, m_Size, NULL);
#ifndef NDEBUG		
		if (!m_Map)
		{
			throw std::exception("Bad CreateFileMapping");
		}
#endif
		m_View = MapViewOfFile(m_Map, FILE_MAP_READ, 0, 0, m_Size);
#ifndef NDEBUG		
		if (!m_View)
		{
			throw std::exception("Bad MapViewOfFile");
		}
#endif
		m_Buckets = static_cast<const TBucket*>(m_View);
		m_Data = reinterpret_cast<const ui16*>((const char*)m_View + (NBUCKETS + 1)*sizeof(TBucket));
	}
		
	bool Has(ui64 hash) throw()
	{
		const size_t iBucket = static_cast<size_t>((hash*NBUCKETS)/NHASHES);
		
		const ui16* data = m_Data + m_Buckets[iBucket].m_Offset;
		const ui16* stop = m_Data + m_Buckets[iBucket + 1].m_Offset;

		/*
		WIN32_MEMORY_RANGE_ENTRY entry;
		entry.VirtualAddress = (PVOID)data;
		entry.NumberOfBytes = (m_Buckets[iBucket + 1].m_Offset - m_Buckets[iBucket].m_Offset)*2;
		PrefetchVirtualMemory(GetCurrentProcess(), 1, &entry, 0);
		*/

		const ui64 hashStart = (iBucket*NHASHES)/NBUCKETS;
		ui64 now = hashStart;
		do 
		{
			now += *data;
			++data;
		}
		while ( (data != stop) && (now < hash) );

		return now == hash;
	}
	
	~THashChecker()
	{
		UnmapViewOfFile(m_View);
		CloseHandle(m_Map);
		CloseHandle(m_File);
	}
};

#ifndef NDEBUG
#include <ctime>

static void TestInput()
{
	TLineReader fInput("..\\surls");
	const char* buffer;
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
#ifndef NDEBUG
		if ((i % 100000) == 0 && i)
		{
			printf("%d %d %f %d\n", i, countFP, static_cast<float>(countFP)*100.f/i, static_cast<int>((CLOCKS_PER_SEC*iLine)/(clock() - begin)));
		}
#endif
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
#ifndef NDEBUG
	printf("%d %f\n", countFP, static_cast<float>(countFP)*100.f/NTESTS);
#endif
}

#endif

int main(int argc, char* argv[])
{
	// TestInput();
	// TestRandom();
	// return 0;
	
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	// SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
	
#ifndef NDEBUG
	TTimer tCheck("check");

	if (argc < 4)
	{
		fprintf(stderr, "bad arguments\n");
		return 1;
	}
#endif

	const char* binIn = argv[1];
	const char* urlsIn = argv[2];
	const char* out = argv[3];

	TFastUrlReader reader(urlsIn);
	THashChecker checker(binIn);

	const char* url;
	size_t len;
	TFastFileWriter fOut(out);

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
		fOut.Write(url, len);
		if (has)
		{
			fOut.Write("\t1\r\n", 4);
		}
		else
		{
			fOut.Write("\t0\r\n", 4);
		}
	}

	return 0;
}

#define NULCHAR    '\0'
#define SPACECHAR  ' '
#define TABCHAR    '\t'
#define DQUOTECHAR '\"'
#define SLASHCHAR  '\\'

static void __cdecl ParseCmdline(
    char* cmdstart,
    char** argv,
    char* args,
    int *numargs,
    int *numchars
    )
{
        char *p;
        char c;
        int inquote;                    /* 1 = inside quotes */
        int copychar;                   /* 1 = copy char to *args */
        unsigned numslash;              /* num of backslashes seen */

        *numchars = 0;
        *numargs = 1;                   /* the program name at least */

        /* first scan the program name, copy it, and count the bytes */
        p = cmdstart;
        if (argv)
            *argv++ = args;

#ifdef WILDCARD
        /* To handle later wild card expansion, we prefix each entry by
        it's first character before quote handling.  This is done
        so _[w]cwild() knows whether to expand an entry or not. */
        if (args)
            *args++ = *p;
        ++*numchars;

#endif  /* WILDCARD */

        /* A quoted program name is handled here. The handling is much
           simpler than for other arguments. Basically, whatever lies
           between the leading double-quote and next one, or a terminal null
           character is simply accepted. Fancier handling is not required
           because the program name must be a legal NTFS/HPFS file name.
           Note that the double-quote characters are not copied, nor do they
           contribute to numchars. */
        inquote = FALSE;
        do {
            if (*p == DQUOTECHAR )
            {
                inquote = !inquote;
                c = (char) *p++;
                continue;
            }
            ++*numchars;
            if (args)
                *args++ = *p;

            c = (char) *p++;
#ifdef _MBCS
            if (_ismbblead(c)) {
                ++*numchars;
                if (args)
                    *args++ = *p;   /* copy 2nd byte too */
                p++;  /* skip over trail byte */
            }
#endif  /* _MBCS */

        } while ( (c != NULCHAR && (inquote || (c !=SPACECHAR && c != TABCHAR))) );

        if ( c == NULCHAR ) {
            p--;
        } else {
            if (args)
                *(args-1) = NULCHAR;
        }

        inquote = 0;

        /* loop on each argument */
        for(;;) {

            if ( *p ) {
                while (*p == SPACECHAR || *p == TABCHAR)
                    ++p;
            }

            if (*p == NULCHAR)
                break;              /* end of args */

            /* scan an argument */
            if (argv)
                *argv++ = args;     /* store ptr to arg */
            ++*numargs;

#ifdef WILDCARD
        /* To handle later wild card expansion, we prefix each entry by
        it's first character before quote handling.  This is done
        so _[w]cwild() knows whether to expand an entry or not. */
        if (args)
            *args++ = *p;
        ++*numchars;

#endif  /* WILDCARD */

        /* loop through scanning one argument */
        for (;;) {
            copychar = 1;
            /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
               2N+1 backslashes + " ==> N backslashes + literal "
               N backslashes ==> N backslashes */
            numslash = 0;
            while (*p == SLASHCHAR) {
                /* count number of backslashes for use below */
                ++p;
                ++numslash;
            }
            if (*p == DQUOTECHAR) {
                /* if 2N backslashes before, start/end quote, otherwise
                    copy literally */
                if (numslash % 2 == 0) {
                    if (inquote && p[1] == DQUOTECHAR) {
                        p++;    /* Double quote inside quoted string */
                    } else {    /* skip first quote char and copy second */
                        copychar = 0;       /* don't copy quote */
                        inquote = !inquote;
                    }
                }
                numslash /= 2;          /* divide numslash by two */
            }

            /* copy slashes */
            while (numslash--) {
                if (args)
                    *args++ = SLASHCHAR;
                ++*numchars;
            }

            /* if at end of arg, break loop */
            if (*p == NULCHAR || (!inquote && (*p == SPACECHAR || *p == TABCHAR)))
                break;

            /* copy character into argument */
#ifdef _MBCS
            if (copychar) {
                if (args) {
                    if (_ismbblead(*p)) {
                        *args++ = *p++;
                        ++*numchars;
                    }
                    *args++ = *p;
                } else {
                    if (_ismbblead(*p)) {
                        ++p;
                        ++*numchars;
                    }
                }
                ++*numchars;
            }
            ++p;
#else  /* _MBCS */
            if (copychar) {
                if (args)
                    *args++ = *p;
                ++*numchars;
            }
            ++p;
#endif  /* _MBCS */
            }

            /* null-terminate the argument */

            if (args)
                *args++ = NULCHAR;          /* terminate string */
            ++*numchars;
        }

        /* We put one last argument in -- a null ptr */
        if (argv)
            *argv++ = NULL;
        ++*numargs;
}

int CustomEntryPoint()
{
	char* lpszCommandLine = GetCommandLineA();
	int argc;
	int dummy;
	char buffer1[10000];
	char buffer2[10000];
	char** argv = (char**)buffer1;
	char* args = (char*)buffer2;
	ParseCmdline(lpszCommandLine, argv, args, &argc, &dummy);
	return main(argc, argv);
}
