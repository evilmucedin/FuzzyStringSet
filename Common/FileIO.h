#pragma once

#include <cstdio>

#include <string.h>
#include <windows.h>

#ifndef NDEBUG		
#	include <exception>
#endif

#include "Types.h"

class TLineReader
{
private:
	FILE* m_FIn;
	static const size_t BUFFER_LEN = 65536;
	char m_Buffer[BUFFER_LEN];

	static char* FGetS(char* string, FILE* fIn, size_t* len)
	{
		char* pointer = string;
		char ch;

		while ((ch = _fgetc_nolock(fIn)) != EOF)
		{
			if ((*pointer++ = ch) == '\n')
			{
				break;
			}
		}
				
		*len = pointer - string;
		if (*len != 0) 
		{
			return string;
		}
		else
		{
			return 0;
		}
	}

public:
	TLineReader(const char* filename)
	{
		m_FIn = fopen(filename, "rb");
#ifndef NDEBUG		
		if (!m_FIn)
		{
			throw std::exception("cannot open file");
		}
#endif
		setvbuf(m_FIn, 0, _IOFBF, 16*1024);
	}

	bool NextLine(const char** result, size_t* len)
	{
		if (FGetS(m_Buffer, m_FIn, len))
		{
#ifndef NDEBUG		
			if (*len > BUFFER_LEN - 100)
			{
				fprintf(stderr, "Warning: long URL\n");
			}
#endif
			while (*len && (m_Buffer[*len - 1] == '\n' || m_Buffer[*len - 1] == '\r'))
			{
				--(*len);
			}
			if (*len)
			{
				*result = m_Buffer;
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	
	~TLineReader()
	{
		fclose(m_FIn);
	}
};

class TUrlReader
{
private:
	TLineReader m_lineReader;

public:
	TUrlReader(const char* filename)
		: m_lineReader(filename)
	{

	}

	bool NextUrl(const char** result, size_t* len)
	{
		if (m_lineReader.NextLine(result, len))
		{
			char* urlBegin = strchr((char*)*result, '\t');
			*len -= urlBegin - *result + 1;
			*result = urlBegin + 1;
			return true;
		}
		else
		{
			return false;
		}
	}
};

const char* MemChr(const char* buf, const char* end, char chr)
{
	while ( (buf != end) && (*buf != chr) ) 
	{
		++buf;
    }
	if (buf != end)
	{
		return buf;
	}
	else
	{
		return 0;
	}
}

class TFastUrlReader
{
private:
	HANDLE m_File;
	HANDLE m_Map;
	ui32 m_Size;
	const char* m_View;
	const char* m_End;
	const char* m_Now;

public:
	TFastUrlReader(const char* filename)
	{
		m_File = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
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
		m_View = reinterpret_cast<const char*>(MapViewOfFile(m_Map, FILE_MAP_READ, 0, 0, m_Size));
#ifndef NDEBUG		
		if (!m_View)
		{
			throw std::exception("Bad MapViewOfFile");
		}
#endif
		m_Now = m_View;
		m_End = m_View + m_Size;
	}

	bool NextUrl(const char** result, size_t* len)
	{
		if (m_Now < m_End)
		{
			const char* urlBegin = static_cast<const char*>(MemChr(m_Now, m_End, '\t'));
			m_Now = urlBegin + 1;
			*result = m_Now;
			const char* urlEnd = static_cast<const char*>(MemChr(m_Now, m_End, '\r'));
			if (urlEnd)
			{
				*len = urlEnd - urlBegin - 1;
				m_Now = urlEnd + 1;
				if (*m_Now == '\n')
				{
					++m_Now;
				}
			}
			else
			{
				*len = m_End - urlBegin - 1;
				m_Now = m_End;
			}
			return true;
		}
		else
		{
			return false;
		}
	}

	~TFastUrlReader()
	{
		UnmapViewOfFile(m_View);
		CloseHandle(m_Map);
		CloseHandle(m_File);
	}
};

class TFileWriter
{
public:
	FILE* m_file;

	TFileWriter(const char* filename)
	{
		m_file = fopen(filename, "wb");
#ifndef NDEBUG		
		if (!m_file)
		{
			throw std::exception("cannot open file for write");
		}
#endif
	}

	~TFileWriter()
	{
		fclose(m_file);
	}
};

class TFastFileWriter
{
private:
	HANDLE m_File;
	static const size_t BUFFER_SIZE = 1 << 16;
	char m_Buffer[BUFFER_SIZE];
	size_t m_BufferLen;

	void Flush()
	{
		WriteFile(m_File, m_Buffer, m_BufferLen, 0, NULL);
		m_BufferLen = 0;
	}

public:
	TFastFileWriter(const char* filename)
	{
		m_File = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#ifndef NDEBUG		
		if (INVALID_HANDLE_VALUE == m_File)
		{
			throw std::exception("Bad CreateFile");
		}
#endif
		m_BufferLen = 0;
	}

	void Write(const char* data, size_t len)
	{
		memcpy(m_Buffer + m_BufferLen, data, len);
		m_BufferLen += len;
		if (m_BufferLen >= BUFFER_SIZE/2)
		{
			Flush();
		}
	}

	~TFastFileWriter()
	{
		Flush();
		CloseHandle(m_File);
	}
};