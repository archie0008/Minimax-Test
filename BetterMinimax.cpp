#include "BetterMinimax.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

struct BMOffset { int dx; int dy; };

static const std::vector<BMOffset>& GetOffsets(int radius, int angleDeg, int mapMode)
{
    thread_local std::vector<BMOffset> offsets;
    offsets.clear();
    if (radius <= 0) { offsets.push_back({0, 0}); return offsets; }

    int a = angleDeg % 360;
    if (a < 0) a += 360;
    const double radians = a * 3.14159265358979323846 / 180.0;
    const double c = std::cos(radians);
    const double s = std::sin(radians);

    if (mapMode == MAP_HORIZONTAL || mapMode == MAP_VERTICAL) {
        offsets.reserve(2 * radius + 1);

        for (int i = -radius; i <= radius; ++i) {
            const double lx = (mapMode == MAP_HORIZONTAL) ? static_cast<double>(i) : 0.0;
            const double ly = (mapMode == MAP_HORIZONTAL) ? 0.0 : static_cast<double>(i);

            const int dx = static_cast<int>(std::lround(lx * c - ly * s));
            const int dy = static_cast<int>(std::lround(lx * s + ly * c));
            offsets.push_back({dx, dy});
        }
        return offsets;
    }

    offsets.reserve((2 * radius + 1) * (2 * radius + 1));
    for (int ly = -radius; ly <= radius; ++ly) {
        for (int lx = -radius; lx <= radius; ++lx) {
            const int dx = static_cast<int>(std::lround(lx * c - ly * s));
            const int dy = static_cast<int>(std::lround(lx * s + ly * c));
            offsets.push_back({dx, dy});
        }
    }
    return offsets;
}

static inline int ClampI(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

template <typename PixelT, typename ChannelT>
static inline ChannelT channel_max_value();

template <>
inline A_u_char channel_max_value<PF_Pixel8, A_u_char>() { return PF_MAX_CHAN8; }

template <>
inline A_u_short channel_max_value<PF_Pixel16, A_u_short>() { return PF_MAX_CHAN16; }

template <typename PixelT>
static inline bool SelectsAlpha(int channel)
{
    return channel == CH_ALPHA_COLOR || channel == CH_ALPHA;
}

template <typename PixelT, typename ChannelT>
static void ProcessPass(const PixelT *src_base, int src_stride,
                        PixelT *dst_base, int dst_stride,
                        int width, int height, int radius, int angle,
                        int operation, int mapMode, int channel)
{
    const ChannelT maxv = channel_max_value<PixelT, ChannelT>();
    const std::vector<BMOffset>& offsets = GetOffsets(radius, angle, mapMode);

    for (int y = 0; y < height; ++y) {
        const PixelT *src_row = src_base + y * src_stride;
        PixelT *dst_row = dst_base + y * dst_stride;

        for (int x = 0; x < width; ++x) {
            const PixelT original = src_row[x];
            PixelT best = original;

            if (operation == OP_MAX) {
                const bool color = (channel == CH_COLOR || channel == CH_ALPHA_COLOR);
                if (color) {
                    best.red = best.green = best.blue = 0;
                } else if (channel == CH_RED) {
                    best.red = 0;
                } else if (channel == CH_GREEN) {
                    best.green = 0;
                } else if (channel == CH_BLUE) {
                    best.blue = 0;
                }
                if (SelectsAlpha<PixelT>(channel))
                    best.alpha = 0;
            } else {
                const bool color = (channel == CH_COLOR || channel == CH_ALPHA_COLOR);
                if (color) {
                    best.red = best.green = best.blue = maxv;
                } else if (channel == CH_RED) {
                    best.red = maxv;
                } else if (channel == CH_GREEN) {
                    best.green = maxv;
                } else if (channel == CH_BLUE) {
                    best.blue = maxv;
                }
                if (SelectsAlpha<PixelT>(channel))
                    best.alpha = maxv;
            }

            for (const BMOffset& off : offsets) {
                const int yy = ClampI(y + off.dy, 0, height - 1);
                const PixelT *row = src_base + yy * src_stride;
                const int xx = ClampI(x + off.dx, 0, width - 1);
                const PixelT &p = row[xx];

                if (channel == CH_COLOR || channel == CH_ALPHA_COLOR) {
                    if (operation == OP_MAX) {
                        best.red = std::max(best.red, p.red);
                        best.green = std::max(best.green, p.green);
                        best.blue = std::max(best.blue, p.blue);
                    } else {
                        best.red = std::min(best.red, p.red);
                        best.green = std::min(best.green, p.green);
                        best.blue = std::min(best.blue, p.blue);
                    }
                } else if (channel == CH_RED) {
                    best.red = (operation == OP_MAX) ? std::max(best.red, p.red) : std::min(best.red, p.red);
                } else if (channel == CH_GREEN) {
                    best.green = (operation == OP_MAX) ? std::max(best.green, p.green) : std::min(best.green, p.green);
                } else if (channel == CH_BLUE) {
                    best.blue = (operation == OP_MAX) ? std::max(best.blue, p.blue) : std::min(best.blue, p.blue);
                }

                if (SelectsAlpha<PixelT>(channel)) {
                    best.alpha = (operation == OP_MAX)
                        ? std::max(best.alpha, p.alpha)
                        : std::min(best.alpha, p.alpha);
                }
            }

            PixelT result = original;
            if (channel == CH_COLOR || channel == CH_ALPHA_COLOR) {
                result.red = best.red;
                result.green = best.green;
                result.blue = best.blue;
            } else if (channel == CH_RED) {
                result.red = best.red;
            } else if (channel == CH_GREEN) {
                result.green = best.green;
            } else if (channel == CH_BLUE) {
                result.blue = best.blue;
            }
            if (SelectsAlpha<PixelT>(channel))
                result.alpha = best.alpha;

            dst_row[x] = result;
        }
    }
}

static PF_Err About(PF_InData *in_data, PF_OutData *out_data,
                    PF_ParamDef *params[], PF_LayerDef *output)
{
    AEGP_SuiteHandler suites(in_data->pica_basicP);
    suites.ANSICallbacksSuite1()->sprintf(
        out_data->return_msg,
        "%s v%d.%d\r%s",
        STR(StrID_Name), MAJOR_VERSION, MINOR_VERSION,
        STR(StrID_Description));
    return PF_Err_NONE;
}

static PF_Err GlobalSetup(PF_InData *in_data, PF_OutData *out_data,
                          PF_ParamDef *params[], PF_LayerDef *output)
{
    out_data->my_version = PF_VERSION(MAJOR_VERSION, MINOR_VERSION,
                                      BUG_VERSION, STAGE_VERSION, BUILD_VERSION);
    out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;
    out_data->out_flags2 = 0;
    return PF_Err_NONE;
}

static PF_Err ParamsSetup(PF_InData *in_data, PF_OutData *out_data,
                          PF_ParamDef *params[], PF_LayerDef *output)
{
    PF_ParamDef def;
    AEFX_CLR_STRUCT(def);

    PF_ADD_SLIDER(STR(StrID_Radius_Param_Name),
                  RADIUS_MIN, RADIUS_MAX, RADIUS_MIN, RADIUS_MAX,
                  RADIUS_DFLT, RADIUS_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_ANGLE(STR(StrID_Angle_Param_Name), ANGLE_DFLT, ANGLE_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(STR(StrID_Operation_Param_Name),
                 4, 1, STR(StrID_Operation_Choices), OPERATION_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(STR(StrID_Map_Param_Name),
                 3, 1, STR(StrID_Map_Choices), MAP_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(STR(StrID_Channel_Param_Name),
                 6, 1, STR(StrID_Channel_Choices), CHANNEL_DISK_ID);

    out_data->num_params = BETTERMINIMAX_NUM_PARAMS;
    return PF_Err_NONE;
}

template <typename PixelT, typename ChannelT>
static void ProcessWorld(const PF_EffectWorld *src, PF_EffectWorld *dst,
                         int width, int height, int radius, int angle,
                         int operation, int mapMode, int channel)
{
    const PixelT *src_base = reinterpret_cast<const PixelT *>(src->data);
    PixelT *dst_base = reinterpret_cast<PixelT *>(dst->data);
    const int src_stride = src->rowbytes / static_cast<int>(sizeof(PixelT));
    const int dst_stride = dst->rowbytes / static_cast<int>(sizeof(PixelT));

    if (operation == OP_MIN_THEN_MAX || operation == OP_MAX_THEN_MIN) {
        std::vector<PixelT> temp(static_cast<size_t>(width) * static_cast<size_t>(height));
        const int firstOp = (operation == OP_MIN_THEN_MAX) ? OP_MIN : OP_MAX;
        const int secondOp = (operation == OP_MIN_THEN_MAX) ? OP_MAX : OP_MIN;

        ProcessPass<PixelT, ChannelT>(
            src_base, src_stride, temp.data(), width,
            width, height, radius, angle, firstOp, mapMode, channel);

        ProcessPass<PixelT, ChannelT>(
            temp.data(), width, dst_base, dst_stride,
            width, height, radius, angle, secondOp, mapMode, channel);
    } else {
        ProcessPass<PixelT, ChannelT>(
            src_base, src_stride, dst_base, dst_stride,
            width, height, radius, angle, operation, mapMode, channel);
    }
}

static PF_Err Render(PF_InData *in_data, PF_OutData *out_data,
                     PF_ParamDef *params[], PF_LayerDef *output)
{
    const PF_EffectWorld *src = &params[BETTERMINIMAX_INPUT]->u.ld;
    const int width = output->width;
    const int height = output->height;
    const int radius = static_cast<int>(params[BETTERMINIMAX_RADIUS]->u.sd.value);
    const int angle = static_cast<int>(params[BETTERMINIMAX_ANGLE]->u.ad.value / 65536);
    const int operation = static_cast<int>(params[BETTERMINIMAX_OPERATION]->u.pd.value);
    const int mapMode = static_cast<int>(params[BETTERMINIMAX_MAP]->u.pd.value);
    const int channel = static_cast<int>(params[BETTERMINIMAX_CHANNEL]->u.pd.value);

    if (PF_WORLD_IS_DEEP(output)) {
        ProcessWorld<PF_Pixel16, A_u_short>(
            src, output, width, height, radius, angle, operation, mapMode, channel);
    } else {
        ProcessWorld<PF_Pixel8, A_u_char>(
            src, output, width, height, radius, angle, operation, mapMode, channel);
    }

    return PF_Err_NONE;
}

DllExport PF_Err EntryPointFunc(PF_Cmd cmd, PF_InData *in_data,
                                PF_OutData *out_data, PF_ParamDef *params[],
                                PF_LayerDef *output, void *extra)
{
    PF_Err err = PF_Err_NONE;

    try {
        switch (cmd) {
            case PF_Cmd_ABOUT:
                err = About(in_data, out_data, params, output);
                break;
            case PF_Cmd_GLOBAL_SETUP:
                err = GlobalSetup(in_data, out_data, params, output);
                break;
            case PF_Cmd_PARAMS_SETUP:
                err = ParamsSetup(in_data, out_data, params, output);
                break;
            case PF_Cmd_RENDER:
                err = Render(in_data, out_data, params, output);
                break;
            default:
                break;
        }
    } catch (PF_Err &thrown_err) {
        err = thrown_err;
    }
    return err;
}
