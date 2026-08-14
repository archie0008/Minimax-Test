#include "BetterMinimax.h"

typedef struct {
    A_u_long index;
    A_char str[256];
} TableString;

TableString g_strs[StrID_NUMTYPES] = {
    { StrID_NONE, "" },
    { StrID_Name, "BetterMinimax" },
    { StrID_Description, "As the name suggests." },
    { StrID_Radius_Param_Name, "Radius" },
    { StrID_Angle_Param_Name, "Direction" },
    { StrID_Operation_Param_Name, "Operation" },
    { StrID_Operation_Choices, "Maximum|Minimum" },
    { StrID_Softness_Param_Name, "Softness" },
    { StrID_Channel_Param_Name, "Channel" },
    { StrID_Channel_Choices, "RGB|Alpha|RGBA" }
};

char *GetStringPtr(int strNum)
{
    return g_strs[strNum].str;
}
