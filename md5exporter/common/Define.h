#ifndef __Define_h__
#define __Define_h__

#define VERT_MAX_BONES 4

enum eHierarchyFlag
{
	eHierarchyFlag_None=0,
	eHierarchyFlag_Pos_X=0x00000001,
	eHierarchyFlag_Pos_Y=0x00000002,
	eHierarchyFlag_Pos_Z=0x00000004,
	eHierarchyFlag_Rot_X=0x00000008,
	eHierarchyFlag_Rot_Y=0x00000010,
	eHierarchyFlag_Rot_Z=0x00000020,
};


#endif // __Define_h__
