//**************************************************************************/
// Copyright (c) 1998-2007 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Appwizard generated plugin
// AUTHOR: 
//***************************************************************************/

#include "debug3dmax.h"

#define debug3dmax_CLASS_ID	Class_ID(0x668a6bf5, 0xaf1d3a3b)

class debug3dmax : public SceneExport {
	public:
		
		static HWND hParams;
		
		int				ExtCount();					// Number of extensions supported
		const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
		const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
		const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
		const TCHAR *	AuthorName();				// ASCII Author name
		const TCHAR *	CopyrightMessage();			// ASCII Copyright message
		const TCHAR *	OtherMessage1();			// Other message #1
		const TCHAR *	OtherMessage2();			// Other message #2
		unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
		void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box

		BOOL SupportsOptions(int ext, DWORD options);
		int				DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0);

		//Constructor/Destructor
		debug3dmax();
		~debug3dmax();		

};



class debug3dmaxClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 		{ return new debug3dmax(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return SCENE_EXPORT_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return debug3dmax_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("debug3dmax"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* Getdebug3dmaxDesc() { 
	static debug3dmaxClassDesc debug3dmaxDesc;
	return &debug3dmaxDesc; 
}





INT_PTR CALLBACK debug3dmaxOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	static debug3dmax *imp = NULL;

	switch(message) {
		case WM_INITDIALOG:
			imp = (debug3dmax *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return 1;
	}
	return 0;
}


//--- debug3dmax -------------------------------------------------------
debug3dmax::debug3dmax()
{

}

debug3dmax::~debug3dmax() 
{

}

int debug3dmax::ExtCount()
{
	#pragma message(TODO("Returns the number of file name extensions supported by the plug-in."))
	return 1;
}

const TCHAR *debug3dmax::Ext(int n)
{		
	#pragma message(TODO("Return the 'i-th' file name extension (i.e. \"3DS\")."))
	return _T("debug");
}

const TCHAR *debug3dmax::LongDesc()
{
	#pragma message(TODO("Return long ASCII description (i.e. \"Targa 2.0 Image File\")"))
	return _T("Debug exporter plugin");
}
	
const TCHAR *debug3dmax::ShortDesc() 
{			
	#pragma message(TODO("Return short ASCII description (i.e. \"Targa\")"))
	return _T("Debug exporter");
}

const TCHAR *debug3dmax::AuthorName()
{			
	#pragma message(TODO("Return ASCII Author name"))
	return _T("HoneyCat");
}

const TCHAR *debug3dmax::CopyrightMessage() 
{	
	#pragma message(TODO("Return ASCII Copyright message"))
	return _T("Copyright (C) 2013 HoneyCat. All rights reserved.");
}

const TCHAR *debug3dmax::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *debug3dmax::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int debug3dmax::Version()
{				
	#pragma message(TODO("Return Version number * 100 (i.e. v3.01 = 301)"))
	return 100;
}

void debug3dmax::ShowAbout(HWND hWnd)
{			
	// Optional
}

BOOL debug3dmax::SupportsOptions(int ext, DWORD options)
{
	#pragma message(TODO("Decide which options to support.  Simply return true for each option supported by each Extension the exporter supports."))
	return TRUE;
}

// This function returns the number of plug-in classes this DLL
typedef int (*LibNumberClassesFun)();

// This function returns the number of plug-in classes this DLL
 typedef ClassDesc* (*LibClassDescFun)(int i);

int	debug3dmax::DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts, DWORD options)
{
	FILE* cfg=fopen("debugplugin.txt","r");
	if (cfg==NULL)
	{
		MessageBox( GetActiveWindow(), _T("error: can't find 'debugplugin.txt'"),_T("debugplugin.txt") , 0 );
		return FALSE;
	}
	TCHAR pluginPath[MAX_PATH+1];
	int length=fread(pluginPath,1,sizeof(pluginPath),cfg);
	if (length==0)
	{
		MessageBox( GetActiveWindow(), _T("error: path in 'debugplugin.txt' is empty"),_T( "debugplugin.txt"), 0 );
		return FALSE;
	}
	pluginPath[length]=0;

	HMODULE hModule;
	hModule = ::LoadLibraryEx( pluginPath, NULL, 0 );
	if( hModule == NULL )
	{
		MessageBox( GetActiveWindow(), pluginPath, _T("load plugin failed!"), 0 );
		return FALSE;
	}

	LibNumberClassesFun LibNumFun=NULL;
	LibNumFun=(LibNumberClassesFun)GetProcAddress( hModule, _T("LibNumberClasses") );
	LibClassDescFun LibClassFun=NULL;
	LibClassFun=(LibClassDescFun)GetProcAddress( hModule, _T("LibClassDesc") );

	if( LibNumFun == NULL || LibClassFun == NULL)
	{
		MessageBox( GetActiveWindow(), pluginPath, _T("error: can't find export function"), 0 );
		return FALSE;
	}
	int num=LibNumFun();
	int nRet =FALSE;
	for (int n=0;n<num;++n)
	{
		ClassDesc* desc=LibClassFun(n);
		SceneExport* doexport=(SceneExport*)desc->Create();
		nRet=doexport->DoExport( name, ei, i, suppressPrompts, options );
		delete doexport;
		if (nRet==FALSE)
		{
			break;
		}
	}
	::FreeLibrary( hModule );
	return nRet;
}


