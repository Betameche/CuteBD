#pragma once

#include "models.hpp"
#include <QString>
#include <QVector>
#include <functional>

namespace Conversion
{

    // Conversion job for a single stream
    struct ConversionJob
    {
        int fileIndex = -1;
        int streamIndex = -1;
        QString inputFile;
        QString outputFile;
        StreamInfo stream;
        bool completed = false;
        QString errorMessage;
    };

    // Ffmpeg-based stream converter
    // Handles video, audio, and subtitle conversions using ffmpeg
    class Converter
    {
    public:
        Converter() = default;

        // Convert a single stream using ffmpeg
        // Returns true on success, false on failure
        bool convertStream(
            const StreamInfo &stream,
            const QString &inputFile,
            int streamIndex,
            const QString &outputFile,
            std::function<void(const QString &)> onProgress = nullptr);

        // Generate ffmpeg conversion command for a stream
        QString generateFFmpegCommand(
            const StreamInfo &stream,
            const QString &inputFile,
            int streamIndex,
            const QString &outputFile);

    private:
        // Conversion parameters for each stream type
        struct VideoConversionParams
        {
            QString codec = "libx264";
            QString preset = "slow";
            int crf = 18;
            QString pixFmt = "yuv420p";
        };

        struct AudioConversionParams
        {
            QString codec = "ac3";
            QString bitrate = "640k";
        };

        VideoConversionParams getVideoParams(const StreamInfo &stream);
        AudioConversionParams getAudioParams(const StreamInfo &stream);
    };

}
