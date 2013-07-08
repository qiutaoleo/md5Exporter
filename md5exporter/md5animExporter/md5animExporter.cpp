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
// AUTHOR: HoneyCat
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
	TimeValue _TvToDump;
	int _FrameCount;
	int _BoneCount;
	int _FrameRate;
	int _AnimatedCount;

	BOOL _IncludeBounds;
	BOOL _HelperObject;
	BOOL _DoomVersion;

	FILE* _OutFile;

	IGameScene * pIgame;

	vector<BoneInfo> _BoneList;

	int SaveMd5Anim( ExpInterface * ei, Interface * gi ) 
	{
		if (!_DoomVersion)
			fprintf(_OutFile,"MD5Version 4843\r\ncommandline \"by HoneyCat md5animExporter v%d\"\r\n\r\n",Version());
		else
			fprintf(_OutFile,"MD5Version 10\r\ncommandline \"by HoneyCat md5animExporter v%d\"\r\n\r\n",Version());

		DumpCount(gi);

		DumpHierarchy();

		if (_IncludeBounds)
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

		BoneInfo bone;
		bone.Name="origin";
		bone.SelfIndex=(int)_BoneList.size();
		bone.ParentIndex=-1;
		bone.Flag=0;
		bone.StartIndex=0;
		bone.Pos=Point3::Origin;
		bone.Rot=Quat();

		_BoneList.push_back(bone);
		_BoneCount++;

		for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
		{
			IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
			CountNodes(pGameNode,0);
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
		if ((obj->GetIGameType()==IGameObject::IGAME_BONE&&
			(_HelperObject||obj->GetMaxObject()->SuperClassID()!=HELPER_CLASS_ID))||
			(_HelperObject&&obj->GetIGameType()==IGameObject::IGAME_HELPER))
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
		//GetLocalTM的值实际上与max节点的自身变换是一样的
		GMatrix mat=pGameNode->GetLocalTM(_TvToDump);

		mat=ToRightHand(mat);

		bone.Pos=mat.Translation();
		bone.Rot=mat.Rotation();
	}

	GMatrix ToRightHand(const GMatrix& mat) const
	{
		Point3 pos=mat.Translation();
		Quat quat=mat.Rotation();
		Matrix3 ret=mat.ExtractMatrix3();
		//检测是否是镜像
		bool isMirrored=DotProd( CrossProd( ret.GetRow(0).Normalize(), ret.GetRow(1).Normalize() ).Normalize(), ret.GetRow(2).Normalize() ) < 0;
		//如果是则对镜像进行旋转的修正
		//修正方式还有待调整
		if (isMirrored)
		{
			float tmp;
			tmp=quat.x;
			quat.x=-quat.y;
			quat.y=tmp;
			tmp=quat.z;
			quat.z=quat.w;
			quat.w=-tmp;
		}
		ret.SetRotate(quat);
		ret.SetTrans(pos);
		return GMatrix(ret);
	}

	bool AlmostEqual2sComplement(float A, float B, int maxUlps=3)
	{
		// Make sure maxUlps is non-negative and small enough that the
		// default NAN won't compare as equal to anything.
		assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
		int aInt = *(int*)&A;
		// Make aInt lexicographically ordered as a twos-complement int
		if (aInt < 0)
			aInt = 0x80000000 - aInt;
		// Make bInt lexicographically ordered as a twos-complement int
		int bInt = *(int*)&B;
		if (bInt < 0)
			bInt = 0x80000000 - bInt;
		int intDiff = abs(aInt - bInt);
		if (intDiff <= maxUlps)
			return true;
		return false;
	}

	void CountAnimated(IGameNode * pGameNode,IGameObject * obj,BoneInfo & bone) 
	{
		bone.StartIndex=_AnimatedCount;
		int flag=0;

		TimeValue start=pIgame->GetSceneStartTime();
		TimeValue end=pIgame->GetSceneEndTime();
		TimeValue ticks=pIgame->GetSceneTicks();

		for (TimeValue tv = start; tv <= end; tv += ticks)
		{
			GMatrix mat=pGameNode->GetLocalTM(tv);
			mat=ToRightHand(mat);
			Point3 pos=mat.Translation();
			Quat rot=mat.Rotation();

			if (!AlmostEqual2sComplement(pos.x,bone.Pos.x))
			{
				flag|=eHierarchyFlag_Pos_X;
			}
			if (!AlmostEqual2sComplement(pos.y,bone.Pos.y))
			{
				flag|=eHierarchyFlag_Pos_Y;
			}
			if (!AlmostEqual2sComplement(pos.z,bone.Pos.z))
			{
				flag|=eHierarchyFlag_Pos_Z;
			}

			if (!AlmostEqual2sComplement(rot.x,bone.Rot.x))
			{
				flag|=eHierarchyFlag_Rot_X;
			}
			if (!AlmostEqual2sComplement(rot.y,bone.Rot.y))
			{
				flag|=eHierarchyFlag_Rot_Y;
			}
			if (!AlmostEqual2sComplement(rot.z,bone.Rot.z))
			{
				flag|=eHierarchyFlag_Rot_Z;
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

			int index=1;
			for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
			{
				IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);

				DumpPosRot(pGameNode,tv,index);
			}

			fprintf(_OutFile,"}\r\n\r\n");
		}
		
	}

	void DumpPosRot( IGameNode * pGameNode,TimeValue tv,int& nodeIndex ) 
	{
		IGameObject * obj = pGameNode->GetIGameObject();
		if ((obj->GetIGameType()==IGameObject::IGAME_BONE&&
			(_HelperObject||obj->GetMaxObject()->SuperClassID()!=HELPER_CLASS_ID))||
			(_HelperObject&&obj->GetIGameType()==IGameObject::IGAME_HELPER))
		{
			GMatrix mat=pGameNode->GetLocalTM(tv);
			mat=ToRightHand(mat);
			Point3 pos=mat.Translation();
			Quat rot=mat.Rotation();

			if (rot.w<0)
			{
				rot=-rot;
			}

			AnimData(nodeIndex, pos, rot);


			nodeIndex++;
		}

		pGameNode->ReleaseIGameObject();

		for(int i=0;i<pGameNode->GetChildCount();i++)
		{
			IGameNode * child = pGameNode->GetNodeChild(i);

			DumpPosRot(child,tv,nodeIndex);
		}
	}

	void AnimData( int& nodeIndex, const Point3 &pos, const Quat &rot ) 
	{
		BoneInfo& info=_BoneList.at(nodeIndex);
		float data[6];
		int size=0;
		if (info.Flag&eHierarchyFlag_Pos_X)
			data[size++]=pos.x;
		if (info.Flag&eHierarchyFlag_Pos_Y)
			data[size++]=pos.y;
		if (info.Flag&eHierarchyFlag_Pos_Z)
			data[size++]=pos.z;

		if (info.Flag&eHierarchyFlag_Rot_X)
			data[size++]=rot.x;
		if (info.Flag&eHierarchyFlag_Rot_Y)
			data[size++]=rot.y;
		if (info.Flag&eHierarchyFlag_Rot_Z)
			data[size++]=rot.z;
		switch (size)
		{
		case 0:
			break;
		case 1:
			fprintf(_OutFile,"\t%f\r\n",
				data[0]);
			break;
		case 2:
			fprintf(_OutFile,"\t%f %f\r\n",
				data[0],data[1]);
			break;
		case 3:
			fprintf(_OutFile,"\t%f %f %f\r\n",
				data[0],data[1],data[2]);
			break;
		case 4:
			fprintf(_OutFile,"\t%f %f %f %f\r\n",
				data[0],data[1],data[2],
				data[3]);
			break;
		case 5:
			fprintf(_OutFile,"\t%f %f %f %f %f\r\n",
				data[0],data[1],data[2],
				data[3],data[4]);
			break;
		case 6:
			fprintf(_OutFile,"\t%f %f %f %f %f %f\r\n",
				data[0],data[1],data[2],
				data[3],data[4],data[5]);
			break;
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

			CheckDlgButton(hWnd,IDC_CHECK_INCLUDEBOUNDS,imp->_IncludeBounds);
			CheckDlgButton(hWnd,IDC_CHECK_HELPER_OBJECT,imp->_HelperObject);
			CheckDlgButton(hWnd,IDC_DOOM_VERSION,imp->_DoomVersion);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
			case IDC_OK:
				imp->_IncludeBounds = IsDlgButtonChecked(hWnd, IDC_CHECK_INCLUDEBOUNDS);
				imp->_HelperObject = IsDlgButtonChecked(hWnd, IDC_CHECK_HELPER_OBJECT);
				imp->_DoomVersion = IsDlgButtonChecked(hWnd, IDC_DOOM_VERSION);
				EndDialog(hWnd, 1);
				return TRUE;
			case IDC_CANCEL:
				EndDialog(hWnd, 0);
				return TRUE;
			case IDC_CHECK_INCLUDEBOUNDS:
				return TRUE;
			case IDC_CHECK_HELPER_OBJECT:
				return TRUE;
			case IDC_DOOM_VERSION:
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
	:_IncludeBounds(FALSE),
	_HelperObject(FALSE),
	_DoomVersion(FALSE)
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
	return 115;
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

			_TvToDump=0;

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


