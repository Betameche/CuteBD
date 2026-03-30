#include "inspector.hpp"
#include "common.hpp"
#include "toolchecker.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace Inspection
{

    QVector<StreamInfo> Inspector::inspectFile(const QString &filePath, QString &errorOut)
    {
        QVector<StreamInfo> result;

        // Check if file exists
        if (!Common::fileExists(filePath))
        {
            errorOut = QString("File not found: %1").arg(filePath);
            return result;
        }

        // Check if ffprobe is available
        auto ffprobe = Tools::checkTool(Tools::FFPROBE);
        if (!ffprobe.available)
        {
            errorOut = QString("ffprobe not available: %1").arg(ffprobe.reason);
            return result;
        }

        // Run ffprobe with JSON output
        QStringList args;
        args << "-v" << "error"
             << "-show_format"
             << "-show_streams"
             << "-of" << "json"
             << filePath;

        auto procResult = Common::runCommand(ffprobe.path, args);

        if (!procResult.success)
        {
            errorOut = QString("ffprobe failed: %1").arg(procResult.error);
            if (!procResult.stdErr.isEmpty())
            {
                errorOut += QString(" (%1)").arg(procResult.stdErr);
            }
            return result;
        }

        // Parse JSON output
        result = parseFFprobeOutput(procResult.stdOut);

        // Validate each stream for Blu-ray compliance
        for (auto &stream : result)
        {
            validateStreamCompliance(stream);
        }

        return result;
    }

    void Inspector::validateStreamCompliance(StreamInfo &stream)
    {
        // Determine compliance based on stream type and properties
        switch (stream.type)
        {
        case StreamType::Video:
            validateVideoCompliance(stream);
            break;
        case StreamType::Audio:
            validateAudioCompliance(stream);
            break;
        case StreamType::Subtitle:
            validateSubtitleCompliance(stream);
            break;
        default:
            stream.compliance = ComplianceStatus::NonCompliant;
            stream.complianceNote = "Unknown stream type";
        }
    }

    QVector<StreamInfo> Inspector::parseFFprobeOutput(const QString &jsonOutput)
    {
        QVector<StreamInfo> result;

        QJsonDocument doc = QJsonDocument::fromJson(jsonOutput.toUtf8());
        if (!doc.isObject())
        {
            qWarning() << "Invalid JSON from ffprobe";
            return result;
        }

        QJsonObject root = doc.object();
        QJsonArray streams = root.value("streams").toArray();

        for (const auto &streamVal : streams)
        {
            if (!streamVal.isObject())
                continue;

            StreamInfo info = createStreamInfo(streamVal.toObject());
            result.append(info);
        }

        return result;
    }

    StreamInfo Inspector::createStreamInfo(const QJsonObject &streamObj)
    {
        StreamInfo info;

        info.index = streamObj.value("index").toInt(-1);

        // Determine stream type
        QString codecType = streamObj.value("codec_type").toString();
        if (codecType == "video")
        {
            info.type = StreamType::Video;
        }
        else if (codecType == "audio")
        {
            info.type = StreamType::Audio;
        }
        else if (codecType == "subtitle")
        {
            info.type = StreamType::Subtitle;
        }
        else
        {
            info.type = StreamType::Unknown;
            return info;
        }

        // Common properties
        info.codecName = streamObj.value("codec_name").toString();
        info.language = streamObj.value("tags").toObject().value("language").toString("und");
        if (info.language.isEmpty())
        {
            info.language = "und";
        }
        info.title = streamObj.value("tags").toObject().value("title").toString();

        // Video-specific
        if (info.type == StreamType::Video)
        {
            info.codecProfile = streamObj.value("profile").toString();
            info.width = streamObj.value("width").toInt(0);
            info.height = streamObj.value("height").toInt(0);
            info.pixelFormat = streamObj.value("pix_fmt").toString();

            // Frame rate (can be in format "24/1" or "24")
            QString rFrameRate = streamObj.value("r_frame_rate").toString();
            if (!rFrameRate.isEmpty())
            {
                if (rFrameRate.contains('/'))
                {
                    QStringList parts = rFrameRate.split('/');
                    if (parts.size() == 2)
                    {
                        double num = parts[0].toDouble();
                        double denom = parts[1].toDouble();
                        info.frameRate = QString::number(denom > 0 ? num / denom : 0, 'f', 3);
                    }
                }
                else
                {
                    info.frameRate = rFrameRate;
                }
            }

            info.level = streamObj.value("level").toInt(0);
            info.bitRate = streamObj.value("bit_rate").toString().toLongLong(0);
        }

        // Audio-specific
        if (info.type == StreamType::Audio)
        {
            info.channels = streamObj.value("channels").toInt(0);
            info.sampleRate = streamObj.value("sample_rate").toString().toInt(0);
        }

        // Subtitle-specific
        if (info.type == StreamType::Subtitle)
        {
            info.subFormat = detectSubtitleFormat(info.codecName);
        }

        return info;
    }

    SubtitleFormat Inspector::detectSubtitleFormat(const QString &codecName)
    {
        QString lower = codecName.toLower();

        if (lower.contains("subrip"))
            return SubtitleFormat::SRT;
        if (lower.contains("ass"))
            return SubtitleFormat::ASS;
        if (lower.contains("ssa"))
            return SubtitleFormat::SSA;
        if (lower.contains("webvtt"))
            return SubtitleFormat::VTT;
        if (lower.contains("pgs") || lower.contains("hdmv"))
            return SubtitleFormat::PGS;
        if (lower.contains("dvb"))
            return SubtitleFormat::DVB;
        if (lower.contains("vobsub"))
            return SubtitleFormat::VOBSUB;

        return SubtitleFormat::Unknown;
    }

    void Inspector::validateVideoCompliance(StreamInfo &stream)
    {
        QStringList issues;

        // Check codec
        if (!Compliance::isCompliantVideoCodec(stream.codecName))
        {
            issues << QString("Codec %1 not H.264").arg(stream.codecName);
        }

        // Check resolution
        if (!Compliance::isCompliantResolution(stream.width, stream.height))
        {
            issues << QString("Resolution %1x%2 not standard (1920x1080 or 1280x720 required)")
                          .arg(stream.width)
                          .arg(stream.height);
        }

        // Check framerate
        if (!Compliance::isCompliantFramerate(stream.frameRate))
        {
            issues << QString("Framerate %1 not standard Blu-ray").arg(stream.frameRate);
        }

        // Check pixel format
        if (!Compliance::isCompliantPixelFormat(stream.pixelFormat))
        {
            issues << QString("Pixel format %1 not 8-bit YUV420p").arg(stream.pixelFormat);
        }

        if (issues.isEmpty())
        {
            stream.compliance = ComplianceStatus::Compliant;
        }
        else
        {
            stream.compliance = ComplianceStatus::NonCompliant;
            stream.complianceNote = issues.join("; ");
        }
    }

    void Inspector::validateAudioCompliance(StreamInfo &stream)
    {
        QStringList issues;

        // Check codec
        if (!Compliance::isCompliantAudioCodec(stream.codecName))
        {
            issues << QString("Codec %1 not Blu-ray compatible").arg(stream.codecName);
        }

        // Check channels
        if (!Compliance::isCompliantAudioChannels(stream.channels))
        {
            issues << QString("Channel count %1 not standard (1, 2, 6, or 8)").arg(stream.channels);
        }

        // Check sample rate (Blu-ray: 48 kHz mostly, some support 96 kHz)
        if (stream.sampleRate != 48000 && stream.sampleRate != 96000)
        {
            issues << QString("Sample rate %1 Hz not standard Blu-ray").arg(stream.sampleRate);
        }

        if (issues.isEmpty())
        {
            stream.compliance = ComplianceStatus::Compliant;
        }
        else
        {
            stream.compliance = ComplianceStatus::NonCompliant;
            stream.complianceNote = issues.join("; ");
        }
    }

    void Inspector::validateSubtitleCompliance(StreamInfo &stream)
    {
        if (!Compliance::isCompliantSubtitleCodec(stream.codecName))
        {
            stream.compliance = ComplianceStatus::NonCompliant;
            stream.complianceNote = QString("Codec %1 requires conversion (target: PGS)")
                                        .arg(stream.codecName);
        }
        else if (stream.subFormat == SubtitleFormat::PGS || stream.subFormat == SubtitleFormat::DVB)
        {
            stream.compliance = ComplianceStatus::Compliant;
        }
        else
        {
            // Text subtitles are convertible but need processing
            stream.compliance = ComplianceStatus::Warning;
            stream.complianceNote = QString("Text subtitle format %1 - will be converted to PGS")
                                        .arg(stream.codecName);
        }
    }

}
