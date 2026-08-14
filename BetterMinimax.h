#pragma once
#ifndef BETTERMINIMAX_H
#define BETTERMINIMAX_H

#include "AEConfig.h"

#ifdef AE_OS_WIN
    typedef unsigned short PixelType;
    #include <Windows.h>
#endif

#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#include "BetterMinimax_Strings.h"

#define MAJOR_VERSION 1
#define MINOR_VERSION 1
#define BUG_VERSION 0
#define STAGE_VERSION PF_Stage_DEVELOP
#define BUILD_VERSION 2

#define RADIUS_MIN 0
#define RADIUS_MAX 64
#define RADIUS_DFLT 8

#define ANGLE_MIN 0
#define ANGLE_MAX 1800
#define ANGLE_DFLT 0

#define SOFTNESS_MIN 0
#define SOFTNESS_MAX 100
#define SOFTNESS_DFLT 0

enum {
    BETTERMINIMAX_INPUT = 0,
    BETTERMINIMAX_RADIUS,
    BETTERMINIMAX_ANGLE,
    BETTERMINIMAX_OPERATION,
    BETTERMINIMAX_SOFTNESS,
    BETTERMINIMAX_CHANNEL,
    BETTERMINIMAX_NUM_PARAMS
};

enum {
    RADIUS_DISK_ID = 1,
    ANGLE_DISK_ID,
    OPERATION_DISK_ID,
    SOFTNESS_DISK_ID,
    CHANNEL_DISK_ID
};

enum {
    OP_MAX = 1,
    OP_MIN = 2
};

enum {
    CH_RGB = 1,
    CH_ALPHA = 2,
    CH_RGBA = 3
};

#ifdef __cplusplus
extern "C" {
#endif

DllExport PF_Err
EntryPointFunc(
    PF_Cmd cmd,
    PF_InData *in_data,
    PF_OutData *out_data,
    PF_ParamDef *params[],
    PF_LayerDef *output,
    void *extra);

#ifdef __cplusplus
}
#endif

#endif
