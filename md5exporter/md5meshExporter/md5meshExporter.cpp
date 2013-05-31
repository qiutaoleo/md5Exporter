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
#include "../common/MyErrorProc.h"
#include "../common/Define.h"
#include <IGame/IGame.h>
#include <IGame/IConversionManager.h>
#include <IGame/IGameModifier.h>
#include <conio.h>
#include <iostream>
#include <hash_map>
#include <map>
#include <hash_set>
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

struct WeightInfo;

struct VertexInfo
{
	VertexInfo()
		:VertIndex(-1),
		TexCoordIndex(-1),
		WeightIndex(-1),
		WeightCount(-1)
	{

	}
	int VertIndex;
	int TexCoordIndex;
	Point3 Pos;
	Point2 UV;
	int WeightIndex;
	int WeightCount;

	vector<WeightInfo> Weights;
};

struct TriInfo
{
	int Index[3];
};

struct WeightInfo 
{
	WeightInfo()
		:Value(0.f)
	{

	}
	float Value;
	IGameNode* Bone;
	Point3 Offset;
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
	TimeValue _TvToDump;
	BOOL _CopyImages;
	BOOL _Compress;
	
	BOOL _LimitBoneNumPerMesh;
	int _MaxBonePerMesh;
	BOOL _DoomVersion;

	bool _TargetExport;

	FILE* _OutFile;

	IGameScene * pIgame;

	int _BoneCount;
	int _MeshCount;
	int _ObjCount;
	int _MtlCount;
	
	hash_map<MCHAR*,int> _BoneIndexMap;
	vector<ObjectInfo> _ObjInfoList;
	vector<MaterialInfo> _MtlInfoList;

	int		SaveMd5Mesh(ExpInterface *ei, Interface *gi);

	void	DumpCount();

	void	CountNodes( IGameNode * pGameNode,BOOL isBoneChild=FALSE ) ;

	void	DumpBones();

	void DumpJoint( IGameNode * pGameNode,int& CurIndex,int ParentIndex=-1) 
	{
		int index=CurIndex;
		IGameObject * obj = pGameNode->GetIGameObject();
		if (obj->GetIGameType()==IGameObject::IGAME_BONE&&
			obj->GetMaxObject()->SuperClassID()!=HELPER_CLASS_ID)
		{
			GMatrix mat=pGameNode->GetWorldTM(_TvToDump);
			Point3 pos=mat.Translation();
			Quat quat=mat.Rotation();
			if (quat.w<0)
			{
				quat=-quat;
			}
			MCHAR* name=pGameNode->GetName();
			fprintf(_OutFile,"\t\"%s\" %d ( %f %f %f ) ( %f %f %f )\r\n",name,ParentIndex,
				pos.x,pos.y,pos.z,quat.x,quat.y,quat.z);
			++CurIndex;
			_BoneIndexMap[name]=index;
		}
		
		pGameNode->ReleaseIGameObject();

		for(int i=0;i<pGameNode->GetChildCount();i++)
		{
			IGameNode * child = pGameNode->GetNodeChild(i);

			DumpJoint(child,CurIndex,index);
		}
	}

	void DumpObjects() 
	{
		fprintf(_OutFile,"objects {\r\n");
		for (int i=0;i<_ObjCount;++i)
		{
			fprintf(_OutFile,"\t\"%s\" %d %d\r\n",_ObjInfoList.at(i).Name,_ObjInfoList.at(i).StartMesh,_ObjInfoList.at(i).MeshCount);
		}
		_ObjInfoList.clear();
		fprintf(_OutFile,"}\r\n\r\n");
	}

	void DumpMaterials() 
	{
		fprintf(_OutFile,"materials {\r\n");
		for (int i=0;i<_MtlCount;++i)
		{
			fprintf(_OutFile,"\t\"%s\" \"%s\" \"%s\"\r\n",_MtlInfoList.at(i).Name,
				GetFileName(_MtlInfoList.at(i).Diffuse),GetFileName(_MtlInfoList.at(i).Opacity));
		}
		_MtlInfoList.clear();
		fprintf(_OutFile,"}\r\n\r\n");
	}

	void DumpMeshes() 
	{
		_MeshCount=0;
		for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
		{
			IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
			DumpMesh(pGameNode);

		}
	}

	void DumpMesh( IGameNode * pGameNode ) 
	{
		IGameObject * obj = pGameNode->GetIGameObject();
		if (!pGameNode->IsNodeHidden()&&
			obj->GetIGameType()==IGameObject::IGAME_MESH)
		{
			IGameMesh * gM = (IGameMesh*)obj;
			if(gM->InitializeData())
			{
				int numMod = obj->GetNumModifiers();
				if(numMod > 0)
				{
					for(int i=0;i<numMod;i++)
					{
						IGameModifier * gMod = obj->GetIGameModifier(i);
						if (gMod->IsSkin())
						{
							DumpSubMesh(pGameNode,((IGameSkin*)gMod)->GetInitialPose(),(IGameSkin*)gMod);//((IGameSkin*)gMod)->GetInitialPose()gM//
							break;;
						}
					}
				}
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

			DumpMesh(child);
		}
	}

	void DumpSubMesh( IGameNode * pGameNode,IGameMesh * gM ,IGameSkin* gSkin) 
	{
		Tab<int> matIDs=gM->GetActiveMatIDs();
		int matCount=matIDs.Count();
		hash_map<MCHAR*,vector<FaceEx *> > sameMatMap;

		for (int m=0;m<matCount;++m)
		{
			Tab<FaceEx *> faces=gM->GetFacesFromMatID(matIDs[m]);
			IGameMaterial * mat=gM->GetMaterialFromFace(faces[0]);
			vector<FaceEx *>& faceList=sameMatMap[mat->GetMaterialName()];
			for (int f=0;f<faces.Count();++f)
			{
				faceList.push_back(faces[f]);
			}
		}
		
		for (hash_map<MCHAR*,vector<FaceEx *> >::iterator it=sameMatMap.begin();
			it!=sameMatMap.end();++it)
		{
			DumpFaces(gM,it->second,gSkin,pGameNode,it->first);
		}
	}

	void CountMesh( IGameObject * obj) 
	{
		_ObjInfoList.at(_ObjCount-1).StartMesh=_MeshCount;
		IGameMesh * gM = (IGameMesh*)obj;
		if(gM->InitializeData())
		{
			Tab<int> matIDs=gM->GetActiveMatIDs();
			int matCount=matIDs.Count();

			hash_map<MCHAR*,vector<FaceEx *> > sameMatMap;

			int tmpMatCount=_MtlCount;

			for (int m=0;m<matCount;++m)
			{
				Tab<FaceEx *> faces=gM->GetFacesFromMatID(matIDs[m]);
				IGameMaterial * mat=gM->GetMaterialFromFace(faces[0]);
				CoutMtl(mat);

				if (_LimitBoneNumPerMesh)
				{
					vector<FaceEx *>& faceList=sameMatMap[mat->GetMaterialName()];
					for (int f=0;f<faces.Count();++f)
					{
						faceList.push_back(faces[f]);
					}
				}
			}

			if (_LimitBoneNumPerMesh)
			{
				int numMod = obj->GetNumModifiers();
				if(numMod > 0)
				{
					for(int i=0;i<numMod;i++)
					{
						IGameModifier * gMod = obj->GetIGameModifier(i);
						if (gMod->IsSkin())
						{
							for (hash_map<MCHAR*,vector<FaceEx *> >::iterator it=sameMatMap.begin();
								it!=sameMatMap.end();++it)
							{
								SplitMesh(((IGameSkin*)gMod)->GetInitialPose(),it->second,(IGameSkin*)gMod);//((IGameSkin*)gMod)->GetInitialPose()gM//
							}
							break;;
						}
					}
				}
			}
			else
			{
				if (tmpMatCount<_MtlCount)
					tmpMatCount=_MtlCount-tmpMatCount;
				else
					tmpMatCount=1;
				_MeshCount+=tmpMatCount;
			}

			_ObjInfoList.at(_ObjCount-1).MeshCount=_MeshCount-_ObjInfoList.at(_ObjCount-1).StartMesh;
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

	void DumpFaces( IGameMesh * gM,vector<FaceEx *>& faces, IGameSkin* gSkin,IGameNode* pGameNode,MCHAR* matName) 
	{
		vector<VertexInfo> vertList;
		vector<TriInfo> triList;
		hash_set<MCHAR*> sameBoneMap;
		int oldBoneNum=0;
		int faceCount=(int)faces.size();
		int weightIndex=0;
		int usedFaceNum=0;
		for (int f=0;f<faceCount;++f)
		{
			oldBoneNum=(int)sameBoneMap.size();

			TriInfo tri;
			for (int i=0;i<3;++i)
			{
				VertexInfo info;
				info.VertIndex=faces[f]->vert[i];
				info.TexCoordIndex=faces[f]->texCoord[i];

				int vertCount=(int)vertList.size();
				int v=0;
				for (;v<vertCount;++v)
				{
					VertexInfo& curinfo=vertList.at(v);
					if (info.VertIndex==curinfo.VertIndex)
					{
						if (info.TexCoordIndex==curinfo.TexCoordIndex)
						{
							tri.Index[i]=v;
							break;
						}
						else
						{
							VertexUV(info, gM);

							if (info.UV.Equals(curinfo.UV))
							{
								tri.Index[i]=v;
								break;
							}

							info.WeightIndex=curinfo.WeightIndex;
							info.WeightCount=curinfo.WeightCount;
						}
					}
					else
					{
						VertexPos(gM, info);

						if (info.Pos.Equals(curinfo.Pos))
						{
							if (info.TexCoordIndex==curinfo.TexCoordIndex)
							{
								tri.Index[i]=v;
								break;
							}
							else
							{
								VertexUV(info, gM);

								if (info.UV.Equals(curinfo.UV))
								{
									tri.Index[i]=v;
									break;
								}

								info.WeightIndex=curinfo.WeightIndex;
								info.WeightCount=curinfo.WeightCount;
							}
						}
					}
				}
				if (v==vertCount)
				{
					VertexUV(info, gM);
					VertexPos(gM, info);

					if (-1==info.WeightIndex&&-1==info.WeightCount)
					{
						info.WeightIndex=weightIndex;
						info.WeightCount=gSkin->GetNumberOfBones(info.VertIndex);
						
						weightIndex+=info.WeightCount>VERT_MAX_BONES?VERT_MAX_BONES:info.WeightCount;

						VertexWeight(info, gSkin, gM);
					}

					vertList.push_back(info);

					tri.Index[i]=(int)vertList.size()-1;

					for (int i=0;i<(int)info.Weights.size();++i)
					{
						sameBoneMap.insert(info.Weights.at(i).Bone->GetName());
					}
				}
			}
			triList.push_back(tri);

			if (_LimitBoneNumPerMesh)
			{
				usedFaceNum++;
				if (oldBoneNum<_MaxBonePerMesh&&
					(int)sameBoneMap.size()>=_MaxBonePerMesh)
				{
					DumpVertex(pGameNode,matName,vertList, usedFaceNum, triList, weightIndex, (int)sameBoneMap.size());
					sameBoneMap.clear();
					vertList.clear();
					triList.clear();
					weightIndex=0;
					usedFaceNum=0;
				}
			}
		}
		if (_LimitBoneNumPerMesh)
			faceCount=usedFaceNum;
		if ((int)sameBoneMap.size()>0)
			DumpVertex(pGameNode,matName,vertList, faceCount, triList, weightIndex, (int)sameBoneMap.size());
	}

	void DumpVertex(IGameNode* pGameNode,MCHAR* matName, vector<VertexInfo> &vertList, int faceCount,
		vector<TriInfo> &triList, int weightIndex,int boneNum) 
	{
		fprintf(_OutFile,"mesh {\r\n");
		fprintf(_OutFile,"\t// meshes: %s\r\n",pGameNode->GetName());
		if (!_DoomVersion)
		{
			fprintf(_OutFile,"\tmeshindex %d\r\n\r\n",_MeshCount++);
		}
		fprintf(_OutFile,"\tshader \"%s\"\r\n\r\n",matName);

		fprintf(_OutFile,"\tnumverts %d\r\n",vertList.size());

		int vertCount=(int)vertList.size();
		for (int v=0;v<vertCount;++v)
		{
			VertexInfo& curinfo=vertList.at(v);

			fprintf(_OutFile,"\tvert %d ( %f %f ) %d %d\r\n",v,
				curinfo.UV.x,curinfo.UV.y,
				curinfo.WeightIndex,curinfo.WeightCount>VERT_MAX_BONES?VERT_MAX_BONES:curinfo.WeightCount);
		}

		fprintf(_OutFile,"\r\n\tnumtris %d\r\n",faceCount);

		int triCount=(int)triList.size();
		for (int t=0;t<triCount;++t)
		{
			TriInfo& tri=triList.at(t);

			fprintf(_OutFile,"\ttri %d %d %d %d\r\n",t,
				tri.Index[0],
				tri.Index[2],
				tri.Index[1]);
		}

		fprintf(_OutFile,"\r\n\tnumweights %d\r\n",weightIndex);

		for (int v=0;v<vertCount;++v)
		{
			VertexInfo& curinfo=vertList.at(v);

			int weightCount=(int)curinfo.Weights.size();
			for (int u=0;u<weightCount;++u)
			{
				WeightInfo& weight=curinfo.Weights.at(u);

				fprintf(_OutFile,"\tweight %d %d %f ( %f %f %f )\r\n",curinfo.WeightIndex+u,
					_BoneIndexMap[weight.Bone->GetName()],
					weight.Value,
					weight.Offset.x,
					weight.Offset.y,
					weight.Offset.z);
			}
		}
		fprintf(_OutFile,"\r\n\t//bones %d\r\n",boneNum);

		fprintf(_OutFile,"}\r\n\r\n");
	}


	void VertexPos( IGameMesh * gM, VertexInfo &info ) 
	{
		info.Pos=gM->GetVertex(info.VertIndex);
	}


	void VertexUV( VertexInfo &info, IGameMesh * gM ) 
	{
		info.UV=gM->GetTexVertex(info.TexCoordIndex);
		if (info.UV.x>1.f)
			info.UV.x=1.f;
		if (info.UV.y<0.f)
			info.UV.y=-info.UV.y;
		else
			info.UV.y=1.f-info.UV.y;
	}


	void VertexWeight( VertexInfo &info, IGameSkin* gSkin, IGameMesh * gM ) 
	{
		int weightCount=info.WeightCount;
		WeightInfo weights[VERT_MAX_BONES]={WeightInfo()};

		for (int b=0;b<weightCount;++b)
		{
			float curWeight=gSkin->GetWeight(info.VertIndex,b);

			if (b<VERT_MAX_BONES)
			{
				weights[b].Value=curWeight;
				weights[b].Bone=gSkin->GetIGameBone(info.VertIndex,b);
			}
			else 
			{
				for (int u=0;u<VERT_MAX_BONES;++u)
				{
					if (weights[u].Value<curWeight)
					{
						weights[u].Value=curWeight;
						weights[u].Bone=gSkin->GetIGameBone(info.VertIndex,b);
						break;
					}
				}
			}
		}

		float totalWeight=0;
		if (weightCount>VERT_MAX_BONES)
			weightCount=VERT_MAX_BONES;
		for (int u=0;u<weightCount;++u)
		{
			totalWeight+=weights[u].Value;
		}
		for (int u=0;u<weightCount;++u)
		{
			weights[u].Value/=totalWeight;

			GMatrix initMat;
			if (!gSkin->GetInitBoneTM(weights[u].Bone,initMat))
			{
				initMat=weights[u].Bone->GetWorldTM(_TvToDump);
			}

			initMat=initMat.Inverse();
			weights[u].Offset=info.Pos*initMat;
			info.Weights.push_back(weights[u]);
		}
	}

	void SplitMesh( IGameMesh * gM, vector<FaceEx *>& faces, IGameSkin* gSkin ) 
	{
		hash_set<IGameNode*> boneMap;
		int meshBone=0;
		int faceCount=(int)faces.size();
		for (int f=0;f<faceCount;++f)
		{
			meshBone=(int)boneMap.size();
			for (int i=0;i<3;++i)
			{
				int weightCount=gSkin->GetNumberOfBones(faces[f]->vert[i]);
				WeightInfo weights[VERT_MAX_BONES]={WeightInfo()};

				for (int b=0;b<weightCount;++b)
				{
					float curWeight=gSkin->GetWeight(faces[f]->vert[i],b);

					if (b<VERT_MAX_BONES)
					{
						weights[b].Value=curWeight;
						weights[b].Bone=gSkin->GetIGameBone(faces[f]->vert[i],b);
					}
					else 
					{
						for (int u=0;u<VERT_MAX_BONES;++u)
						{
							if (weights[u].Value<curWeight)
							{
								weights[u].Value=curWeight;
								weights[u].Bone=gSkin->GetIGameBone(faces[f]->vert[i],b);
								break;
							}
						}
					}
				}

				if (weightCount>VERT_MAX_BONES)
					weightCount=VERT_MAX_BONES;
				
				for (int u=0;u<weightCount;++u)
				{
					boneMap.insert(weights[u].Bone);
				}
			}
			
			if (meshBone<_MaxBonePerMesh&&
				(int)boneMap.size()>=_MaxBonePerMesh)
			{
				_MeshCount++;
				boneMap.clear();
			}
		}
		if ((int)boneMap.size()>0)
		{
			_MeshCount++;
		}
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

			CheckDlgButton(hWnd, IDC_CHECK_LIMITBONENUMPERMESH, imp->_LimitBoneNumPerMesh);
			SetDlgItemInt(hWnd,IDC_EDIT_LIMITBONENUMPERMESH,imp->_MaxBonePerMesh,FALSE);
			CheckDlgButton(hWnd,IDC_DOMM_VERSION,imp->_DoomVersion);
			CheckDlgButton(hWnd, IDC_COPY_IMAGES, imp->_CopyImages);
			CheckDlgButton(hWnd, IDC_COMPRESS, imp->_Compress);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
			case IDC_OK:
				imp->_MaxBonePerMesh=GetDlgItemInt(hWnd,IDC_EDIT_LIMITBONENUMPERMESH,NULL,FALSE);
				imp->_LimitBoneNumPerMesh = IsDlgButtonChecked(hWnd, IDC_CHECK_LIMITBONENUMPERMESH);
				imp->_DoomVersion = IsDlgButtonChecked(hWnd, IDC_DOMM_VERSION);
				imp->_CopyImages = IsDlgButtonChecked(hWnd, IDC_COPY_IMAGES);
				imp->_Compress = IsDlgButtonChecked(hWnd, IDC_COMPRESS);
				EndDialog(hWnd, 1);
				return TRUE;
			case IDC_CANCEL:
				EndDialog(hWnd, 0);
				return TRUE;
			case IDC_CHECK_LIMITBONENUMPERMESH:
				return TRUE;
			case IDC_EDIT_LIMITBONENUMPERMESH:
				return TRUE;
			case IDC_DOMM_VERSION:
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
_Compress(FALSE),
_LimitBoneNumPerMesh(TRUE),
_MaxBonePerMesh(36),
_DoomVersion(FALSE)
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
	return 120;
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
			cm->SetCoordSystem(IGameConversionManager::IGAME_MAX);
			_TargetExport=(options & SCENE_EXPORT_SELECTED) ? true : false;
			pIgame->InitialiseIGame();
			pIgame->SetStaticFrame(0);

			_TvToDump=0;

			SimpleFile outFile(name,"wb");
			_OutFile=outFile.File();

			result=SaveMd5Mesh(ei,i);

			MessageBox( GetActiveWindow(), name, _T("md5mesh finish"), 0 );

			pIgame->ReleaseIGame();
			_OutFile=NULL;
		}
		else
			result=TRUE;
	}
	return result;
}

int md5meshExporter::SaveMd5Mesh( ExpInterface *ei, Interface *gi )
{
	if (!_DoomVersion)
		fprintf(_OutFile,"MD5Version 4843\r\ncommandline \"by HoneyCat md5meshExporter v%d\"\r\n\r\n",Version());
	else
		fprintf(_OutFile,"MD5Version 10\r\ncommandline \"by HoneyCat md5meshExporter v%d\"\r\n\r\n",Version());

	DumpCount();

	DumpBones();

	if (!_DoomVersion)
	{
		DumpObjects();

		DumpMaterials();
	}

	DumpMeshes();

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

	fprintf(_OutFile,"numJoints %d\r\n",_BoneCount+1);
	fprintf(_OutFile,"numMeshes %d\r\n",_MeshCount);
	if (!_DoomVersion)
	{
		fprintf(_OutFile,"numObjects %d\r\n",_ObjCount);
		fprintf(_OutFile,"numMaterials %d\r\n\r\n",_MtlCount);
	}
}

void md5meshExporter::CountNodes( IGameNode * pGameNode,BOOL isBoneChild )
{
	IGameObject * obj = pGameNode->GetIGameObject();
	if (obj->GetIGameType()==IGameObject::IGAME_BONE&&
		obj->GetMaxObject()->SuperClassID()!=HELPER_CLASS_ID)
	{
		_BoneCount++;
		isBoneChild=TRUE;
	}
	else if (!pGameNode->IsNodeHidden()&&
		obj->GetIGameType()==IGameObject::IGAME_MESH&&
		!isBoneChild)
	{
		_ObjCount++;

		ObjectInfo info;
		info.Name=pGameNode->GetName();
		_ObjInfoList.push_back(info);

		CountMesh(obj);
	}
	//fprintf(_OutFile,"%s %d\n",pGameNode->GetName(),obj->GetIGameType());
	pGameNode->ReleaseIGameObject();

	for(int i=0;i<pGameNode->GetChildCount();i++)
	{
		IGameNode * child = pGameNode->GetNodeChild(i);

		CountNodes(child,isBoneChild);
	}
}

void md5meshExporter::DumpBones() 
{
	fprintf(_OutFile,"joints {\r\n");

	fprintf(_OutFile,"\t\"%s\" %d ( %f %f %f ) ( %f %f %f )\r\n","origin",-1,
		0.f,0.f,0.f,0.f,0.f,0.f);

	for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
	{
		IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
		
		int index=1;
		DumpJoint(pGameNode,index,0);
	}
	fprintf(_OutFile,"}\r\n\r\n");
}

void md5meshExporter::CoutMtl( IGameMaterial* pGameMtl )
{
	int m=0;
	TSTR mtlName=pGameMtl->GetMaterialName();
	for (;m<_MtlCount;++m)
	{
		TSTR curName=_MtlInfoList.at(m).Name;
		if (mtlName==curName)
		{
			break;
		}
	}

	if (m==_MtlCount)
	{
		MaterialInfo mtlInfo;
		mtlInfo.Name=pGameMtl->GetMaterialName();

		int texCount = pGameMtl->GetNumberOfTextureMaps();
		for(int i=0;i<texCount;i++)
		{
			IGameTextureMap * tex = pGameMtl->GetIGameTextureMap(i);
			MCHAR * name = tex->GetBitmapFileName();
			switch (tex->GetStdMapSlot())
			{
			case ID_DI:
				mtlInfo.Diffuse=name;
				break;
			case ID_OP:
				mtlInfo.Opacity=name;
				break;
			}
		}
		
		_MtlInfoList.push_back(mtlInfo);
		_MtlCount++;
	}
}



