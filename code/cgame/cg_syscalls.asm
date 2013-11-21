code

equ trap_Print							-1
equ trap_Error							-2
equ trap_Milliseconds					-3
equ trap_Cvar_Register					-4
equ trap_Cvar_Update					-5
equ trap_Cvar_Set						-6
equ trap_Cvar_VariableStringBuffer		-7
equ trap_Cvar_CheckRange				-8
equ trap_Argc							-9
equ trap_Argv							-10
equ trap_Args							-11
equ trap_FS_FOpenFile					-12
equ trap_FS_Read						-13
equ trap_FS_Write						-14
equ trap_FS_FCloseFile					-15
equ trap_SendConsoleCommand				-16
equ trap_AddCommand						-17
equ trap_SendClientCommand				-18
equ trap_UpdateScreen					-19
equ trap_CM_LoadMap						-20
equ trap_CM_NumInlineModels				-21
equ trap_CM_InlineModel					-22
equ trap_CM_LoadModel					-23
equ trap_CM_TempBoxModel				-24
equ trap_CM_PointContents				-25
equ trap_CM_TransformedPointContents	-26
equ trap_CM_BoxTrace					-27
equ trap_CM_TransformedBoxTrace			-28
equ trap_CM_MarkFragments				-29
equ trap_S_StartSound					-30
equ trap_S_StartLocalSound				-31
equ trap_S_ClearLoopingSounds			-32
equ trap_S_AddLoopingSound				-33
equ trap_S_UpdateEntityPosition			-34
equ trap_S_Respatialize					-35
equ trap_S_RegisterSound				-36
equ trap_S_StartBackgroundTrack			-37
equ trap_R_LoadWorldMap					-38
equ trap_R_RegisterModel				-39
equ trap_R_RegisterSkin					-40
equ trap_R_RegisterShader				-41
equ trap_R_ClearScene					-42
equ trap_R_AddRefEntityToScene			-43
equ trap_R_AddPolyToScene				-44
equ trap_R_AddLightToScene				-45
equ trap_R_RenderScene					-46
equ trap_R_SetColor						-47
equ trap_R_DrawStretchPic				-48
equ trap_R_ModelBounds					-49
equ trap_R_LerpTag						-50
equ trap_GetGlconfig					-51
equ trap_GetGameState					-52
equ trap_GetCurrentSnapshotNumber		-53
equ trap_GetSnapshot					-54
equ trap_GetServerCommand				-55
equ trap_GetCurrentCmdNumber			-56
equ trap_GetUserCmd						-57
equ trap_SetUserCmdValue				-58
equ trap_R_RegisterShaderNoMip			-59
equ trap_MemoryRemaining				-60
equ trap_R_RegisterFont					-61
equ trap_Key_IsDown						-62
equ trap_Key_GetCatcher					-63
equ trap_Key_SetCatcher					-64
equ trap_Key_GetKey						-65
equ trap_PC_AddGlobalDefine				-66
equ trap_PC_LoadSource					-67
equ trap_PC_FreeSource					-68
equ trap_PC_ReadToken					-69
equ trap_PC_SourceFileAndLine			-70
equ trap_S_StopBackgroundTrack			-71
equ trap_RealTime						-72
equ trap_SnapVector						-73
equ trap_RemoveCommand					-74
equ trap_R_LightForPoint				-75
equ trap_CIN_PlayCinematic				-76
equ trap_CIN_StopCinematic				-77
equ trap_CIN_RunCinematic 				-78
equ trap_CIN_DrawCinematic				-79
equ trap_CIN_SetExtents					-80
equ trap_R_RemapShader					-81
equ trap_S_AddRealLoopingSound			-82
equ trap_S_StopLoopingSound				-83
equ trap_CM_TempCapsuleModel			-84
equ trap_CM_CapsuleTrace				-85
equ trap_CM_TransformedCapsuleTrace		-86
equ trap_R_AddAdditiveLightToScene		-87
equ trap_GetEntityToken					-88
equ trap_R_AddPolysToScene				-89
equ trap_R_inPVS						-90
equ trap_FS_Seek						-91

equ memset						-101
equ memcpy						-102
equ strncpy						-103
equ sin							-104
equ cos							-105
equ atan2						-106
equ sqrt						-107
equ floor						-108
equ ceil						-109
equ testPrintInt				-110
equ testPrintFloat				-111
equ acos						-112

