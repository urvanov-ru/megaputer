#include "stdafx.h"
#include "searcher.h"


const size_t MAX_BUFFER_SIZE = 2000000;

Searcher::Searcher()
{
	bufferSize = 100;
	buffer = new _TCHAR[bufferSize];
	filter = NULL;
	words = NULL;
	wordsSize = NULL;
}

Searcher::~Searcher()
{
	if (buffer != NULL)
	{
		delete [] buffer;
		buffer = NULL;
	}
	if (filter != NULL)
	{
		delete [] filter;
		filter = NULL;
	}
	if (words != NULL)
	{
		for (size_t n = 0; n < wordsCount; n++)
		{
			delete [] words[n];
		}
		delete [] words;
		words = NULL;
	}
	if (wordsSize != NULL)
	{
		delete [] wordsSize;
		wordsSize = NULL;
	}
}

errno_t Searcher::Open(LPCTSTR fileName)
{
	return (_tfopen_s(&file, fileName, _TEXT("r")));
}

errno_t Searcher::Close()
{
	return fclose(file);
}

errno_t Searcher::SetFilter(LPCTSTR searchFilter)
{
	// Очистка ранее заполненных данных.
	if (filter != NULL)
	{
		delete [] filter;
		filter = NULL;
	}
	if (words != NULL)
	{
		for (size_t n = 0; n < wordsCount; n++)
		{
			delete [] words[n];
		}
		delete [] words;
		words = NULL;
	}
	if (wordsSize != NULL)
	{
		delete [] wordsSize;
		wordsSize = NULL;
	}

	if (searchFilter == NULL)
	{
		filter = NULL;
		return 0;
	}
	filterLength = _tcslen(searchFilter);
	if (filterLength == 0)
	{
		filter = NULL;
		return 0;
	}
	
	filter = new _TCHAR[filterLength + 1];

	
	

	// Приводим пришедший фильтр к нормальному виду.
	size_t srcPos;
	size_t dstPos = 1;
	filter[0] = searchFilter[0];
	for (srcPos = 1; srcPos < filterLength; srcPos++)
	{
		_TCHAR lastChar = filter[dstPos - 1];
		if (searchFilter[srcPos] == '*')
		{
			if (lastChar == '*')
			{
				// Подряд идущие звёздочки бессмысленны.
				// Оставляем только одну.
			}
			else
			{
				filter[dstPos] = searchFilter[srcPos];
				dstPos++;
			}
		}
		else if (searchFilter[srcPos] == '?')
		{
			if (lastChar == '*')
			{
				// Упорядочиваем фильтр так, чтобы
				// символы ? всегда шли перед звёздочкой.
				filter[dstPos - 1] = '?';
				filter[dstPos] = '*';
				dstPos++;
			}
			else
			{
				filter[dstPos] = searchFilter[srcPos];
				dstPos++;
			}
		}
		else
		{
			filter[dstPos] = searchFilter[srcPos];
			dstPos++;
		}
	}
	filter[dstPos] = 0;
	filterLength = dstPos;
	
	// Разбираем фразы.
	// Нужно для ускорения поиска.
	// Запоминаем фразы, идущие после * до любого из символов ? или *.
	wordsCount = 0;
	bool isWord = false;
	for (size_t n = 0; n < dstPos - 1; n++)
	{
		if (filter[n] == '*')
		{
			wordsCount++;
		}
	}
	
	wordsSize = new size_t[wordsCount];
	words = new LPTSTR[wordsCount];
	size_t wordIndex = 0;
	size_t wordBegin = 0;
	isWord = false;

	for (size_t n = 0; n < dstPos; n++)
	{
		if (isWord)
		{
			if ((filter[n] == '*') || (filter[n] == '?')
				||(n + 1 == dstPos))
			{
				size_t length = n - wordBegin;
				wordsSize[wordIndex] = length;
				words[wordIndex] = new _TCHAR[length + 1];
				for (size_t m = 0; m < length; m++)
				{
					words[wordIndex][m] = filter[wordBegin + m];
				}
				words[wordIndex][length] = 0;
				wordIndex++;
				isWord = false;
			}
		}
		if (!isWord)
		{
			if (n + 1 != dstPos)
			{
				if (filter[n] == '*')
				{
					wordBegin = n + 1;
					isWord = true;
				}
			}
		}
	}

	return 0;
}


LPCTSTR Searcher::ReadLine()
{
	if (feof(file))
	{
		return NULL;
	}
	bool lineReaded = false;
	buffer[0] = 0;
	size_t posInBuffer = 0;
	while (!lineReaded)
	{
		if (_fgetts(buffer + posInBuffer, bufferSize - posInBuffer,
			file) == NULL)
		{
			if (feof(file))
			{
				lineReaded = true;
			}
			else if (ferror(file))
			{
				return NULL;
			}
		}
		int length = _tcslen(buffer);
		if (buffer[length - 1] == '\n')
		{
			// Строка успешно считана. Убираем считанный символ
			// перевода строки.
			buffer[length - 1] = 0;
			lineReaded = true;
		}
		else if (bufferSize >= MAX_BUFFER_SIZE)
		{
			// Вся строка не поместилась, но буфер итак
			// уже имеет максимальную длину.
			lineReaded = true;
		}
		else
		{
			// Вся строка не поместилась. Увеличиваем буфер
			// и дочитываем недостающие символы.
			posInBuffer = bufferSize - 1;
			size_t newBufferSize = bufferSize * 2;
			if (newBufferSize > MAX_BUFFER_SIZE)
			{
				newBufferSize = MAX_BUFFER_SIZE;
			}
			_TCHAR* newBuffer = new _TCHAR[newBufferSize];
			if (_tcscpy_s(newBuffer, newBufferSize, buffer))
			{
				delete [] newBuffer;
				return NULL;
			}
			delete[] buffer;
			buffer = newBuffer;
			bufferSize = newBufferSize;
		}
	}
	
	return buffer;
}

bool Searcher::IsMatch()
{
	size_t p = 0;
	size_t l = filterLength;
	size_t bl = _tcslen(buffer);
	size_t tb = 0;
	size_t te = 0;
	size_t tl = 0;
	if ((filter[0] != '*') && (filter[0] != '?')
		&& (filter[0] != buffer[0]))
	{
		return false;
	}

	size_t t = 0;

	size_t wordIndex = 0;

	while (t < l)
	{
		if (filter[t] == '?')
		{
			p++;
			t++;
		}
		else if (filter[t] == '*')
		{
			if (t + 1 == l)
			{
				return true;
			}
			else
			{
				size_t n = t + 1;
				tb = n;
				te = tb + wordsSize[wordIndex];
				tl = wordsSize[wordIndex];
				// Ищем следующую фразу, с которой нужно сравнивать.
				_TCHAR* w = _tcsstr(buffer + p, words[wordIndex]);
				if (w == NULL)
				{
					return false;
				}
				else
				{
					p = (w - buffer + tl);
					t += tl + 1;
				}
				wordIndex++;
			}
		}
		else if (filter[t] == buffer[p])
		{
			p++;
			t++;
		}
		else
		{
			return false;
		}
		if (p > bl)
		{
			return false;
		}
	}
	if (p == bl)
	{
		return true;
	}
	else
	{
		return false;
	}
}

LPCTSTR Searcher::GetNextLine()
{
	if (feof(file))
	{
		return NULL;
	}
	if (filter == NULL)
	{
		return NULL;
	}
	
	LPCTSTR line = ReadLine();
	while (line)
	{
		if (IsMatch())
		{
			return line;
		}
		line = ReadLine();
	}
	return NULL;
}