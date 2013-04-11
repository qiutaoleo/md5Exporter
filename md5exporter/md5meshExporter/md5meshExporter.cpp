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

#include "md5meshExporter.h"
#include "../common/SimpleFile.h"
#include "../common//MyErrorProc.h"
#include <IGame/IGame.h>
#include <IGame/IConversionManager.h>
#include <conio.h>
#include <iostream>
using namespace std;

/************************************************************************/
/* 

MD5Version <int:version>
commandline <string:commandline>

numJoints <int:numJoints>
numMeshes <int:numMeshes>
numObjects <int:numObjects>
numMaterials <int:numMaterials>

joints {
<string:name> <int:parentIndex> ( <vec3:position> ) ( <vec3:orientation> )
...
}

objects {
<string:name> <int:startMesh> <int:meshCount>
...
}

materials {
<string:name> <string:diffuseTexture> <string:opacityTexture>
...
}

mesh {
meshindex <int:meshIndex>

shader <string:materialName> 

numverts <int:numVerts>
vert <int:vertexIndex> ( <vec2:texCoords> ) <int:startWeight> <int:weightCount>
...

numtris <int:numTriangles>
tri <int:triangleIndex> <int:vertIndex0> <int:vertIndex1> <int:vertIndex2>
...

numweights <int:numWeights>
weight <int:weightIndex> <int:jointIndex> <float:weightBias> ( <vec3:weightPosition> )
...

}
...

*/
/************************************************************************/

#define md5meshExporter_CLASS_ID	Class_ID(0xdd5ae717, 0x35c3d455)

struct ObjectInfo 
{
	ObjectInfo()
		:StartMesh(0),
		MeshCount(0)
	{

	}

	MCHAR* Name;
	int StartMesh;
	int MeshCount;
};

struct MaterialInfo
{
	MaterialInfo()
		:Diffuse(NULL),
		Opacity(NULL)
	{

	}
	MCHAR* Name;
	MCHAR* Diffuse;
	MCHAR* Opacity;
};



class md5meshExporter : public SceneExport {
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
		md5meshExporter();
		~md5meshExporter();		
public:
	BOOL _CopyImages;
	BOOL _Compress;
	bool _TargetExport;

	FILE* _OutFile;

	IGameScene * pIgame;

	int _BoneCount;
	int _MeshCount;
	int _ObjCount;
	int _MtlCount;
	
	Tab<ObjectInfo*> _ObjInfoList;
	Tab<MaterialInfo*> _MtlInfoList;

	int		SaveMd5Mesh(ExpInterface *ei, Interface *gi);

	void	DumpCount();

	void	CountNodes( IGameNode * pGameNode ) ;

	void	DumpBones();

	void DumpJoint( IGameNode * pGameNode,int& CurIndex,int ParentIndex=-1) 
	{
		IGameObject * obj = pGameNode->GetIGameObject();
		if (obj->GetIGameType()==IGameObject::IGAME_BONE)
		{
			GMatrix mat=obj->GetIGameObjectTM();
			Point3 pos=mat.Translation();
			Point3 quat=mat.Rotation();
			fprintf(_OutFile,"\t\"%s\" %d ( %f %f %f ) ( %f %f %f )\r\n",pGameNode->GetName(),ParentIndex,
				pos.x,pos.y,pos.z,quat.x,quat.y,quat.z);
		}
		
		pGameNode->ReleaseIGameObject();

		int index=CurIndex;
		for(int i=0;i<pGameNode->GetChildCount();i++)
		{
			IGameNode * child = pGameNode->GetNodeChild(i);

			DumpJoint(child,++CurIndex,index);
		}
	}

	void DumpObjects() 
	{
		fprintf(_OutFile,"objects {\r\n");
		for (int i=0;i<_ObjCount;++i)
		{
			fprintf(_OutFile,"\t\"%s\" %d %d\r\n",_ObjInfoList[i]->Name,_ObjInfoList[i]->StartMesh,_ObjInfoList[i]->MeshCount);
			delete _ObjInfoList[i];
			_ObjInfoList[i]=NULL;
		}
		_ObjInfoList.Delete(0,_ObjCount);
		fprintf(_OutFile,"}\r\n\r\n");
	}

	void DumpMaterials() 
	{
		fprintf(_OutFile,"materials {\r\n");
		for (int i=0;i<_MtlCount;++i)
		{
			fprintf(_OutFile,"\t\"%s\" \"%s\" \"%s\"\r\n",_MtlInfoList[i]->Name,
				GetFileName(_MtlInfoList[i]->Diffuse),GetFileName(_MtlInfoList[i]->Opacity));
			delete _MtlInfoList[i];
			_MtlInfoList[i]=NULL;
		}
		_MtlInfoList.Delete(0,_MtlCount);
		fprintf(_OutFile,"}\r\n\r\n");
	}

	void DumpObject( IGameNode * pGameNode ) 
	{
		IGameObject * obj = pGameNode->GetIGameObject();
		if (!pGameNode->IsNodeHidden()&&
			obj->GetIGameType()==IGameObject::IGAME_MESH)
		{
			IGameMesh * gM = (IGameMesh*)obj;
			if(gM->InitializeData())
			{
				DumpSubObject(gM);
				fprintf(_OutFile,"name %s\r\n\r\n",pGameNode->GetName());
			}
			else
			{
				DebugPrint("BadObject\n");
			}
		}

		pGameNode->ReleaseIGameObject();

		for(int i=0;i<pGameNode->GetChildCount();i++)
		{
			IGameNode * child = pGameNode->GetNodeChild(i);

			DumpObject(child);
		}
	}

	void DumpSubObject( IGameMesh * gM ) 
	{
		Tab<int> mapNums = gM->GetActiveMapChannelNum();
		int mapCount = mapNums.Count();
		for(int i=0;i < mapCount;i++)
		{
			int fCount = gM->GetNumberOfFaces();
			IGameMaterial* mat=NULL;
			for(int k=0;k<fCount;k++)
			{
				IGameMaterial* curmat=gM->GetMaterialFromFace(k);
				
				if (mat!=NULL&&strcmp(mat->GetMaterialName(),curmat->GetMaterialName())!=0)
				{
					fprintf(_OutFile,"name \"%s\" %d\r\n\r\n",mat->GetMaterialName(),mat->GetDiffuseData()->GetType());

					int texCount = mat->GetNumberOfTextureMaps();
					for(int i=0;i<texCount;i++)
					{
						IGameTextureMap * tex = mat->GetIGameTextureMap(i);
						TCHAR * name = tex->GetBitmapFileName();
						fprintf(_OutFile,"shader \"%s\" %d %s\r\n\r\n",name,tex->GetStdMapSlot(),tex->GetTextureName());
					}
				}
				mat=curmat;

				
				//DWORD v[3];
				//if (gM->GetMapFaceIndex(mapNums[i],k,v))
				//{
				//	//use data here
				//}
			}
			fprintf(_OutFile,"name \"%s\" %d\r\n\r\n",mat->GetMaterialName(),mat->GetSubMaterialCount());

			int texCount = mat->GetNumberOfTextureMaps();
			for(int i=0;i<texCount;i++)
			{
				IGameTextureMap * tex = mat->GetIGameTextureMap(i);
				TCHAR * name = tex->GetBitmapFileName();
				fprintf(_OutFile,"shader \"%s\"\r\n\r\n",name);
			}
		}
	}

	void DumpMesh( IGameMesh * gM ) 
	{
		Tab<int> mapNums = gM->GetActiveMapChannelNum();
		int mapCount = mapNums.Count();
		for(int i=0;i < mapCount;i++)
		{
			//int vCount = gM->GetNumberOfMapVerts(mapNums[i]);
			//
			//for(int j=0;j<vCount;j++)
			//{
			//	Point3 v;
			//	if(gM->GetMapVertex(mapNums[i],j,v))
			//	{
			//		//use data here
			//	}
			//}
			int fCount = gM->GetNumberOfFaces();
			IGameMaterial* mat=NULL;
			for(int k=0;k<fCount;k++)
			{
				IGameMaterial* curmat=gM->GetMaterialFromFace(k);

				if (mat!=NULL&&strcmp(mat->GetMaterialName(),curmat->GetMaterialName())!=0)
				{
					fprintf(_OutFile,"name \"%s\" %d\r\n\r\n",mat->GetMaterialName(),mat->GetDiffuseData()->GetType());

					int texCount = mat->GetNumberOfTextureMaps();
					for(int i=0;i<texCount;i++)
					{
						IGameTextureMap * tex = mat->GetIGameTextureMap(i);
						TCHAR * name = tex->GetBitmapFileName();
						fprintf(_OutFile,"shader \"%s\" %d %s\r\n\r\n",name,tex->GetStdMapSlot(),tex->GetTextureName());
					}
				}
				mat=curmat;


				//DWORD v[3];
				//if (gM->GetMapFaceIndex(mapNums[i],k,v))
				//{
				//	//use data here
				//}
			}
			fprintf(_OutFile,"name \"%s\" %d\r\n\r\n",mat->GetMaterialName(),mat->GetSubMaterialCount());

			int texCount = mat->GetNumberOfTextureMaps();
			for(int i=0;i<texCount;i++)
			{
				IGameTextureMap * tex = mat->GetIGameTextureMap(i);
				TCHAR * name = tex->GetBitmapFileName();
				fprintf(_OutFile,"shader \"%s\"\r\n\r\n",name);
			}
		}
	}

	void CountMesh( IGameObject * obj) 
	{
		_ObjInfoList[_ObjCount-1]->StartMesh=_MeshCount;
		IGameMesh * gM = (IGameMesh*)obj;
		if(gM->InitializeData())
		{
			Tab<int> mapNums = gM->GetActiveMapChannelNum();
			int mapCount = mapNums.Count();
			for(int i=0;i < mapCount;i++)
			{
				int fCount = gM->GetNumberOfFaces();
				Tab<TSTR> meshMtlNames;
				for(int k=0;k<fCount;k++)
				{
					IGameMaterial* mat=gM->GetMaterialFromFace(k);
					int nameCount=meshMtlNames.Count();
					int n=0;
					TSTR matName=mat->GetMaterialName();
					for (;n<nameCount;++n)
					{
						TSTR curName=meshMtlNames[n];
						if (matName==curName)
						{
							break;
						}
					}

					if (n==nameCount)
					{
						meshMtlNames.Append(1,&matName);
						_MeshCount++;

						CoutMtl(mat);
					}
				}
			}
			_ObjInfoList[_ObjCount-1]->MeshCount=_MeshCount-_ObjInfoList[_ObjCount-1]->StartMesh;
		}
		else
		{
			DebugPrint("BadObject\n");
		}
	}

	void CoutMtl( IGameMaterial* pGameMtl );

	TSTR GetFileName( MCHAR* path ) 
	{
		if (path)
		{
			TSTR strPath,strFile;
			SplitPathFile( path,&strPath,&strFile );
			return strFile;
		}
		return _T("");
	}



	









};



class md5meshExporterClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 		{ return new md5meshExporter(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return SCENE_EXPORT_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return md5meshExporter_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("md5meshExporter"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* Getmd5meshExporterDesc() { 
	static md5meshExporterClassDesc md5meshExporterDesc;
	return &md5meshExporterDesc; 
}





INT_PTR CALLBACK md5meshExporterOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	static md5meshExporter *imp = NULL;

	switch(message) {
		case WM_INITDIALOG:
			imp = (md5meshExporter *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));

			CheckDlgButton(hWnd, IDC_COPY_IMAGES, imp->_CopyImages);
			CheckDlgButton(hWnd, IDC_COMPRESS, imp->_Compress);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
			case IDC_OK:
				imp->_CopyImages = IsDlgButtonChecked(hWnd, IDC_COPY_IMAGES);
				imp->_Compress = IsDlgButtonChecked(hWnd, IDC_COMPRESS);
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


//--- md5meshExporter -------------------------------------------------------
md5meshExporter::md5meshExporter()
	:
_CopyImages(FALSE),
_Compress(FALSE)
{

}

md5meshExporter::~md5meshExporter() 
{

}

int md5meshExporter::ExtCount()
{
	//#pragma message(TODO("Returns the number of file name extensions supported by the plug-in."))
	return 1;
}

const TCHAR *md5meshExporter::Ext(int n)
{		
	//#pragma message(TODO("Return the 'i-th' file name extension (i.e. \"3DS\")."))
	return _T("md5mesh");
}

const TCHAR *md5meshExporter::LongDesc()
{
	//#pragma message(TODO("Return long ASCII description (i.e. \"Targa 2.0 Image File\")"))
	return _T("MD5 Mesh File");
}
	
const TCHAR *md5meshExporter::ShortDesc() 
{			
	//#pragma message(TODO("Return short ASCII description (i.e. \"Targa\")"))
	return _T("MD5 Mesh");
}

const TCHAR *md5meshExporter::AuthorName()
{			
	//#pragma message(TODO("Return ASCII Author name"))
	return _T("HoneyCat");
}

const TCHAR *md5meshExporter::CopyrightMessage() 
{	
	//#pragma message(TODO("Return ASCII Copyright message"))
	return _T("Copyright (C) 2013 HoneyCat. All rights reserved.");
}

const TCHAR *md5meshExporter::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *md5meshExporter::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int md5meshExporter::Version()
{				
	//#pragma message(TODO("Return Version number * 100 (i.e. v3.01 = 301)"))
	return 100;
}

void md5meshExporter::ShowAbout(HWND hWnd)
{			
	// Optional
}

BOOL md5meshExporter::SupportsOptions(int ext, DWORD options)
{
	//#pragma message(TODO("Decide which options to support.  Simply return true for each option supported by each Extension the exporter supports."))
	return TRUE;
}


int	md5meshExporter::DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts, DWORD options)
{
	//#pragma message(TODO("Implement the actual file Export here and"))

	AllocConsole();
	freopen("CONOUT$","w+t",stdout); 
	freopen("CONIN$","r+t",stdin);
	printf("开始导出\n");
	int result=TRUE;
	if(!suppressPrompts)
	{

		result =DialogBoxParam(hInstance, 
				MAKEINTRESOURCE(IDD_PANEL), 
				GetActiveWindow(), 
				md5meshExporterOptionsDlgProc, (LPARAM)this);
		if (result>0)
		{
			MyErrorProc pErrorProc;
			SetErrorCallBack(&pErrorProc);

			pIgame=GetIGameInterface();

			IGameConversionManager * cm = GetConversionManager();
			cm->SetCoordSystem(IGameConversionManager::IGAME_D3D);
			_TargetExport=(options & SCENE_EXPORT_SELECTED) ? true : false;
			pIgame->InitialiseIGame();
			pIgame->SetStaticFrame(0);

			SimpleFile outFile(name,"wb");
			_OutFile=outFile.File();

			result=SaveMd5Mesh(ei,i);

			pIgame->ReleaseIGame();
			_OutFile=NULL;
		}
		else
			result=TRUE;
	}
	getch();
	FreeConsole();
	return result;
}

int md5meshExporter::SaveMd5Mesh( ExpInterface *ei, Interface *gi )
{
	fprintf(_OutFile,"MD5Version 4843\r\ncommandline \"by HoneyCat md5meshExporter v%d\"\r\n\r\n",Version());

	DumpCount();

	DumpBones();

	DumpObjects();

	DumpMaterials();

	fflush(_OutFile);
	return TRUE;
}


void md5meshExporter::DumpCount()
{
	_BoneCount=0;
	_MeshCount=0;
	_ObjCount=0;
	_MtlCount=0;

	for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
	{
		IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
		CountNodes(pGameNode);

	}
	_MtlCount=_MtlInfoList.Count();

	fprintf(_OutFile,"numJoints %d\r\n",_BoneCount);
	fprintf(_OutFile,"numMeshes %d\r\n",_MeshCount);
	fprintf(_OutFile,"numObjects %d\r\n",_ObjCount);
	fprintf(_OutFile,"numMaterials %d\r\n\r\n",_MtlCount);
}

void md5meshExporter::CountNodes( IGameNode * pGameNode )
{
	IGameObject * obj = pGameNode->GetIGameObject();
	if (obj->GetIGameType()==IGameObject::IGAME_BONE)
	{
		_BoneCount++;
	}
	else if (!pGameNode->IsNodeHidden()&&
		obj->GetIGameType()==IGameObject::IGAME_MESH)
	{
		_ObjCount++;

		ObjectInfo* info=new ObjectInfo();
		info->Name=pGameNode->GetName();
		_ObjInfoList.Append(1,&info);

		CountMesh(obj);
	}
	fprintf(_OutFile,"%s %d\n",pGameNode->GetName(),obj->GetIGameType());
	pGameNode->ReleaseIGameObject();

	for(int i=0;i<pGameNode->GetChildCount();i++)
	{
		IGameNode * child = pGameNode->GetNodeChild(i);

		CountNodes(child);
	}
}

void md5meshExporter::DumpBones() 
{
	fprintf(_OutFile,"joints {\r\n");
	for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
	{
		IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
		
		int index=0;
		DumpJoint(pGameNode,index);
	}
	fprintf(_OutFile,"}\r\n\r\n");
}

void md5meshExporter::CoutMtl( IGameMaterial* pGameMtl )
{
	int m=0;
	TSTR mtlName=pGameMtl->GetMaterialName();
	for (;m<_MtlCount;++m)
	{
		TSTR curName=_MtlInfoList[m]->Name;
		if (mtlName==curName)
		{
			break;
		}
	}

	if (m==_MtlCount)
	{
		MaterialInfo* mtlInfo=new MaterialInfo();
		mtlInfo->Name=pGameMtl->GetMaterialName();

		int texCount = pGameMtl->GetNumberOfTextureMaps();
		for(int i=0;i<texCount;i++)
		{
			IGameTextureMap * tex = pGameMtl->GetIGameTextureMap(i);
			MCHAR * name = tex->GetBitmapFileName();
			switch (tex->GetStdMapSlot())
			{
			case ID_DI:
				mtlInfo->Diffuse=name;
				break;
			case ID_OP:
				mtlInfo->Opacity=name;
				break;
			}
		}
		
		_MtlInfoList.Append(1,&mtlInfo);
		_MtlCount++;
	}
}



