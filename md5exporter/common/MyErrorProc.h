#ifndef __MyErrorProc_h__
#define __MyErrorProc_h__
#include <IGame/IGameError.h>

class MyErrorProc : public IGameErrorCallBack
{
public:
	void ErrorProc(IGameError error)
	{
		TCHAR * buf = GetLastIGameErrorText();
		DebugPrint(_T("ErrorCode = %d ErrorText = %s\n"), error,buf);
	}
};

#endif // __MyErrorProc_h__
