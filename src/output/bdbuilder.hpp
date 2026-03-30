#pragma once

#include "models.hpp"
#include <QString>
#include <QVector>
#include <functional>

namespace Output
{

    // Blu-ray BDMV builder
    // Orchestrates tsmuxer and ISO generation
    class BDBuilder
    {
    public:
        BDBuilder() = default;

        // Build complete Blu-ray structure (BDMV folder)
        // Returns path to BDMV folder on success, empty string on failure
        QString buildBDMV(
            const BlurayProject &project,
            std::function<void(const QString &)> onProgress = nullptr);

        // Generate ISO from BDMV folder
        // Uses xorriso (preferred) or mkisofs (fallback)
        // Returns path to ISO file on success, empty string on failure
        QString generateISO(
            const QString &bdmvPath,
            const QString &outputISOPath,
            const QString &volumeLabel,
            std::function<void(const QString &)> onProgress = nullptr);

    private:
        // Generate tsMuxer meta.txt file
        QString generateTsmuxerMeta(const BlurayProject &project);

        // Run tsMuxer
        bool runTsmuxer(const QString &metaFile, const QString &outputPath);

        // Verify BDMV folder structure
        bool verifyBDMVStructure(const QString &bdmvPath);

        // Find ISO creation tool (xorriso or mkisofs)
        QString findISOTool();

        // Run ISO generation
        bool runISOGeneration(
            const QString &tool,
            const QString &bdmvPath,
            const QString &outputISOPath,
            const QString &volumeLabel,
            std::function<void(const QString &)> onProgress);
    };

}
