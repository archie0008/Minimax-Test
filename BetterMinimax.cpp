#include "BetterMinimax.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

struct BMOffset { int dx; int dy; };

static const std::vector<BMOffset>& GetOffsets(int radius, int angleDeg)
{
    thread_local std::vector<BMOffset> offsets;
    offsets.clear();
    if (radius <= 0) { offsets.push_back({0, 0}); return offsets; }

    int a = angleDeg % 360;
    if (a < 0) a += 360;
    const double r = a * 3.14159265358979323846 / 180.0;
    const double c = std::cos(r);
    const double sn = std::sin(r);
    offsets.reserve((2 * radius + 1) * (2 * radius + 1));

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const double lx = dx * c + dy * sn;
            const double ly = -dx * sn + dy * c;
            if (std::fabs(lx) <= radius + 1e-9 && std::fabs(ly) <= radius + 1e-9)
                offsets.push_back({dx, dy});
        }
    }
    return offsets;
}

static inline int ClampI(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
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

    // CC 2015-era Skeleton path: 8/16 bpc.
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
                 2, 1, STR(StrID_Operation_Choices), OPERATION_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_SLIDER(STR(StrID_Softness_Param_Name),
                  SOFTNESS_MIN, SOFTNESS_MAX, SOFTNESS_MIN, SOFTNESS_MAX,
                  SOFTNESS_DFLT, SOFTNESS_DISK_ID);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(STR(StrID_Channel_Param_Name),
                 3, 1, STR(StrID_Channel_Choices), CHANNEL_DISK_ID);

    out_data->num_params = BETTERMINIMAX_NUM_PARAMS;
    return PF_Err_NONE;
}

template <typename PixelT, typename ChannelT>
static inline ChannelT channel_max_value();

template <>
inline A_u_char channel_max_value<PF_Pixel8, A_u_char>() { return PF_MAX_CHAN8; }

template <>
inline A_u_short channel_max_value<PF_Pixel16, A_u_short>() { return PF_MAX_CHAN16; }

template <typename PixelT, typename ChannelT>
static void ProcessWorld(const PF_EffectWorld *src, PF_EffectWorld *dst,
                         int width, int height, int radius, int angle,
                         int operation, int softness, int channel)
{
    const ChannelT maxv = channel_max_value<PixelT, ChannelT>();
    const float blend = std::max(0.0f, std::min(1.0f, softness / 100.0f));
    const char *src_base = reinterpret_cast<const char *>(src->data);
    char *dst_base = reinterpret_cast<char *>(dst->data);
    const std::vector<BMOffset>& offsets = GetOffsets(radius, angle);

    for (int y = 0; y < height; ++y) {
        const PixelT *src_row =
            reinterpret_cast<const PixelT *>(src_base + y * src->rowbytes);
        PixelT *dst_row =
            reinterpret_cast<PixelT *>(dst_base + y * dst->rowbytes);

        for (int x = 0; x < width; ++x) {
            const PixelT original = src_row[x];
            PixelT best = original;

            if (operation == OP_MAX) {
                best.red = best.green = best.blue = 0;
                best.alpha = 0;
            } else {
                best.red = best.green = best.blue = maxv;
                best.alpha = maxv;
            }

            for (const BMOffset& off : offsets) {
                const int yy = ClampI(y + off.dy, 0, height - 1);
                const PixelT *row =
                    reinterpret_cast<const PixelT *>(src_base + yy * src->rowbytes);
                const int xx = ClampI(x + off.dx, 0, width - 1);
                const PixelT &p = row[xx];

                    if (channel == CH_RGB || channel == CH_RGBA) {
                        if (operation == OP_MAX) {
                            best.red = std::max(best.red, p.red);
                            best.green = std::max(best.green, p.green);
                            best.blue = std::max(best.blue, p.blue);
                        } else {
                            best.red = std::min(best.red, p.red);
                            best.green = std::min(best.green, p.green);
                            best.blue = std::min(best.blue, p.blue);
                        }
                    }

                    if (channel == CH_ALPHA || channel == CH_RGBA) {
                        if (operation == OP_MAX)
                            best.alpha = std::max(best.alpha, p.alpha);
                        else
                            best.alpha = std::min(best.alpha, p.alpha);
                    }
                }

            if (blend > 0.0f) {
                PixelT result = original;
                if (channel == CH_RGB || channel == CH_RGBA) {
                    result.red = static_cast<ChannelT>(
                        original.red * (1.0f - blend) + best.red * blend + 0.5f);
                    result.green = static_cast<ChannelT>(
                        original.green * (1.0f - blend) + best.green * blend + 0.5f);
                    result.blue = static_cast<ChannelT>(
                        original.blue * (1.0f - blend) + best.blue * blend + 0.5f);
                }
                if (channel == CH_ALPHA || channel == CH_RGBA) {
                    result.alpha = static_cast<ChannelT>(
                        original.alpha * (1.0f - blend) + best.alpha * blend + 0.5f);
                }
                dst_row[x] = result;
            } else {
                // Preserve channels that weren't selected.
                PixelT result = original;
                if (channel == CH_RGB || channel == CH_RGBA) {
                    result.red = best.red;
                    result.green = best.green;
                    result.blue = best.blue;
                }
                if (channel == CH_ALPHA || channel == CH_RGBA)
                    result.alpha = best.alpha;
                dst_row[x] = result;
            }
        }
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
    const int softness = static_cast<int>(params[BETTERMINIMAX_SOFTNESS]->u.sd.value);
    const int channel = static_cast<int>(params[BETTERMINIMAX_CHANNEL]->u.pd.value);

    if (PF_WORLD_IS_DEEP(output)) {
        ProcessWorld<PF_Pixel16, A_u_short>(
            src, output, width, height, radius, angle, operation, softness, channel);
    } else {
        ProcessWorld<PF_Pixel8, A_u_char>(
            src, output, width, height, radius, angle, operation, softness, channel);
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
