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


enum {
    BETTERMINIMAX_INPUT = 0,
    BETTERMINIMAX_RADIUS,
    BETTERMINIMAX_ANGLE,
    BETTERMINIMAX_OPERATION,
    BETTERMINIMAX_MAP,
    BETTERMINIMAX_CHANNEL,
    BETTERMINIMAX_NUM_PARAMS
};

enum {
    RADIUS_DISK_ID = 1,
    ANGLE_DISK_ID,
    OPERATION_DISK_ID,
    MAP_DISK_ID,
    CHANNEL_DISK_ID
};

enum {
    MAP_BOTH = 1,
    MAP_HORIZONTAL = 2,
    MAP_VERTICAL = 3
};

enum {
    OP_MAX = 1,
    OP_MIN = 2,
    OP_MIN_THEN_MAX = 3,
    OP_MAX_THEN_MIN = 4
};

enum {
    CH_COLOR = 1,
    CH_ALPHA_COLOR = 2,
    CH_RED = 3,
    CH_GREEN = 4,
    CH_BLUE = 5,
    CH_ALPHA = 6
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
