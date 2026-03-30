#include "mainwindow.hpp"
#include "inspector.hpp"
#include "converter.hpp"
#include "bdbuilder.hpp"
#include "common.hpp"
#include "toolchecker.hpp"

#include <QtConcurrent/QtConcurrent>
#include <QFileInfo>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>

void MainWindow::onInspectStreams()
{
    if (project.titles.isEmpty())
    {
        logMessage("No files to inspect", "red");
        return;
    }

    if (pipelineState != PipelineState::Idle)
    {
        logMessage("Pipeline already running", "orange");
        return;
    }

    pipelineState = PipelineState::Inspecting;
    updatePipelineButtons();

    logMessage("Starting stream inspection...", "blue");
    progressBar->setVisible(true);
    progressBar->setValue(0);

    if (!inspectionWatcher)
    {
        inspectionWatcher = new QFutureWatcher<void>(this);
        connect(inspectionWatcher, &QFutureWatcher<void>::finished, this, &MainWindow::onInspectionFinished);
    }

    QFuture<void> future = QtConcurrent::run([this]()
                                             { inspectAllStreamsAsync(); });

    inspectionWatcher->setFuture(future);
}

void MainWindow::inspectAllStreamsAsync()
{
    Inspection::Inspector inspector;
    int filesInspected = 0;
    int totalFiles = project.titles.size();

    for (auto &title : project.titles)
    {
        QString error;
        title.streams = inspector.inspectFile(title.filePath, error);

        if (title.streams.isEmpty())
        {
            QMetaObject::invokeMethod(this, [this, fileName = title.fileName, error]()
                                      { logMessage(QString("Error inspecting %1: %2").arg(fileName, error), "red"); }, Qt::QueuedConnection);
        }
        else
        {
            QMetaObject::invokeMethod(this, [this, fileName = title.fileName, count = title.streams.size()]()
                                      { logMessage(QString("Inspected %1: %2 streams").arg(fileName, QString::number(count)), "green"); }, Qt::QueuedConnection);
            title.inspected = true;
        }

        filesInspected++;
        int percent = (totalFiles > 0) ? (filesInspected * 100) / totalFiles : 0;
        QMetaObject::invokeMethod(this, [this, percent]()
                                  { progressBar->setValue(percent); }, Qt::QueuedConnection);
    }

    QMetaObject::invokeMethod(this, [this]()
                              { logMessage("Stream inspection completed", "green"); }, Qt::QueuedConnection);
}

void MainWindow::onInspectionFinished()
{
    pipelineState = PipelineState::Idle;
    progressBar->setValue(100);

    updateFileTable();
    updateStreamTable();
    updatePipelineButtons();

    convertBtn->setEnabled(true);
    buildBDMVBtn->setEnabled(true);
}

void MainWindow::onStartConversion()
{
    if (project.titles.isEmpty())
    {
        logMessage("No files to convert", "red");
        return;
    }

    bool hasConversions = false;
    for (const auto &title : project.titles)
    {
        for (const auto &stream : title.streams)
        {
            if (stream.action == ConversionAction::Convert)
            {
                hasConversions = true;
                break;
            }
        }
        if (hasConversions)
            break;
    }

    if (!hasConversions)
    {
        logMessage("No streams marked for conversion", "blue");
        return;
    }

    if (pipelineState != PipelineState::Idle)
    {
        logMessage("Pipeline already running", "orange");
        return;
    }

    pipelineState = PipelineState::Converting;
    updatePipelineButtons();

    logMessage("Starting stream conversions...", "blue");
    progressBar->setVisible(true);
    progressBar->setValue(0);

    if (!conversionWatcher)
    {
        conversionWatcher = new QFutureWatcher<void>(this);
        connect(conversionWatcher, &QFutureWatcher<void>::finished, this, &MainWindow::onConversionFinished);
    }

    QFuture<void> future = QtConcurrent::run([this]()
                                             {
        Conversion::Converter converter;
        int totalStreams = 0;
        int processedStreams = 0;

        for (auto& title : project.titles) {
            for (auto& stream : title.streams) {
                if (stream.action == ConversionAction::Convert) {
                    totalStreams++;
                }
            }
        }

        for (int fileIdx = 0; fileIdx < project.titles.size(); ++fileIdx) {
            auto& title = project.titles[fileIdx];

            for (int streamIdx = 0; streamIdx < title.streams.size(); ++streamIdx) {
                auto& stream = title.streams[streamIdx];

                if (stream.action != ConversionAction::Convert) {
                    continue;
                }

                QString baseName = QFileInfo(title.filePath).baseName();
                QString tempDir = Common::getTempDirectory();
                QString outputFile = Common::joinPath(tempDir, QString("cutebd_convert_%1_%2.mkv")
                    .arg(baseName).arg(streamIdx));

                QMetaObject::invokeMethod(this, [this, titleName = title.fileName, streamIndex = stream.index, streamType = stream.type]() {
                    logMessage(QString("Converting %1 stream %2 (type: %3)...")
                        .arg(titleName)
                        .arg(streamIndex)
                        .arg(streamTypeIcon(streamType)), "blue");
                }, Qt::QueuedConnection);

                bool success = converter.convertStream(stream, title.filePath, stream.index, outputFile, nullptr);

                if (success && Common::fileExists(outputFile)) {
                    stream.convertedPath = outputFile;
                    title.converted = true;
                    QMetaObject::invokeMethod(this, [this, streamIndex = stream.index]() {
                        logMessage(QString("✓ Stream %1 converted successfully").arg(streamIndex), "green");
                    }, Qt::QueuedConnection);
                } else {
                    stream.action = ConversionAction::Drop;
                    QMetaObject::invokeMethod(this, [this, streamIndex = stream.index]() {
                        logMessage(QString("✗ Failed to convert stream %1").arg(streamIndex), "red");
                    }, Qt::QueuedConnection);
                }

                processedStreams++;
                QMetaObject::invokeMethod(this, [this, processedStreams, totalStreams]() {
                    int percent = (totalStreams > 0) ? (processedStreams * 100) / totalStreams : 0;
                    progressBar->setValue(percent);
                }, Qt::QueuedConnection);
            }
        }

        for (auto& title : project.titles) {
            QString lastConverted;
            for (auto& stream : title.streams) {
                if (stream.action == ConversionAction::Convert && !stream.convertedPath.isEmpty()) {
                    lastConverted = stream.convertedPath;
                }
            }

            if (!lastConverted.isEmpty()) {
                title.normalizedOutputPath = lastConverted;
            } else {
                title.normalizedOutputPath = title.filePath;
            }
        } });

    conversionWatcher->setFuture(future);
}

void MainWindow::onConversionFinished()
{
    pipelineState = PipelineState::Idle;
    progressBar->setValue(100);

    logMessage("Stream conversions completed", "green");
    logMessage("Ready to build BDMV structure", "blue");

    updateStreamTable();
    updatePipelineButtons();

    QString summary;
    for (const auto &title : project.titles)
    {
        if (title.converted)
        {
            summary += QString("  • %1: %2 stream(s) converted\n")
                           .arg(title.fileName)
                           .arg(title.streams.size());
        }
    }

    if (!summary.isEmpty())
    {
        logMessage(QString("Conversion Summary:\n%1").arg(summary), "blue");
    }

    buildBDMVBtn->setEnabled(true);
}

void MainWindow::onBuildBDMV()
{
    if (project.titles.isEmpty())
    {
        logMessage("No files to process", "red");
        return;
    }

    if (outputFolderEdit->text().isEmpty())
    {
        logMessage("Please set output folder", "orange");
        return;
    }

    if (projectNameEdit->text().isEmpty())
    {
        logMessage("Please set project name", "orange");
        return;
    }

    if (pipelineState != PipelineState::Idle)
    {
        logMessage("Pipeline already running", "orange");
        return;
    }

    pipelineState = PipelineState::BuildingBDMV;
    updatePipelineButtons();

    project.outputFolder = outputFolderEdit->text();
    project.projectName = projectNameEdit->text();

    logMessage(QString("Building BDMV in: %1").arg(project.outputFolder), "blue");
    progressBar->setVisible(true);
    progressBar->setValue(0);

    if (!bdmvWatcher)
    {
        bdmvWatcher = new QFutureWatcher<void>(this);
        connect(bdmvWatcher, &QFutureWatcher<void>::finished, this, &MainWindow::onBDMVBuildFinished);
    }

    QFuture<void> future = QtConcurrent::run([this]()
                                             {
        Output::BDBuilder builder;

        QString bdmvPath = builder.buildBDMV(project, [this](const QString& msg) {
            QMetaObject::invokeMethod(this, [this, msg]() {
                logMessage(msg, "blue");
                progressBar->setValue(progressBar->value() + 10);
            }, Qt::QueuedConnection);
        });

        if (bdmvPath.isEmpty()) {
            QMetaObject::invokeMethod(this, [this]() {
                logMessage("Failed to build BDMV structure", "red");
            }, Qt::QueuedConnection);
        } else {
            project.state = BlurayProject::ProjectState::ConversionsDone;
            QMetaObject::invokeMethod(this, [this, bdmvPath]() {
                logMessage(QString("BDMV structure created: %1").arg(bdmvPath), "green");
                progressBar->setValue(100);
            }, Qt::QueuedConnection);
        } });

    bdmvWatcher->setFuture(future);
}

void MainWindow::onBDMVBuildFinished()
{
    pipelineState = PipelineState::Idle;
    updatePipelineButtons();

    if (Tools::checkTool(Tools::XORRISO, false).available || Tools::checkTool(Tools::MKISOFS, false).available)
    {
        generateISOBtn->setEnabled(true);
        logMessage("BDMV build complete. Ready to generate ISO.", "green");
    }
}

void MainWindow::onGenerateISO()
{
    if (outputFolderEdit->text().isEmpty())
    {
        logMessage("Please set output folder", "orange");
        return;
    }

    if (projectNameEdit->text().isEmpty())
    {
        logMessage("Please set project name", "orange");
        return;
    }

    if (pipelineState != PipelineState::Idle)
    {
        logMessage("Pipeline already running", "orange");
        return;
    }

    QString bdmvPath = Common::joinPath(outputFolderEdit->text(), "BDMV");
    if (!Common::directoryExists(bdmvPath))
    {
        logMessage("BDMV folder not found. Build BDMV structure first.", "red");
        return;
    }

    pipelineState = PipelineState::GeneratingISO;
    updatePipelineButtons();

    QString isoPath = Common::joinPath(outputFolderEdit->text(),
                                       projectNameEdit->text() + ".iso");

    logMessage(QString("Generating ISO: %1").arg(isoPath), "blue");
    progressBar->setVisible(true);
    progressBar->setValue(0);

    if (!isoWatcher)
    {
        isoWatcher = new QFutureWatcher<void>(this);
        connect(isoWatcher, &QFutureWatcher<void>::finished, this, &MainWindow::onISOGenerationFinished);
    }

    QFuture<void> future = QtConcurrent::run([this, bdmvPath, isoPath]()
                                             {
        Output::BDBuilder builder;

        QString projectName = projectNameEdit->text();
        QString result = builder.generateISO(bdmvPath, isoPath, projectName,
            [this](const QString& msg) {
                QMetaObject::invokeMethod(this, [this, msg]() {
                    logMessage(msg, "blue");
                }, Qt::QueuedConnection);
            });

        if (result.isEmpty()) {
            QMetaObject::invokeMethod(this, [this]() {
                logMessage("Failed to generate ISO", "red");
            }, Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(this, [this, result]() {
                logMessage(QString("ISO generated successfully: %1").arg(result), "green");
                progressBar->setValue(100);
            }, Qt::QueuedConnection);
        } });

    isoWatcher->setFuture(future);
}

void MainWindow::onISOGenerationFinished()
{
    pipelineState = PipelineState::Idle;
    updatePipelineButtons();

    project.state = BlurayProject::ProjectState::Complete;
    logMessage("Blu-ray ISO creation complete!", "green");
}
