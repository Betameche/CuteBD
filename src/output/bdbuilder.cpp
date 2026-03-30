#include "bdbuilder.hpp"
#include "common.hpp"
#include "toolchecker.hpp"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>

namespace Output
{

    QString BDBuilder::buildBDMV(
        const BlurayProject &project,
        std::function<void(const QString &)> onProgress)
    {
        if (onProgress)
            onProgress("Starting BDMV build...");

        // Check if tsmuxer is available
        auto tsmuxer = Tools::checkTool(Tools::TSMUXER);
        if (!tsmuxer.available)
        {
            qWarning() << "tsmuxer not available:" << tsmuxer.reason;
            return "";
        }

        // Create output directory structure
        QString bdmvBase = Common::joinPath(project.outputFolder, "BDMV");

        if (!Common::createDirectoryRecursive(bdmvBase))
        {
            qWarning() << "Failed to create BDMV directory:" << bdmvBase;
            return "";
        }

        if (onProgress)
            onProgress("BDMV directory created");

        // Generate tsmuxer meta file
        QString metaFile = Common::joinPath(project.outputFolder, "playlist.txt");

        if (!generateTsmuxerMeta(project).isEmpty())
        {
            auto writeResult = Common::writeFile(metaFile, generateTsmuxerMeta(project));
            if (!writeResult.success)
            {
                qWarning() << "Failed to write meta file:" << writeResult.error;
                return "";
            }
        }
        else
        {
            qWarning() << "Failed to generate tsmuxer meta file";
            return "";
        }

        if (onProgress)
            onProgress("Generated tsmuxer configuration");

        // Run tsmuxer
        if (!runTsmuxer(metaFile, project.outputFolder))
        {
            qWarning() << "tsmuxer execution failed";
            return "";
        }

        if (onProgress)
            onProgress("tsmuxer completed");

        // Verify BDMV structure
        if (!verifyBDMVStructure(bdmvBase))
        {
            qWarning() << "BDMV structure verification failed";
            return "";
        }

        if (onProgress)
            onProgress("BDMV structure verified");

        return bdmvBase;
    }

    QString BDBuilder::generateISO(
        const QString &bdmvPath,
        const QString &outputISOPath,
        const QString &volumeLabel,
        std::function<void(const QString &)> onProgress)
    {
        if (onProgress)
            onProgress("Starting ISO generation...");

        // Verify BDMV exists
        if (!Common::directoryExists(bdmvPath))
        {
            qWarning() << "BDMV path does not exist:" << bdmvPath;
            return "";
        }

        // Find available ISO tool
        QString isoTool = findISOTool();
        if (isoTool.isEmpty())
        {
            qWarning() << "No ISO creation tool found (need xorriso or mkisofs)";
            return "";
        }

        if (onProgress)
        {
            onProgress(QString("Using ISO tool: %1").arg(isoTool));
        }

        // Run ISO generation
        if (!runISOGeneration(isoTool, bdmvPath, outputISOPath, volumeLabel, onProgress))
        {
            qWarning() << "ISO generation failed";
            return "";
        }

        // Verify ISO was created
        if (!Common::fileExists(outputISOPath))
        {
            qWarning() << "ISO file was not created:" << outputISOPath;
            return "";
        }

        if (onProgress)
            onProgress("ISO generation completed successfully");

        return outputISOPath;
    }

    QString BDBuilder::generateTsmuxerMeta(const BlurayProject &project)
    {
        QStringList meta;
        meta << "MUXOPT --blu-ray";
        meta << "";

        // Add each title to the playlist
        for (int i = 0; i < project.titles.size(); ++i)
        {
            const auto &title = project.titles[i];

            if (title.normalizedOutputPath.isEmpty())
            {
                qWarning() << "Title" << i << "has no normalized output path";
                continue;
            }

            // Add video stream
            int vidCount = 0;
            for (const auto &stream : title.streams)
            {
                if (stream.type == StreamType::Video && stream.action != ConversionAction::Drop)
                {
                    meta << QString("V_MPEG4/ISO/AVC, \"%1\", track=%2")
                                .arg(title.normalizedOutputPath)
                                .arg(stream.index);
                    ++vidCount;
                }
            }

            if (vidCount == 0)
            {
                qWarning() << "Title" << i << "has no valid video stream";
                continue;
            }

            // Add audio streams
            for (const auto &stream : title.streams)
            {
                if (stream.type == StreamType::Audio && stream.action != ConversionAction::Drop)
                {
                    meta << QString("A_AC3, \"%1\", track=%2")
                                .arg(title.normalizedOutputPath)
                                .arg(stream.index);
                }
            }

            // Add subtitle streams if present
            // NOTE: Full PGS encoding not implemented in MVP, so skip for now
            for (const auto &stream : title.streams)
            {
                if (stream.type == StreamType::Subtitle && stream.action != ConversionAction::Drop)
                {
                    // Skip - requires separate PGS files
                }
            }

            if (i < project.titles.size() - 1)
            {
                meta << ""; // Blank line between titles
            }
        }

        return meta.join('\n');
    }

    bool BDBuilder::runTsmuxer(const QString &metaFile, const QString &outputPath)
    {
        auto tsmuxer = Tools::checkTool(Tools::TSMUXER);
        if (!tsmuxer.available)
        {
            return false;
        }

        QStringList args;
        args << metaFile << outputPath;

        auto result = Common::runCommand(tsmuxer.path, args);

        return result.success;
    }

    bool BDBuilder::verifyBDMVStructure(const QString &bdmvPath)
    {
        // Check if required directories exist
        QStringList requiredDirs = {"CLIPINF", "PLAYLIST", "AUXDATA", "JAR"};

        for (const auto &dir : requiredDirs)
        {
            QString path = Common::joinPath(bdmvPath, dir);
            if (!Common::directoryExists(path))
            {
                qWarning() << "Missing required directory:" << path;
                // Some directories may be optional in certain scenarios
            }
        }

        // At least CLIPINF and PLAYLIST should exist
        return Common::directoryExists(Common::joinPath(bdmvPath, "CLIPINF")) && Common::directoryExists(Common::joinPath(bdmvPath, "PLAYLIST"));
    }

    QString BDBuilder::findISOTool()
    {
        // Prefer xorriso (more modern, UDF support)
        auto xorriso = Tools::checkTool(Tools::XORRISO, false);
        if (xorriso.available)
        {
            return xorriso.path;
        }

        // Fallback to mkisofs
        auto mkisofs = Tools::checkTool(Tools::MKISOFS, false);
        if (mkisofs.available)
        {
            return mkisofs.path;
        }

        return "";
    }

    bool BDBuilder::runISOGeneration(
        const QString &tool,
        const QString &bdmvPath,
        const QString &outputISOPath,
        const QString &volumeLabel,
        std::function<void(const QString &)> onProgress)
    {
        QStringList args;
        QString toolName = QFileInfo(tool).fileName();

        if (toolName == "xorriso" || toolName.contains("xorriso"))
        {
            // xorriso command line
            args << "-as" << "mkisofs"
                 << "-R"   // Rock Ridge
                 << "-J"   // Joliet
                 << "-udf" // UDF file system
                 << "-V" << volumeLabel
                 << "-o" << outputISOPath
                 << bdmvPath;
        }
        else if (toolName == "mkisofs" || toolName.contains("mkisofs"))
        {
            // mkisofs command line
            args << "-R"
                 << "-J"
                 << "-V" << volumeLabel
                 << "-o" << outputISOPath
                 << bdmvPath;
        }
        else
        {
            qWarning() << "Unknown ISO tool:" << toolName;
            return false;
        }

        auto result = Common::runCommand(tool, args, onProgress);

        return result.success;
    }

}
