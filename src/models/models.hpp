#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>

// Compliance status for a stream or file
enum class ComplianceStatus
{
    Compliant,   // Fully Blu-ray compatible
    Warning,     // Acceptable but not ideal (e.g., lower channels, non-standard fps)
    NonCompliant // Not Blu-ray compatible without conversion
};

// Conversion action for a stream
enum class ConversionAction
{
    Keep,    // Use as-is
    Convert, // Convert to Blu-ray compatible format
    Drop     // Exclude from mux
};

// Stream types
enum class StreamType
{
    Video,
    Audio,
    Subtitle,
    Unknown
};

// Subtitle format classification
enum class SubtitleFormat
{
    SRT,    // Plain text SRT
    ASS,    // Advanced SubStation Alpha
    SSA,    // SubStation Alpha
    VTT,    // WebVTT
    PGS,    // Blu-ray native bitmap subtitle
    DVB,    // DVB subtitle
    VOBSUB, // DVD subtitle
    Unknown
};

// Represents a single media stream within a file
struct StreamInfo
{
    int index = -1; // ffprobe stream index
    StreamType type = StreamType::Unknown;
    QString codecName;    // h264, ac3, subrip, etc.
    QString codecProfile; // baseline, main, high, etc. (video)
    QString language;     // ISO 639 language code or "und"
    QString title;        // Stream title if available

    // Video-specific
    int width = 0;
    int height = 0;
    QString pixelFormat;   // yuv420p, etc.
    QString frameRate;     // "23.976", "25", "29.97", "30" fps
    int level = 0;         // H.264 level (e.g., 42 for 4.2)
    long long bitRate = 0; // bits per second

    // Audio-specific
    int channels = 0;   // 1, 2, 6, etc.
    int sampleRate = 0; // 48000, etc.

    // Subtitle-specific
    SubtitleFormat subFormat = SubtitleFormat::Unknown;

    // Compliance
    ComplianceStatus compliance = ComplianceStatus::Compliant;
    QString complianceNote; // Human-readable reason for non-compliance

    // User action
    ConversionAction action = ConversionAction::Keep;

    // Output when converted
    QString convertedPath; // Path to converted file if action == Convert

    QString debugString() const;
};

// Represents a single input MKV file
struct VideoItem
{
    QString filePath;
    QString fileName; // Just the filename, for display

    long long durationMs = 0; // Duration in milliseconds

    QVector<StreamInfo> streams;

    // Titles for chapter markers
    QString title;

    // Overall compliance (worst of all streams)
    ComplianceStatus overallCompliance = ComplianceStatus::Compliant;

    // State for conversion pipeline
    bool inspected = false;
    bool converted = false; // true if any stream needed/completed conversion
    QString conversionErrorMsg;

    // Results after pipeline
    QString normalizedOutputPath; // Path to final muxable file (either original or converted)

    int videoStreamCount() const;
    int audioStreamCount() const;
    int subtitleStreamCount() const;

    QString complianceSummary() const;
};

// Represents the entire Blu-ray project
struct BlurayProject
{
    QVector<VideoItem> titles; // All input files
    QString outputFolder;      // Where to generate BDMV/ and ISO
    QString projectName;       // For ISO label

    // Build state
    enum class ProjectState
    {
        Empty,
        FilesAdded,
        StreamsInspected,
        ConversionsReady,
        Converting,
        ConversionsDone,
        Muxing,
        CreatingISO,
        Complete,
        Failed
    };
    ProjectState state = ProjectState::Empty;
};

// Inline implementations
inline QString StreamInfo::debugString() const
{
    switch (type)
    {
    case StreamType::Video:
        return QString("Video[%1] %2 %3x%4 %5fps Level %6 %7")
            .arg(index)
            .arg(codecName)
            .arg(width)
            .arg(height)
            .arg(frameRate)
            .arg(level)
            .arg(pixelFormat);
    case StreamType::Audio:
        return QString("Audio[%1] %2 %3 %4ch %5Hz %6")
            .arg(index)
            .arg(language)
            .arg(codecName)
            .arg(channels)
            .arg(sampleRate)
            .arg(title);
    case StreamType::Subtitle:
        return QString("Subtitle[%1] %2 %3 (%4)")
            .arg(index)
            .arg(language)
            .arg(codecName)
            .arg(title);
    default:
        return QString("Unknown[%1]").arg(index);
    }
}

inline int VideoItem::videoStreamCount() const
{
    int count = 0;
    for (const auto &s : streams)
    {
        if (s.type == StreamType::Video)
            ++count;
    }
    return count;
}

inline int VideoItem::audioStreamCount() const
{
    int count = 0;
    for (const auto &s : streams)
    {
        if (s.type == StreamType::Audio)
            ++count;
    }
    return count;
}

inline int VideoItem::subtitleStreamCount() const
{
    int count = 0;
    for (const auto &s : streams)
    {
        if (s.type == StreamType::Subtitle)
            ++count;
    }
    return count;
}

inline QString VideoItem::complianceSummary() const
{
    int compliant = 0, warn = 0, fail = 0;
    for (const auto &s : streams)
    {
        switch (s.compliance)
        {
        case ComplianceStatus::Compliant:
            ++compliant;
            break;
        case ComplianceStatus::Warning:
            ++warn;
            break;
        case ComplianceStatus::NonCompliant:
            ++fail;
            break;
        }
    }

    QString summary;
    if (fail > 0)
    {
        summary += QString("%1 non-compliant").arg(fail);
    }
    if (warn > 0)
    {
        if (!summary.isEmpty())
            summary += ", ";
        summary += QString("%1 warning").arg(warn);
    }
    if (compliant > 0)
    {
        if (!summary.isEmpty())
            summary += ", ";
        summary += QString("%1 compliant").arg(compliant);
    }
    return summary.isEmpty() ? "No streams" : summary;
}
