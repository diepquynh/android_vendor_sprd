# Microsoft Developer Studio Project File - Name="omxil" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=omxil - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "omxil.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "omxil.mak" CFG="omxil - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "omxil - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "omxil - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "omxil - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f omxil.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "omxil.exe"
# PROP BASE Bsc_Name "omxil.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Cmd_Line "make"
# PROP Rebuild_Opt "/a"
# PROP Target_File "omxil.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "omxil - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f omxil.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "omxil.exe"
# PROP BASE Bsc_Name "omxil.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Cmd_Line "make"
# PROP Rebuild_Opt "/a"
# PROP Target_File "omxil.exe"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "omxil - Win32 Release"
# Name "omxil - Win32 Debug"

!IF  "$(CFG)" == "omxil - Win32 Release"

!ELSEIF  "$(CFG)" == "omxil - Win32 Debug"

!ENDIF 

# Begin Group "src"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\common.c
# End Source File
# Begin Source File

SOURCE=.\src\common.h
# End Source File
# Begin Source File

SOURCE=.\src\component_loader.h
# End Source File
# Begin Source File

SOURCE=.\src\content_pipe_file.c
# End Source File
# Begin Source File

SOURCE=.\src\content_pipe_file.h
# End Source File
# Begin Source File

SOURCE=.\src\content_pipe_inet.c
# End Source File
# Begin Source File

SOURCE=.\src\content_pipe_inet.h
# End Source File
# Begin Source File

SOURCE=.\src\extension_struct.h
# End Source File
# Begin Source File

SOURCE=.\src\omx_comp_debug_levels.h
# End Source File
# Begin Source File

SOURCE=.\src\omx_create_loaders.h
# End Source File
# Begin Source File

SOURCE=.\src\omx_create_loaders_linux.c
# End Source File
# Begin Source File

SOURCE=.\src\omx_reference_resource_manager.c
# End Source File
# Begin Source File

SOURCE=.\src\omx_reference_resource_manager.h
# End Source File
# Begin Source File

SOURCE=.\src\omxcore.c
# End Source File
# Begin Source File

SOURCE=.\src\omxcore.h
# End Source File
# Begin Source File

SOURCE=.\src\omxregister.c
# End Source File
# Begin Source File

SOURCE=.\src\queue.c
# End Source File
# Begin Source File

SOURCE=.\src\queue.h
# End Source File
# Begin Source File

SOURCE=.\src\st_static_component_loader.c
# End Source File
# Begin Source File

SOURCE=.\src\st_static_component_loader.h
# End Source File
# Begin Source File

SOURCE=.\src\tsemaphore.c
# End Source File
# Begin Source File

SOURCE=.\src\tsemaphore.h
# End Source File
# Begin Source File

SOURCE=.\src\utils.c
# End Source File
# Begin Source File

SOURCE=.\src\utils.h
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\OMX_Audio.h
# End Source File
# Begin Source File

SOURCE=.\include\OMX_Component.h
# End Source File
# Begin Source File

SOURCE=.\include\OMX_ContentPipe.h
# End Source File
# Begin Source File

SOURCE=.\include\OMX_Core.h
# End Source File
# Begin Source File

SOURCE=.\include\OMX_Image.h
# End Source File
# Begin Source File

SOURCE=.\include\OMX_Index.h
# End Source File
# Begin Source File

SOURCE=.\include\OMX_IVCommon.h
# End Source File
# Begin Source File

SOURCE=.\include\OMX_Other.h
# End Source File
# Begin Source File

SOURCE=.\include\OMX_Types.h
# End Source File
# Begin Source File

SOURCE=.\include\OMX_Video.h
# End Source File
# End Group

# Begin Source File

SOURCE=.\Android.mk
# End Source File
# End Target
# End Project
