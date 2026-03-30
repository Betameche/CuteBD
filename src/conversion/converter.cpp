#include "converter.hpp"
#include "common.hpp"
#include "toolchecker.hpp"
#include "compliance.hpp"
#include <QDebug>
#include <QStringList>
#include <cmath>

namespace Conversion
{

    bool Converter::convertStream(
        const StreamInfo &stream,
        const QString &inputFile,
        int streamIndex,
        const QString &outputFile,
        std::function<void(const QString &)> onProgress)
    {
        // Check if ffmpeg is available
        auto ffmpeg = Tools::checkTool(Tools::FFMPEG);
        if (!ffmpeg.available)
        {
            qWarning() << "ffmpeg not available:" << ffmpeg.reason;
            return false;
        }

        QString command = generateFFmpegCommand(stream, inputFile, streamIndex, outputFile);

        if (command.isEmpty())
        {
            qWarning() << "Failed to generate ffmpeg command";
            return false;
        }

        // Parse command into executable and args
        QStringList parts = command.split(' ', Qt::SkipEmptyParts);
        if (parts.isEmpty())
        {
            return false;
        }

        QString exe = parts.first();
        QStringList args = parts.mid(1);

        // Run conversion
        auto result = Common::runCommand(exe, args, onProgress);

        if (!result.success)
        {
            qWarning() << "ffmpeg conversion failed:" << result.error;
            if (!result.stdErr.isEmpty())
            {
                qWarning() << result.stdErr;
            }
        }

        return result.success;
    }

    QString Converter::generateFFmpegCommand(
        const StreamInfo &stream,
        const QString &inputFile,
        int streamIndex,
        const QString &outputFile)
    {
        // Base command
        QStringList cmd;
        cmd << "ffmpeg" << "-i" << inputFile;

        // Stream selection and conversion depends on type
        switch (stream.type)
        {
        case StreamType::Video:
        {
            auto params = getVideoParams(stream);

            // Select video stream and convert
            cmd << "-map" << QString("0:v:%1").arg(streamIndex);
            cmd << "-c:v" << params.codec;
            cmd << "-preset" << params.preset;
            cmd << "-crf" << QString::number(params.crf);
            cmd << "-pix_fmt" << params.pixFmt;

            // Copy audio and subtitles as-is
            cmd << "-map" << "0:a?" << "-c:a" << "copy";
            cmd << "-map" << "0:s?" << "-c:s" << "copy";

            break;
        }

        case StreamType::Audio:
        {
            auto params = getAudioParams(stream);

            // Select audio stream and convert
            cmd << "-map" << QString("0:a:%1").arg(streamIndex);
            cmd << "-c:a" << params.codec;
            cmd << "-b:a" << params.bitrate;

            // Copy video and subtitles
            cmd << "-map" << "0:v?" << "-c:v" << "copy";
            cmd << "-map" << "0:s?" << "-c:s" << "copy";

            break;
        }

        case StreamType::Subtitle:
        {
            // For subtitles, we'll use ffmpeg extraction to SRT or other format
            // Full PGS encoding requires external tools
            cmd << "-map" << QString("0:s:%1").arg(streamIndex);
            cmd << "-c:s" << "subrip"; // Convert to SRT as intermediate

            break;
        }

        default:
            return "";
        }

        // Output options
        cmd << "-y" << outputFile; // -y: overwrite output

        return cmd.join(' ');
    }

    Converter::VideoConversionParams Converter::getVideoParams(const StreamInfo &stream)
    {
        VideoConversionParams params;

        // Adaptive parameters based on current codec and properties
        if (stream.codecName == "h264" && stream.pixelFormat == "yuv420p")
        {
            // Already mostly compliant, use lighter settings
            params.preset = "fast";
            params.crf = 20; // Slightly higher quality preservation
        }
        else
        {
            // Non-compliant video, use stricter settings for quality
            params.crf = 18;
        }

        return params;
    }

    Converter::AudioConversionParams Converter::getAudioParams(const StreamInfo &stream)
    {
        AudioConversionParams params;

        // Adjust bitrate based on channel count
        if (stream.channels >= 6)
        {
            params.bitrate = "640k"; // 5.1 or 7.1 surround
        }
        else if (stream.channels == 1)
        {
            params.bitrate = "192k"; // Mono
        }
        else
        {
            params.bitrate = "320k"; // Stereo
        }

        return params;
    }

}
