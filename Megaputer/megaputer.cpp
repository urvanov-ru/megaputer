// megaputer.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include "searcher.h"

void printError(errno_t err, LPCSTR message);

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 3)
	{
		_tprintf(_TEXT("Usage: megaputer.exe <input_file> <filter>\n"));
		return 1;
	}
	LPCTSTR inputFile = argv[1];
	LPCTSTR filter = argv[2];

	Searcher searcher;
	errno_t err = searcher.Open(inputFile);
	if (!err)
	{
		searcher.SetFilter(filter);
		LPCTSTR nextLine = _TEXT("");
		while (nextLine)
		{
			nextLine = searcher.GetNextLine();
			if (nextLine != NULL)
			{
				_tprintf(nextLine);
				_tprintf(_TEXT("\n"));
			}
		}
		err = searcher.Close();
		if (err)
		{
			printError(err, "Error closing input file: ");
		}
	}
	else
	{
		printError(err, "Can not open input file: ");		
	}
	return 0;
}


void printError(errno_t err, LPCSTR message)
{
	printf(message);
	CHAR buff[100];
	if (strerror_s(buff, 100, err) == 0)
	{
		printf(buff);
	}
	printf("\n");
}
