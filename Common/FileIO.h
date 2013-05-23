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
			while (*len && (m_Buffer[*len - 1] == '\n'))
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