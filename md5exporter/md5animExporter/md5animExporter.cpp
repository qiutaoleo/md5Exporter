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

#include "md5animExporter.h"
#include "../common/SimpleFile.h"
#include "../common/MyErrorProc.h"
#include "../common/Define.h"
#include <IGame/IGame.h>
#include <IGame/IConversionManager.h>
#include <IGame/IGameModifier.h>
#include <conio.h>
#include <iostream>
#include <hash_map>
#include <map>
using namespace std;

/************************************************************************/
/* 

MD5Version <int:version>
commandline <string:commandline>

numFrames <int:numFrames>
numJoints <int:numJoints>
frameRate <int:frameRate>
numAnimatedComponents <int:numAnimatedComponents>

hierarchy {
<string:jointName> <int:parentIndex> <int:flags> <int:startIndex>
...
}

bounds {
( vec3:boundMin ) ( vec3:boundMax )
...
}

baseframe {
( vec3:position ) ( vec3:orientation )
...
}

frame <int:frameNum> {
<float:frameData> ...
}
...

*/
/************************************************************************/


#define md5animExporter_CLASS_ID	Class_ID(0x7833d653, 0xc53b2ccf)

struct BoneInfo 
{
	BoneInfo()
		:Name(NULL),
		SelfIndex(-1),
		ParentIndex(-1),
		Flag(0),
		StartIndex(-1)
	{

	}
	MCHAR* Name;
	int SelfIndex;
	int ParentIndex;
	int Flag;
	int StartIndex;
	Point3 Pos;
	Quat Rot;
};



class md5animExporter : public SceneExport {
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
		md5animExporter();
		~md5animExporter();		
public:
	int _FrameCount;
	int _BoneCount;
	int _FrameRate;
	int _AnimatedCount;

	FILE* _OutFile;

	IGameScene * pIgame;

	vector<BoneInfo> _BoneList;

	int SaveMd5Anim( ExpInterface * ei, Interface * gi ) 
	{
		fprintf(_OutFile,"MD5Version 4843\r\ncommandline \"by HoneyCat md5animExporter v%d\"\r\n\r\n",Version());

		DumpCount(gi);

		DumpHierarchy();

		DumpBounds();

		DumpBaseFrame();

		DumpFrames();

		fflush(_OutFile);
		return TRUE;
	}

	void DumpCount(Interface * gi) 
	{
		Interval animation=gi->GetAnimRange();
		_FrameCount=(animation.End()-animation.Start())/GetTicksPerFrame()+1;
		_BoneCount=0;
		_FrameRate=GetFrameRate();
		_AnimatedCount=0;

		for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
		{
			IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
			CountNodes(pGameNode);
		}

		fprintf(_OutFile,"numFrames %d\r\n",_FrameCount);
		fprintf(_OutFile,"numJoints %d\r\n",_BoneCount);
		fprintf(_OutFile,"frameRate %d\r\n",_FrameRate);
		fprintf(_OutFile,"numAnimatedComponents %d\r\n\r\n",_AnimatedCount);
	}

	void CountNodes( IGameNode * pGameNode ,int ParentIndex=-1) 
	{
		int index=(int)_BoneList.size();
		IGameObject * obj = pGameNode->GetIGameObject();
		if (obj->GetIGameType()==IGameObject::IGAME_BONE&&
			obj->GetMaxObject()->SuperClassID()!=HELPER_CLASS_ID)
		{
			_BoneCount++;

			BoneInfo bone;
			bone.Name=pGameNode->GetName();
			bone.SelfIndex=(int)_BoneList.size();
			bone.ParentIndex=ParentIndex;

			BasePose(pGameNode,obj,bone);

			CountAnimated(pGameNode,obj,bone);

			_BoneList.push_back(bone);
		}
		
		//fprintf(_OutFile,"%s %d\n",pGameNode->GetName(),obj->GetIGameType());
		pGameNode->ReleaseIGameObject();

		for(int i=0;i<pGameNode->GetChildCount();i++)
		{
			IGameNode * child = pGameNode->GetNodeChild(i);

			CountNodes(child,index);
		}
	}

	void BasePose( IGameNode * pGameNode,IGameObject * obj , BoneInfo& bone ) 
	{
		GMatrix mat=obj->GetIGameObjectTM();
		IGameNode* parent=pGameNode->GetNodeParent();
		if (parent)
		{
			GMatrix parentMat=parent->GetIGameObject()->GetIGameObjectTM();
			mat*=parentMat.Inverse();
		}

		bone.Pos=mat.Translation();
		bone.Rot=mat.Rotation();
	}

	void CountAnimated(IGameNode * pGameNode,IGameObject * obj,BoneInfo & bone) 
	{
		bone.StartIndex=_AnimatedCount;

		IGameKeyTab posKeys,rotKeys;
		IGameControl * pGC = pGameNode->GetIGameControl();
		int flag=0;
		if (pGC->GetFullSampledKeys(posKeys,_FrameRate,IGAME_POS))
		{
			for(int i = 0;i<posKeys.Count();i++)
			{
				Point3& pos=posKeys[i].sampleKey.pval;
				if (abs(pos.x-bone.Pos.x)>1E-6f)
				{
					flag|=eHierarchyFlag_Pos_X;
				}
				if (abs(pos.y-bone.Pos.y)>1E-6f)
				{
					flag|=eHierarchyFlag_Pos_Y;
				}
				if (abs(pos.z-bone.Pos.z)>1E-6f)
				{
					flag|=eHierarchyFlag_Pos_Z;
				}
			}
		}
		if (pGC->GetFullSampledKeys(rotKeys,_FrameRate,IGAME_ROT))
		{
			for(int i = 0;i<rotKeys.Count();i++)
			{
				Quat& rot=rotKeys[i].sampleKey.qval;
				if (abs(rot.x-bone.Rot.x)>1E-6f)
				{
					flag|=eHierarchyFlag_Rot_X;
				}
				if (abs(rot.y-bone.Rot.y)>1E-6f)
				{
					flag|=eHierarchyFlag_Rot_Y;
				}
				if (abs(rot.z-bone.Rot.z)>1E-6f)
				{
					flag|=eHierarchyFlag_Rot_Z;
				}
			}
		}

		if (flag&eHierarchyFlag_Pos_X)
			_AnimatedCount++;
		if (flag&eHierarchyFlag_Pos_Y)
			_AnimatedCount++;
		if (flag&eHierarchyFlag_Pos_Z)
			_AnimatedCount++;

		if (flag&eHierarchyFlag_Rot_X)
			_AnimatedCount++;
		if (flag&eHierarchyFlag_Rot_Y)
			_AnimatedCount++;
		if (flag&eHierarchyFlag_Rot_Z)
			_AnimatedCount++;

		bone.Flag=flag;
	}

	void DumpHierarchy() 
	{
		fprintf(_OutFile,"hierarchy {\r\n");
		for(int i = 0; i <_BoneCount;i++)
		{
			BoneInfo& info=_BoneList.at(i);
			fprintf(_OutFile,"\t\"%s\" %d %d %d\r\n",info.Name,info.ParentIndex,info.Flag,info.StartIndex);
		}
		fprintf(_OutFile,"}\r\n\r\n");
	}

	void DumpBounds() 
	{
		fprintf(_OutFile,"bounds {\r\n");

		TimeValue start=pIgame->GetSceneStartTime();
		TimeValue end=pIgame->GetSceneEndTime();
		TimeValue ticks=pIgame->GetSceneTicks();

		for (TimeValue tv = start; tv <= end; tv += ticks)
		{
			Box3 objBound;
			for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
			{
				IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
				IGameObject * obj = pGameNode->GetIGameObject();
				if (!pGameNode->IsNodeHidden()&&
					obj->GetIGameType()==IGameObject::IGAME_MESH)
				{
					INode* maxNode=pGameNode->GetMaxNode();
					const ObjectState& objState=maxNode->EvalWorldState(tv);
					Object* maxObj=objState.obj;
					Box3 bb;
					Matrix3 mat=maxNode->GetNodeTM(tv);
					maxObj->GetDeformBBox(tv,bb,&mat);
					objBound+=bb;
				}
			}

			Point3 min=objBound.Min()-_BoneList.at(0).Pos;
			Point3 max=objBound.Max()-_BoneList.at(0).Pos;
			fprintf(_OutFile,"\t( %f %f %f ) ( %f %f %f )\r\n",
				min.x,min.y,min.z,
				max.x,max.y,max.z);
		}

		
		fprintf(_OutFile,"}\r\n\r\n");
	}

	void DumpBaseFrame() 
	{
		fprintf(_OutFile,"baseframe {\r\n");

		int boneCount=(int)_BoneList.size();
		for (int i=0;i<boneCount;++i)
		{
			BoneInfo& info=_BoneList.at(i);
			Quat rot=info.Rot;
			if (rot.w<0)
			{
				rot=-rot;
			}
			fprintf(_OutFile,"\t( %f %f %f ) ( %f %f %f )\r\n",
				info.Pos.x,info.Pos.y,info.Pos.z,
				rot.x,rot.y,rot.z);
		}

		fprintf(_OutFile,"}\r\n\r\n");
	}

	void DumpFrames() 
	{
		TimeValue start=pIgame->GetSceneStartTime();
		TimeValue end=pIgame->GetSceneEndTime();
		TimeValue ticks=pIgame->GetSceneTicks();

		for (TimeValue tv = start; tv <= end; tv += ticks)
		{
			int frameNum=tv/ticks;
			fprintf(_OutFile,"frame %d {\r\n",frameNum);

			for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
			{
				IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
				DumpPosRot(pGameNode,tv);
			}

			fprintf(_OutFile,"}\r\n\r\n");
		}
		
	}

	void DumpPosRot( IGameNode * pGameNode,TimeValue tv ) 
	{
		IGameObject * obj = pGameNode->GetIGameObject();
		if (obj->GetIGameType()==IGameObject::IGAME_BONE&&
			obj->GetMaxObject()->SuperClassID()!=HELPER_CLASS_ID)
		{
			INode* maxNode=pGameNode->GetMaxNode();
			const ObjectState& objState=maxNode->EvalWorldState(tv);
			Matrix3 mat=maxNode->GetNodeTM(tv);
			mat.NoScale();
			INode* parent=maxNode->GetParentNode();
			bool isMirrored=false;

			if (parent)
			{
				Matrix3 parentMat=parent->GetNodeTM(tv);
				isMirrored=DotProd( CrossProd( parentMat.GetRow(0).Normalize(),parentMat.GetRow(1).Normalize() ).Normalize(), parentMat.GetRow(2).Normalize() ) < 0;
				parentMat.NoScale();
				parentMat.Invert();
				mat*=parentMat;
			}

			Point3 pos=mat.GetTrans();
			if (isMirrored)
			{
				pos = pos * -1.0f;
			}
			Quat rot(mat);

			fprintf(_OutFile,"\t%f %f %f %f %f %f\r\n",
				pos.x,pos.y,pos.z,
				rot.x,rot.y,rot.z);
		}

		pGameNode->ReleaseIGameObject();

		for(int i=0;i<pGameNode->GetChildCount();i++)
		{
			IGameNode * child = pGameNode->GetNodeChild(i);

			DumpPosRot(child,tv);
		}
	}


















};



class md5animExporterClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 		{ return new md5animExporter(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return SCENE_EXPORT_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return md5animExporter_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("md5animExporter"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* Getmd5animExporterDesc() { 
	static md5animExporterClassDesc md5animExporterDesc;
	return &md5animExporterDesc; 
}





INT_PTR CALLBACK md5animExporterOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	static md5animExporter *imp = NULL;

	switch(message) {
		case WM_INITDIALOG:
			imp = (md5animExporter *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
			case IDC_OK:
				EndDialog(hWnd, 1);
				return TRUE;
			case IDC_CANCEL:
				EndDialog(hWnd, 0);
				return TRUE;
			default:
				break;
			}

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return TRUE;
	}
	return FALSE;
}


//--- md5animExporter -------------------------------------------------------
md5animExporter::md5animExporter()
{

}

md5animExporter::~md5animExporter() 
{

}

int md5animExporter::ExtCount()
{
	//#pragma message(TODO("Returns the number of file name extensions supported by the plug-in."))
	return 1;
}

const TCHAR *md5animExporter::Ext(int n)
{		
	//#pragma message(TODO("Return the 'i-th' file name extension (i.e. \"3DS\")."))
	return _T("md5anim");
}

const TCHAR *md5animExporter::LongDesc()
{
	//#pragma message(TODO("Return long ASCII description (i.e. \"Targa 2.0 Image File\")"))
	return _T("MD5 Anim file");
}
	
const TCHAR *md5animExporter::ShortDesc() 
{			
	//#pragma message(TODO("Return short ASCII description (i.e. \"Targa\")"))
	return _T("MD5 Anim");
}

const TCHAR *md5animExporter::AuthorName()
{			
	//#pragma message(TODO("Return ASCII Author name"))
	return _T("HoneyCat");
}

const TCHAR *md5animExporter::CopyrightMessage() 
{	
	//#pragma message(TODO("Return ASCII Copyright message"))
	return _T("Copyright (C) 2013 HoneyCat. All rights reserved.");
}

const TCHAR *md5animExporter::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *md5animExporter::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int md5animExporter::Version()
{				
	//#pragma message(TODO("Return Version number * 100 (i.e. v3.01 = 301)"))
	return 100;
}

void md5animExporter::ShowAbout(HWND hWnd)
{			
	// Optional
}

BOOL md5animExporter::SupportsOptions(int ext, DWORD options)
{
	//#pragma message(TODO("Decide which options to support.  Simply return true for each option supported by each Extension the exporter supports."))
	return TRUE;
}


int	md5animExporter::DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts, DWORD options)
{
	//#pragma message(TODO("Implement the actual file Export here and"))

	int result=TRUE;
	if(!suppressPrompts)
	{

		result =DialogBoxParam(hInstance, 
			MAKEINTRESOURCE(IDD_PANEL), 
			GetActiveWindow(), 
			md5animExporterOptionsDlgProc, (LPARAM)this);
		if (result>0)
		{
			MyErrorProc pErrorProc;
			SetErrorCallBack(&pErrorProc);

			pIgame=GetIGameInterface();

			IGameConversionManager * cm = GetConversionManager();
			cm->SetCoordSystem(IGameConversionManager::IGAME_MAX);
			pIgame->InitialiseIGame();
			pIgame->SetStaticFrame(0);

			SimpleFile outFile(name,"wb");
			_OutFile=outFile.File();

			result=SaveMd5Anim(ei,i);

			MessageBox( GetActiveWindow(), name, _T("md5anim finish"), 0 );

			pIgame->ReleaseIGame();
			_OutFile=NULL;
		}
		else
			result=TRUE;
	}
	return result;
}


