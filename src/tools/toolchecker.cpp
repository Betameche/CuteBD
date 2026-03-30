#include "toolchecker.hpp"
#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

namespace Tools
{

    ToolStatus checkTool(const QString &toolName, bool required)
    {
        ToolStatus status;
        status.name = toolName;
        status.isRequired = required;

        // Try to find in PATH using QProcess which
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);

#ifdef Q_OS_WIN
        // Windows: check for .exe extension
        QString searchName = toolName + ".exe";
#else
        // Linux/macOS: tool name as-is
        QString searchName = toolName;
#endif

        // Try absolute path if it looks like a path
        if (toolName.contains("/") || toolName.contains("\\"))
        {
            if (QFileInfo::exists(toolName))
            {
                status.path = toolName;
                status.available = QFileInfo(toolName).isExecutable();
                if (status.available)
                {
                    // Try to get version
                    process.start(toolName, QStringList() << "--version");
                    if (process.waitForFinished(2000))
                    {
                        QString output = process.readAll();
                        // Extract first line for version display
                        status.version = output.split('\n').first();
                    }
                }
                return status;
            }
        }

        // Search in PATH
        QString path = QStandardPaths::findExecutable(searchName);

        if (!path.isEmpty())
        {
            status.path = path;
            status.available = true;

            // Get version info
            process.start(path, QStringList() << "--version");
            if (process.waitForFinished(2000))
            {
                QString output = process.readAll();
                status.version = output.split('\n').first();
            }
        }
        else
        {
            status.available = false;
            status.reason = QString("Tool '%1' not found in PATH").arg(toolName);
        }

        return status;
    }

    QMap<QString, ToolStatus> checkAllTools()
    {
        QMap<QString, ToolStatus> results;

        // Required tools
        results[FFPROBE] = checkTool(FFPROBE, true);
        results[FFMPEG] = checkTool(FFMPEG, true);
        results[TSMUXER] = checkTool(TSMUXER, true);

        // ISO creation tools (one of these required)
        auto xorriso = checkTool(XORRISO, false);
        auto mkisofs = checkTool(MKISOFS, false);
        results[XORRISO] = xorriso;
        results[MKISOFS] = mkisofs;

        // Optional subtitle conversion
        results[JAVA] = checkTool(JAVA, false);
        results[BDSUP2SUB] = checkTool(BDSUP2SUB, false);

        return results;
    }

    bool canRunFFmpeg()
    {
        auto status = checkTool(FFMPEG);
        if (!status.available)
        {
            return false;
        }

        // Quick codec check: verify h264 and ac3 encoding support
        QProcess process;
        process.start(status.path, QStringList() << "-codecs");
        if (process.waitForFinished(3000))
        {
            QString output = process.readAll();
            // Should show h264 and ac3 support
            return output.contains("h264") && output.contains("ac3");
        }

        return status.available;
    }

    bool canConvertSubtitles()
    {
        // Check if Java is available
        auto java = checkTool(JAVA, false);
        if (!java.available)
        {
            return false;
        }

        // Check if bdsup2sub.jar can be found
        // Common locations on Linux
        QStringList possiblePaths = {
            "/usr/share/bdsup2sub",
            "/usr/local/share/bdsup2sub",
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/bdsup2sub",
            QDir::homePath() + "/bdsup2sub",
            QDir::homePath() + "/.local/share/bdsup2sub",
        };

        for (const auto &dir : possiblePaths)
        {
            QFileInfo jar(dir + "/bdsup2sub.jar");
            if (jar.exists() && jar.isFile())
            {
                return true;
            }
        }

        // If not found in standard locations, just return that Java is available
        // User can still use subtitle conversion by specifying path
        return java.available;
    }

}
