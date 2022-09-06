/* swinumbers.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/*
 * Title:       swinumbers.h - SWI numbers for Brazil, Arthur & RISC OS
 * Author:      Stuart K. Swales 31-Jan-1989
*/

#ifndef __swinumbers_h
#define __swinumbers_h

/* Mask with SWI number to make error return with V set - Arthur feature */

#define XOS_MASK 0x00020000


/* Brazil and Arthur SWIs */

#define OS_WriteC                       0x0
#define OS_WriteS                       0x1
#define OS_Write0                       0x2
#define OS_NewLine                      0x3
#define OS_ReadC                        0x4
#define OS_CLI                          0x5
#define OS_Byte                         0x6
#define OS_Word                         0x7
#define OS_File                         0x8
#define OS_Args                         0x9
#define OS_BGet                         0xA
#define OS_BPut                         0xB
#define OS_GBPB                         0xC
#define OS_Find                         0xD
#define OS_ReadLine                     0xE
#define OS_Control                      0xF
#define OS_GetEnv                       0x10
#define OS_Exit                         0x11
#define OS_SetEnv                       0x12
#define OS_IntOn                        0x13
#define OS_IntOff                       0x14
#define OS_CallBack                     0x15
#define OS_EnterOS                      0x16
#define OS_BreakPt                      0x17
#define OS_BreakCtrl                    0x18
#define OS_UnusedSWI                    0x19
#define OS_UpdateMEMC                   0x1A
#define OS_SetCallBack                  0x1B
#define OS_Mouse                        0x1C


/* Arthur OS SWIs */

#define OS_Heap                         0x1D
#define OS_Module                       0x1E
#define OS_Claim                        0x1F
#define OS_Release                      0x20
#define OS_ReadUnsigned                 0x21
#define OS_GenerateEvent                0x22
#define OS_ReadVarVal                   0x23
#define OS_SetVarVal                    0x24
#define OS_GSInit                       0x25
#define OS_GSRead                       0x26
#define OS_GSTrans                      0x27
#define OS_BinaryToDecimal              0x28
#define OS_FSControl                    0x29
#define OS_ChangeDynamicArea            0x2A
#define OS_GenerateError                0x2B
#define OS_ReadEscapeState              0x2C
#define OS_EvaluateExpression           0x2D
#define OS_SpriteOp                     0x2E
#define OS_ReadPalette                  0x2F
#define OS_ServiceCall                  0x30
#define OS_ReadVduVariables             0x31
#define OS_ReadPoint                    0x32
#define OS_UpCall                       0x33
#define OS_CallAVector                  0x34
#define OS_ReadModeVariable             0x35
#define OS_RemoveCursors                0x36
#define OS_RestoreCursors               0x37
#define OS_SWINumberToString            0x38
#define OS_SWINumberFromString          0x39
#define OS_ValidateAddress              0x3A
#define OS_CallAfter                    0x3B
#define OS_CallEvery                    0x3C
#define OS_RemoveTickerEvent            0x3D
#define OS_InstallKeyHandler            0x3E
#define OS_CheckModeValid               0x3F
#define OS_ChangeEnvironment            0x40
#define OS_ClaimScreenMemory            0x41
#define OS_ReadMonotonicTime            0x42
#define OS_SubstituteArgs               0x43
#define OS_PrettyPrint                  0x44
#define OS_Plot                         0x45
#define OS_WriteN                       0x46


/* RISC OS SWIs */

#define OS_AddToVector                  0x47
#define OS_WriteEnv                     0x48
#define OS_ReadArgs                     0x49
#define OS_ReadRAMFsLimits              0x4A
#define OS_ClaimDeviceVector            0x4B
#define OS_ReleaseDeviceVector          0x4C
#define OS_DelinkApplication            0x4D
#define OS_RelinkApplication            0x4E
#define OS_HeapSort                     0x4F
#define OS_ExitAndDie                   0x50
#define OS_ReadMemMapInfo               0x51
#define OS_ReadMemMapEntries            0x52
#define OS_SetMemMapEntries             0x53
#define OS_AddCallBack                  0x54
#define OS_ReadDefaultHandler           0x55
#define OS_SetECFOrigin                 0x56
#define OS_SerialOp                     0x57
#define OS_ReadSysInfo                  0x58
#define OS_Confirm                      0x59
#define OS_ChangedBox                   0x5A
#define OS_CRC                          0x5B
#define OS_ReadDynamicArea              0x5C
#define OS_PrintChar                    0x5D

#define OS_ConvertStandardDateAndTime   0xC0
#define OS_ConvertDateAndTime           0xC1
#define OS_ConvertHex1                  0xD0
#define OS_ConvertHex2                  0xD1
#define OS_ConvertHex4                  0xD2
#define OS_ConvertHex6                  0xD3
#define OS_ConvertHex8                  0xD4
#define OS_ConvertCardinal1             0xD5
#define OS_ConvertCardinal2             0xD6
#define OS_ConvertCardinal3             0xD7
#define OS_ConvertCardinal4             0xD8
#define OS_ConvertInteger1              0xD9
#define OS_ConvertInteger2              0xDA
#define OS_ConvertInteger3              0xDB
#define OS_ConvertInteger4              0xDC
#define OS_ConvertBinary1               0xDD
#define OS_ConvertBinary2               0xDE
#define OS_ConvertBinary3               0xDF
#define OS_ConvertBinary4               0xE0
#define OS_ConvertSpacedCardinal1       0xE1
#define OS_ConvertSpacedCardinal2       0xE2
#define OS_ConvertSpacedCardinal3       0xE3
#define OS_ConvertSpacedCardinal4       0xE4
#define OS_ConvertSpacedInteger1        0xE5
#define OS_ConvertSpacedInteger2        0xE6
#define OS_ConvertSpacedInteger3        0xE7
#define OS_ConvertSpacedInteger4        0xE8
#define OS_ConvertFixedNetStation       0xE9
#define OS_ConvertNetStation            0xEA
#define OS_ConvertFixedFileSize         0xEB
#define OS_ConvertFileSize              0xEC


#define OS_WriteI                       0x100

#define OS_UserSWI                      0x200


#define IIC_Control                     0x240


/* System Extension SWIs */

#define Econet_CreateReceive            0x40000
#define Econet_ExamineReceive           0x40001
#define Econet_ReadReceive              0x40002
#define Econet_AbandonReceive           0x40003
#define Econet_WaitForReception         0x40004
#define Econet_EnumerateReceive         0x40005
#define Econet_StartTransmit            0x40006
#define Econet_PollTransmit             0x40007
#define Econet_AbandonTransmit          0x40008
#define Econet_DoTransmit               0x40009
#define Econet_ReadLocalStationAndNet   0x4000A
#define Econet_ConvertStatusToString    0x4000B
#define Econet_ConvertStatusToError     0x4000C
#define Econet_ReadProtection           0x4000D
#define Econet_SetProtection            0x4000E
#define Econet_ReadStationNumber        0x4000F
#define Econet_PrintBanner              0x40010
#define Econet_017                      0x40011
#define Econet_ReleasePort              0x40012
#define Econet_AllocatePort             0x40013
#define Econet_DeAllocatePort           0x40014
#define Econet_ClaimPort                0x40015
#define Econet_StartImmediate           0x40016
#define Econet_DoImmediate              0x40017

#define NetFS_ReadFSNumber              0x40040
#define NetFS_SetFSNumber               0x40041
#define NetFS_ReadFSName                0x40042
#define NetFS_SetFSName                 0x40043
#define NetFS_ReadCurrentContext        0x40044
#define NetFS_SetCurrentContext         0x40045
#define NetFS_ReadFSTimeouts            0x40046
#define NetFS_SetFSTimeouts             0x40047
#define NetFS_DoFSOp                    0x40048
#define NetFS_EnumerateFSList           0x40049
#define NetFS_EnumerateFS               0x4004A
#define NetFS_ConvertDate               0x4004B
#define NetFS_DoFSOpToGivenFS           0x4004C

#define Font_CacheAddr                  0x40080
#define Font_FindFont                   0x40081
#define Font_LoseFont                   0x40082
#define Font_ReadDefn                   0x40083
#define Font_ReadInfo                   0x40084
#define Font_StringWidth                0x40085
#define Font_Paint                      0x40086
#define Font_Caret                      0x40087
#define Font_ConverttoOS                0x40088
#define Font_Converttopoints            0x40089
#define Font_SetFont                    0x4008A
#define Font_CurrentFont                0x4008B
#define Font_FutureFont                 0x4008C
#define Font_FindCaret                  0x4008D
#define Font_CharBBox                   0x4008E
#define Font_ReadScaleFactor            0x4008F
#define Font_SetScaleFactor             0x40090
#define Font_ListFonts                  0x40091
#define Font_SetFontColours             0x40092
#define Font_SetPalette                 0x40093
#define Font_ReadThresholds             0x40094
#define Font_SetThresholds              0x40095
#define Font_FindCaretJ                 0x40096
#define Font_StringBBox                 0x40097
#define Font_ReadColourTable            0x40098

#define Wimp_Initialise                 0x400C0
#define Wimp_CreateWindow               0x400C1
#define Wimp_CreateIcon                 0x400C2
#define Wimp_DeleteWindow               0x400C3
#define Wimp_DeleteIcon                 0x400C4
#define Wimp_OpenWindow                 0x400C5
#define Wimp_CloseWindow                0x400C6
#define Wimp_Poll                       0x400C7
#define Wimp_RedrawWindow               0x400C8
#define Wimp_UpdateWindow               0x400C9
#define Wimp_GetRectangle               0x400CA
#define Wimp_GetWindowState             0x400CB
#define Wimp_GetWindowInfo              0x400CC
#define Wimp_SetIconState               0x400CD
#define Wimp_GetIconState               0x400CE
#define Wimp_GetPointerInfo             0x400CF
#define Wimp_DragBox                    0x400D0
#define Wimp_ForceRedraw                0x400D1
#define Wimp_SetCaretPosition           0x400D2
#define Wimp_GetCaretPosition           0x400D3
#define Wimp_CreateMenu                 0x400D4
#define Wimp_DecodeMenu                 0x400D5
#define Wimp_WhichIcon                  0x400D6
#define Wimp_SetExtent                  0x400D7
#define Wimp_SetPointerShape            0x400D8
#define Wimp_OpenTemplate               0x400D9
#define Wimp_CloseTemplate              0x400DA
#define Wimp_LoadTemplate               0x400DB
#define Wimp_ProcessKey                 0x400DC
#define Wimp_CloseDown                  0x400DD
#define Wimp_StartTask                  0x400DE
#define Wimp_ReportError                0x400DF
#define Wimp_GetWindowOutline           0x400E0
#define Wimp_PollIdle                   0x400E1
#define Wimp_PlotIcon                   0x400E2
#define Wimp_SetMode                    0x400E3
#define Wimp_SetPalette                 0x400E4
#define Wimp_ReadPalette                0x400E5
#define Wimp_SetColour                  0x400E6
#define Wimp_SendMessage                0x400E7
#define Wimp_CreateSubMenu              0x400E8
#define Wimp_SpriteOp                   0x400E9
#define Wimp_BaseOfSprites              0x400EA
#define Wimp_BlockCopy                  0x400EB
#define Wimp_SlotSize                   0x400EC
#define Wimp_ReadPixTrans               0x400ED
#define Wimp_ClaimFreeMemory            0x400EE
#define Wimp_CommandWindow              0x400EF
#define Wimp_TextColour                 0x400F0
#define Wimp_TransferBlock              0x400F1
#define Wimp_ReadSysInfo                0x400F2
#define Wimp_SetFontColours             0x400F3

#define Sound_Configure                 0x40140
#define Sound_Enable                    0x40141
#define Sound_Stereo                    0x40142
#define Sound_Speaker                   0x40143
#define Sound_Volume                    0x40180
#define Sound_SoundLog                  0x40181
#define Sound_LogScale                  0x40182
#define Sound_InstallVoice              0x40183
#define Sound_RemoveVoice               0x40184
#define Sound_AttachVoice               0x40185
#define Sound_ControlPacked             0x40186
#define Sound_Tuning                    0x40187
#define Sound_Pitch                     0x40188
#define Sound_Control                   0x40189
#define Sound_AttachNamedVoice          0x4018A
#define Sound_ReadControlBlock          0x4018B
#define Sound_WriteControlBlock         0x4018C

#define Sound_QInit                     0x401C0
#define Sound_QSchedule                 0x401C1
#define Sound_QRemove                   0x401C2
#define Sound_QFree                     0x401C3
#define Sound_QSDispatch                0x401C4
#define Sound_QTempo                    0x401C5
#define Sound_QBeat                     0x401C6
#define Sound_QInterface                0x401C7

#define NetPrint_ReadPSNumber           0x40200
#define NetPrint_SetPSNumber            0x40201
#define NetPrint_ReadPSName             0x40202
#define NetPrint_SetPSName              0x40203
#define NetPrint_ReadPSTimeouts         0x40204
#define NetPrint_SetPSTimeouts          0x40205

#define ADFS_DiscOp                     0x40240
#define ADFS_HDC                        0x40241
#define ADFS_Drives                     0x40242
#define ADFS_FreeSpace                  0x40243
#define ADFS_Retries                    0x40244
#define ADFS_DescribeDisc               0x40245

#define Podule_ReadID                   0x40280
#define Podule_ReadHeader               0x40281
#define Podule_EnumerateChunks          0x40282
#define Podule_ReadChunk                0x40283
#define Podule_ReadBytes                0x40284
#define Podule_WriteBytes               0x40285
#define Podule_CallLoader               0x40286
#define Podule_RawRead                  0x40287
#define Podule_RawWrite                 0x40288
#define Podule_HardwareAddress          0x40289

#define WaveSynth_Load                  0x40300

#define Debugger_Disassemble            0x40380

#define FPEmulator_Version              0x40480

#define FileCore_DiscOp                 0x40540
#define FileCore_Create                 0x40541
#define FileCore_Drives                 0x40542
#define FileCore_FreeSpace              0x40543
#define FileCore_FloppyStructure        0x40544
#define FileCore_DescribeDisc           0x40545

#define Shell_Create                    0x405C0
#define Shell_Destroy                   0x405C1

#define Hourglass_On                    0x406C0
#define Hourglass_Off                   0x406C1
#define Hourglass_Smash                 0x406C2
#define Hourglass_Start                 0x406C3
#define Hourglass_Percentage            0x406C4
#define Hourglass_LEDs                  0x406C5

#define Draw_ProcessPath                0x40700
#define Draw_ProcessPathFP              0x40701
#define Draw_Fill                       0x40702
#define Draw_FillFP                     0x40703
#define Draw_Stroke                     0x40704
#define Draw_StrokeFP                   0x40705
#define Draw_StrokePath                 0x40706
#define Draw_StrokePathFP               0x40707
#define Draw_FlattenPath                0x40708
#define Draw_FlattenPathFP              0x40709
#define Draw_TransformPath              0x4070A
#define Draw_TransformPathFP            0x4070B

#define RamFS_DiscOp                    0x40780
#define RamFS_NOP                       0x40781
#define RamFS_Drives                    0x40782
#define RamFS_FreeSpace                 0x40783
#define RamFS_NOP2                      0x40784
#define RamFS_DescribeDisc              0x40785

#endif  /* __swinumbers_h */

/* end of swinumbers.h */
