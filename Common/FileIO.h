#pragma once

#include <cstdio>

#include <string>
#include <exception>

class TLineReader
{
private:
	FILE* m_FIn;
	static const size_t BUFFER_LEN = 65536;
	char m_Buffer[BUFFER_LEN];

public:
	TLineReader(const std::string& filename)
	{
		m_FIn = fopen(filename.c_str(), "rb");
		if (!m_FIn)
		{
			throw std::exception("cannot open file");
		}
	}

	bool NextLine(char** result, size_t* len)
	{
		if (fgets(m_Buffer, BUFFER_LEN, m_FIn))
		{
			*len = strlen(m_Buffer);
			if (*len > BUFFER_LEN - 100)
			{
				fprintf(stderr, "Warning: long URL\n");
			}
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
	TUrlReader(const std::string& filename)
		: m_lineReader(filename)
	{

	}

	bool NextUrl(char** result, size_t* len)
	{
		if (m_lineReader.NextLine(result, len))
		{
			char* urlBegin = strchr(*result, '\t');
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

class TFileWriter
{
public:
	FILE* m_file;

	TFileWriter(const char* filename)
	{
		m_file = fopen(filename, "wb");
		if (!m_file)
		{
			throw std::exception("cannot open file for write");
		}
	}

	~TFileWriter()
	{
		fclose(m_file);
	}
};