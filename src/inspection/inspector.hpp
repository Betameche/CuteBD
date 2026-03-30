#pragma once

#include "models.hpp"
#include "compliance.hpp"
#include <QString>
#include <QVector>
#include <QJsonObject>

namespace Inspection
{

    // Inspect a media file and extract all streams with compliance status
    // Uses ffprobe JSON output for accuracy
    class Inspector
    {
    public:
        Inspector() = default;

        // Inspect a single file and populate streams with compliance info
        // Returns empty vector if inspection fails
        QVector<StreamInfo> inspectFile(const QString &filePath, QString &errorOut);

        // Get detailed compliance report for a stream
        void validateStreamCompliance(StreamInfo &stream);

    private:
        // Parse ffprobe JSON output
        QVector<StreamInfo> parseFFprobeOutput(const QString &jsonOutput);

        // Map ffprobe stream data to StreamInfo
        StreamInfo createStreamInfo(const QJsonObject &streamObj);

        // Determine subtitle format from codec
        SubtitleFormat detectSubtitleFormat(const QString &codecName);

        // Validate individual stream types
        void validateVideoCompliance(StreamInfo &stream);
        void validateAudioCompliance(StreamInfo &stream);
        void validateSubtitleCompliance(StreamInfo &stream);
    };

}
