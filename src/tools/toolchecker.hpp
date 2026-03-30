#pragma once

#include <QString>
#include <QMap>

// Tool availability checker - stateless, uses free functions per CppCoreGuidelines
// All operations are side-effect free and reusable
namespace Tools
{

    struct ToolStatus
    {
        QString name;
        bool available = false;
        QString path;
        QString version;
        QString reason; // Error message if not available

        bool isRequired = true; // If false, feature is just disabled
    };

    // Check if a specific CLI tool can be found in PATH
    // Returns availability and version string if available
    ToolStatus checkTool(const QString &toolName, bool required = true);

    // Check all required and optional tools
    // Returns map of tool names to their statuses
    QMap<QString, ToolStatus> checkAllTools();

    // Tool name constants for consistency (prevents typos)
    constexpr const char *FFPROBE = "ffprobe";
    constexpr const char *FFMPEG = "ffmpeg";
    constexpr const char *TSMUXER = "tsmuxer";
    constexpr const char *XORRISO = "xorriso";
    constexpr const char *MKISOFS = "mkisofs";
    constexpr const char *BDSUP2SUB = "bdsup2sub";
    constexpr const char *JAVA = "java";

    // Extended checks
    bool canRunFFmpeg();        // Check ffmpeg with specific codec support
    bool canConvertSubtitles(); // Check for PGS conversion capability

}
