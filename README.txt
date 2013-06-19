3ds max export md5 file format,*.md5mesh and *.md5anim.

support 2012 and 2011

///////////////////////////////////////////////////////////
//downloads
///////////////////////////////////////////////////////////
binary plugin file 2011 and 2012,32 and x64.

///////////////////////////////////////////////////////////
//build
///////////////////////////////////////////////////////////
debug and release complie for 2012.
debug2011 and release2011 complie for 2011.
before build 2011 version,need add a system environment variable "ADSK_3DSMAX_2011_SDK=<2011 sdk
root>".
because "3DSMAX_2011_SDK_PATH" first chat is number,can't use in
vs2010.

///////////////////////////////////////////////////////////
//debug3dmax
///////////////////////////////////////////////////////////
this is a wraper dle, use to read 'debugplugin.txt' get real plugin path, dynamic load dle and do export.
in this way DON'T NEED CLOSE 3DS MAX WHEN YOU REBUILD EXPORTER PLUGIN.

how to use?
put 'debugplugin.txt' to export directory.
