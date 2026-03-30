#pragma once

#include <QString>

namespace Compliance
{

    // Language code lookups
    QString languageNameFromCode(const QString &isoCode);
    QString isoCodeFromLanguageName(const QString &name);

    // Common Blu-ray audio channel layouts
    constexpr int STEREO_CHANNELS = 2;
    constexpr int SURROUND_5_1_CHANNELS = 6;
    constexpr int SURROUND_7_1_CHANNELS = 8;

    // Check if audio channel count is Blu-ray compliant
    inline bool isCompliantAudioChannels(int channels)
    {
        return channels == 1 || channels == 2 || channels == 6 || channels == 8;
    }

    // Check if audio codec is compliant (AC3, E-AC3, DTS, DTS-HD, LPCM, MPEG Audio, AAC)
    bool isCompliantAudioCodec(const QString &codecName);

    // Check if video codec is compliant (H.264 / AVC only in MVP)
    inline bool isCompliantVideoCodec(const QString &codecName)
    {
        return codecName == "h264" || codecName == "mpeg4"; // mpeg4 supported but non-standard
    }

    // Check if video resolution is Blu-ray standard
    inline bool isCompliantResolution(int width, int height)
    {
        return (width == 1920 && height == 1080)    // Full HD
               || (width == 1280 && height == 720); // HD
    }

    // Check if framerate is compliant (23.976, 24, 25, 29.97, 30 fps)
    bool isCompliantFramerate(const QString &fpsStr);

    // Check if video pixel format is compliant (8-bit YUV420p)
    inline bool isCompliantPixelFormat(const QString &format)
    {
        return format == "yuv420p";
    }

    // Check if subtitle codec is Blu-ray compatible
    bool isCompliantSubtitleCodec(const QString &codecName);

}
