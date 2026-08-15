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
    { StrID_Operation_Choices, "Minimum|Maximum|Minimum Then Maximum|Maximum Then Minimum" },
    { StrID_Map_Param_Name, "Map" },
    { StrID_Map_Choices, "Both|Horizontal|Vertical" },
    { StrID_Channel_Param_Name, "Channel" },
    { StrID_Channel_Choices, "Color|Alpha and Color|Red|Green|Blue|Alpha" }
};

char *GetStringPtr(int strNum)
{
    return g_strs[strNum].str;
}
