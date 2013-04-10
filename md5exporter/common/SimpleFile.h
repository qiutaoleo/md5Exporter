#ifndef __SimpleFile_h__
#define __SimpleFile_h__
#include <Windows.h>
#include <stdio.h>
#include <tchar.h>

class SimpleFile
{
public:
	SimpleFile(const TCHAR *filename,const TCHAR *mode)
	{
		_file = NULL; Open(filename, mode);
	}

	~SimpleFile()
	{ 
		Close();
	}

	FILE* File()
	{
		return _file;
	}

	int Close() 
	{ 
		int result=0; 
		if(_file) 
			result=fclose(_file); 
		_file = NULL; 
		return result; 
	}

	void Open(const TCHAR *filename,const TCHAR *mode) 
	{ 
		Close(); 
		_file= _tfopen(filename,mode); 
	}
protected:
	FILE* _file;
};
#endif // __SimpleFile_h__
