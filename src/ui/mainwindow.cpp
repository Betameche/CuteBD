#include "mainwindow.hpp"
#include "models.hpp"
#include "toolchecker.hpp"
#include "compliance.hpp"
#include "inspector.hpp"
#include "converter.hpp"
#include "bdbuilder.hpp"
#include "common.hpp"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QProgressBar>
#include <QTextEdit>
#include <QFileDialog>
#include <QMenu>
#include <QSettings>
#include <QSplitter>
#include <QGroupBox>
#include <QStatusBar>
#include <QDebug>
#include <QStandardPaths>
#include <QtConcurrent>
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("CuteBD - Blu-ray ISO Creator");
    setWindowIcon(QIcon(":/icons/app.png"));

    resize(1200, 800);

    initializeUI();
    connectSignals();
    checkToolAvailability();
    loadSettings();
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::initializeUI()
{
    // Create central widget and main layout
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // === FILES SECTION ===
    QGroupBox *filesGroup = new QGroupBox("Input Files", this);
    QVBoxLayout *filesLayout = new QVBoxLayout(filesGroup);

    fileTable = new QTableWidget(this);
    fileTable->setColumnCount(5);
    fileTable->setHorizontalHeaderLabels({"File", "Duration", "Video", "Audio", "Subtitles", "Compliance"});
    fileTable->horizontalHeader()->setStretchLastSection(true);
    fileTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    fileTable->setSelectionMode(QAbstractItemView::SingleSelection);

    QHBoxLayout *fileButtonsLayout = new QHBoxLayout();
    addFilesBtn = new QPushButton("Add Files...", this);
    removeFileBtn = new QPushButton("Remove Selected", this);
    clearAllBtn = new QPushButton("Clear All", this);

    fileButtonsLayout->addWidget(addFilesBtn);
    fileButtonsLayout->addWidget(removeFileBtn);
    fileButtonsLayout->addWidget(clearAllBtn);
    fileButtonsLayout->addStretch();

    filesLayout->addWidget(fileTable);
    filesLayout->addLayout(fileButtonsLayout);

    // === STREAM DETAILS SECTION ===
    QGroupBox *streamGroup = new QGroupBox("Stream Details", this);
    QVBoxLayout *streamLayout = new QVBoxLayout(streamGroup);

    fileDetailsLabel = new QLabel("Select a file to view stream details", this);

    streamTable = new QTableWidget(this);
    streamTable->setColumnCount(8);
    streamTable->setHorizontalHeaderLabels({"Index", "Type", "Codec", "Properties", "Language", "Compliance", "Action", "Notes"});
    streamTable->horizontalHeader()->setStretchLastSection(true);
    streamTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    streamTable->setContextMenuPolicy(Qt::CustomContextMenu);

    streamLayout->addWidget(fileDetailsLabel);
    streamLayout->addWidget(streamTable);

    // === PIPELINE SECTION ===
    QGroupBox *pipelineGroup = new QGroupBox("Conversion Pipeline", this);
    QVBoxLayout *pipelineLayout = new QVBoxLayout(pipelineGroup);

    inspectBtn = new QPushButton("1. Inspect Streams", this);
    convertBtn = new QPushButton("2. Convert Non-Compliant", this);
    buildBDMVBtn = new QPushButton("3. Build BDMV Structure", this);
    generateISOBtn = new QPushButton("4. Generate ISO", this);

    inspectBtn->setEnabled(false);
    convertBtn->setEnabled(false);
    buildBDMVBtn->setEnabled(false);
    generateISOBtn->setEnabled(false);

    QHBoxLayout *pipelineButtonsLayout = new QHBoxLayout();
    pipelineButtonsLayout->addWidget(inspectBtn);
    pipelineButtonsLayout->addWidget(convertBtn);
    pipelineButtonsLayout->addWidget(buildBDMVBtn);
    pipelineButtonsLayout->addWidget(generateISOBtn);

    pipelineLayout->addLayout(pipelineButtonsLayout);

    // === OUTPUT SECTION ===
    QGroupBox *outputGroup = new QGroupBox("Output Settings", this);
    QVBoxLayout *outputLayout = new QVBoxLayout(outputGroup);

    QHBoxLayout *folderLayout = new QHBoxLayout();
    folderLayout->addWidget(new QLabel("Output Folder:", this));
    outputFolderEdit = new QLineEdit(this);
    outputFolderEdit->setPlaceholderText("/home/user/bluray_output");
    outputFolderBrowseBtn = new QPushButton("Browse...", this);
    folderLayout->addWidget(outputFolderEdit);
    folderLayout->addWidget(outputFolderBrowseBtn);

    QHBoxLayout *projectLayout = new QHBoxLayout();
    projectLayout->addWidget(new QLabel("Project Name:", this));
    projectNameEdit = new QLineEdit(this);
    projectNameEdit->setPlaceholderText("e.g., 'My Blu-ray Project'");
    projectLayout->addWidget(projectNameEdit);

    outputLayout->addLayout(folderLayout);
    outputLayout->addLayout(projectLayout);

    // === TOOL STATUS SECTION ===
    toolStatusLabel = new QLabel("Checking tools...", this);
    toolStatusLabel->setStyleSheet("color: orange;");

    // === LOG/STATUS SECTION ===
    progressBar = new QProgressBar(this);
    progressBar->setValue(0);
    progressBar->setVisible(false);

    logOutput = new QTextEdit(this);
    logOutput->setReadOnly(true);
    logOutput->setMaximumHeight(150);
    logOutput->setFont(QFont("Courier", 8));

    // Add sections to main layout
    mainLayout->addWidget(filesGroup, 1);
    mainLayout->addWidget(streamGroup, 1);
    mainLayout->addWidget(pipelineGroup);
    mainLayout->addWidget(outputGroup);
    mainLayout->addWidget(toolStatusLabel);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(logOutput);

    setCentralWidget(central);

    // Status bar
    statusBar()->showMessage("Ready");
}

void MainWindow::connectSignals()
{
    // File operations
    connect(addFilesBtn, &QPushButton::clicked, this, &MainWindow::onAddFiles);
    connect(removeFileBtn, &QPushButton::clicked, this, &MainWindow::onRemoveSelected);
    connect(clearAllBtn, &QPushButton::clicked, this, &MainWindow::onClearAll);

    // File selection
    connect(fileTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onFileSelectionChanged);

    // Stream operations
    connect(streamTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::onStreamTableContextMenu);
    connect(streamTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onStreamSelectionChanged);

    // Pipeline
    connect(inspectBtn, &QPushButton::clicked, this, &MainWindow::onInspectStreams);
    connect(convertBtn, &QPushButton::clicked, this, &MainWindow::onStartConversion);
    connect(buildBDMVBtn, &QPushButton::clicked, this, &MainWindow::onBuildBDMV);
    connect(generateISOBtn, &QPushButton::clicked, this, &MainWindow::onGenerateISO);

    // Output folder
    connect(outputFolderBrowseBtn, &QPushButton::clicked, this, &MainWindow::onOutputFolderBrowse);
}

void MainWindow::checkToolAvailability()
{
    toolStatus = Tools::checkAllTools();
    updateToolStatus();
}

void MainWindow::updateToolStatus()
{
    QStringList status;
    bool allRequired = true;

    auto check = [&status, &allRequired](const QString &name, const Tools::ToolStatus &tool)
    {
        if (tool.available)
        {
            status << QString("✓ %1").arg(name);
        }
        else
        {
            status << QString("✗ %1").arg(name);
            if (tool.isRequired)
                allRequired = false;
        }
    };

    check("ffprobe", toolStatus[Tools::FFPROBE]);
    check("ffmpeg", toolStatus[Tools::FFMPEG]);
    check("tsmuxer", toolStatus[Tools::TSMUXER]);
    check("ISO tool", toolStatus[Tools::XORRISO].available ? toolStatus[Tools::XORRISO] : toolStatus[Tools::MKISOFS]);

    QString message = status.join("  |  ");
    toolStatusLabel->setText(message);

    if (allRequired)
    {
        toolStatusLabel->setStyleSheet("color: green;");
        inspectBtn->setEnabled(!project.titles.isEmpty());
    }
    else
    {
        toolStatusLabel->setStyleSheet("color: red;");
        inspectBtn->setEnabled(false);
    }
}

void MainWindow::updatePipelineButtons()
{
    bool idle = (pipelineState == PipelineState::Idle);
    bool hasFiles = !project.titles.isEmpty();
    bool hasOutput = !outputFolderEdit->text().isEmpty();

    inspectBtn->setEnabled(idle && hasFiles && toolStatus[Tools::FFPROBE].available);
    convertBtn->setEnabled(idle && hasFiles);
    buildBDMVBtn->setEnabled(idle && hasFiles && hasOutput);
    generateISOBtn->setEnabled(idle && hasOutput);

    // Show operation state
    if (!idle)
    {
        switch (pipelineState)
        {
        case PipelineState::Inspecting:
            statusBar()->showMessage("Inspecting streams...");
            break;
        case PipelineState::Converting:
            statusBar()->showMessage("Converting streams...");
            break;
        case PipelineState::BuildingBDMV:
            statusBar()->showMessage("Building BDMV structure...");
            break;
        case PipelineState::GeneratingISO:
            statusBar()->showMessage("Generating ISO...");
            break;
        default:
            break;
        }
    }
    else
    {
        statusBar()->showMessage("Ready");
    }
}

void MainWindow::logMessage(const QString &msg, const QString &color)
{
    QString html = QString("<span style='color:%1'>%2</span>").arg(color, msg.toHtmlEscaped());
    logOutput->append(html);

    // Auto-scroll to bottom
    QTextCursor cursor = logOutput->textCursor();
    cursor.movePosition(QTextCursor::End);
    logOutput->setTextCursor(cursor);
}

void MainWindow::onAddFiles()
{
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        "Select MKV Files",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "MKV Files (*.mkv);;All Files (*)");

    if (files.isEmpty())
        return;

    for (const auto &file : files)
    {
        VideoItem item;
        item.filePath = file;
        item.fileName = QFileInfo(file).fileName();

        project.titles.append(item);
    }

    logMessage(QString("Added %1 file(s)").arg(files.size()), "blue");
    updateFileTable();

    inspectBtn->setEnabled(toolStatus[Tools::FFPROBE].available);
}

void MainWindow::onRemoveSelected()
{
    if (selectedFileIndex < 0)
        return;

    logMessage(QString("Removed: %1").arg(project.titles[selectedFileIndex].fileName));
    project.titles.removeAt(selectedFileIndex);

    updateFileTable();
    updateStreamTable();

    inspectBtn->setEnabled(toolStatus[Tools::FFPROBE].available && !project.titles.isEmpty());
}

void MainWindow::onClearAll()
{
    project.titles.clear();
    selectedFileIndex = -1;

    logMessage("Cleared all files");
    updateFileTable();
    updateStreamTable();

    inspectBtn->setEnabled(false);
}

void MainWindow::onFileSelectionChanged()
{
    QList<QTableWidgetItem *> selected = fileTable->selectedItems();
    if (selected.isEmpty())
    {
        selectedFileIndex = -1;
        updateStreamTable();
        return;
    }

    selectedFileIndex = fileTable->row(selected.first());
    updateStreamTable();
}

void MainWindow::onStreamSelectionChanged()
{
    QList<QTableWidgetItem *> selected = streamTable->selectedItems();
    if (selected.isEmpty())
    {
        selectedStreamIndex = -1;
        return;
    }

    selectedStreamIndex = streamTable->row(selected.first());
}

void MainWindow::updateFileTable()
{
    fileTable->setRowCount(project.titles.size());

    for (int i = 0; i < project.titles.size(); ++i)
    {
        const auto &title = project.titles[i];

        auto *fileItem = new QTableWidgetItem(title.fileName);
        auto *durationItem = new QTableWidgetItem("--");
        auto *videoItem = new QTableWidgetItem(QString::number(title.videoStreamCount()));
        auto *audioItem = new QTableWidgetItem(QString::number(title.audioStreamCount()));
        auto *subItem = new QTableWidgetItem(QString::number(title.subtitleStreamCount()));
        auto *complianceItem = new QTableWidgetItem("--");

        fileTable->setItem(i, 0, fileItem);
        fileTable->setItem(i, 1, durationItem);
        fileTable->setItem(i, 2, videoItem);
        fileTable->setItem(i, 3, audioItem);
        fileTable->setItem(i, 4, subItem);
        fileTable->setItem(i, 5, complianceItem);
    }
}

void MainWindow::updateStreamTable()
{
    streamTable->setRowCount(0);

    if (selectedFileIndex < 0 || selectedFileIndex >= project.titles.size())
    {
        fileDetailsLabel->setText("Select a file to view stream details");
        return;
    }

    const auto &title = project.titles[selectedFileIndex];
    fileDetailsLabel->setText(QString("File: %1 | Compliance: %2")
                                  .arg(title.fileName, title.complianceSummary()));

    streamTable->setRowCount(title.streams.size());

    for (int i = 0; i < title.streams.size(); ++i)
    {
        const auto &stream = title.streams[i];

        auto *indexItem = new QTableWidgetItem(QString::number(stream.index));
        auto *typeItem = new QTableWidgetItem(streamTypeIcon(stream.type));
        auto *codecItem = new QTableWidgetItem(stream.codecName);

        QString props;
        if (stream.type == StreamType::Video)
        {
            props = QString("%1x%2 %3fps").arg(stream.width).arg(stream.height).arg(stream.frameRate);
        }
        else if (stream.type == StreamType::Audio)
        {
            props = QString("%1ch %2Hz").arg(stream.channels).arg(stream.sampleRate);
        }
        auto *propsItem = new QTableWidgetItem(props);

        auto *langItem = new QTableWidgetItem(Compliance::languageNameFromCode(stream.language));
        auto *compItem = new QTableWidgetItem(complianceBadge(stream.compliance));
        auto *actionItem = new QTableWidgetItem(conversionActionText(stream.action));
        auto *noteItem = new QTableWidgetItem(stream.complianceNote);

        streamTable->setItem(i, 0, indexItem);
        streamTable->setItem(i, 1, typeItem);
        streamTable->setItem(i, 2, codecItem);
        streamTable->setItem(i, 3, propsItem);
        streamTable->setItem(i, 4, langItem);
        streamTable->setItem(i, 5, compItem);
        streamTable->setItem(i, 6, actionItem);
        streamTable->setItem(i, 7, noteItem);
    }
}

QString MainWindow::complianceBadge(ComplianceStatus status) const
{
    switch (status)
    {
    case ComplianceStatus::Compliant:
        return "✓ OK";
    case ComplianceStatus::Warning:
        return "⚠ Warning";
    case ComplianceStatus::NonCompliant:
        return "✗ Non-compliant";
    default:
        return "?";
    }
}

QString MainWindow::streamTypeIcon(StreamType type) const
{
    switch (type)
    {
    case StreamType::Video:
        return "🎬 Video";
    case StreamType::Audio:
        return "🔊 Audio";
    case StreamType::Subtitle:
        return "📝 Subtitle";
    default:
        return "? Unknown";
    }
}

QString MainWindow::conversionActionText(ConversionAction action) const
{
    switch (action)
    {
    case ConversionAction::Keep:
        return "Keep";
    case ConversionAction::Convert:
        return "Convert";
    case ConversionAction::Drop:
        return "Drop";
    default:
        return "?";
    }
}

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

    // Create watcher if not exists
    if (!conversionWatcher)
    {
        conversionWatcher = new QFutureWatcher<void>(this);
        connect(conversionWatcher, &QFutureWatcher<void>::finished, this, &MainWindow::onConversionFinished);
    }

    // Run conversions in background thread
    QFuture<void> future = QtConcurrent::run([this]()
                                             {
        Conversion::Converter converter;
        
        int totalStreams = 0;
        int processedStreams = 0;

        // Count total streams to convert
        for (auto& title : project.titles) {
            for (auto& stream : title.streams) {
                if (stream.action == ConversionAction::Convert) {
                    totalStreams++;
                }
            }
        }

        // Process each file and stream
        for (int fileIdx = 0; fileIdx < project.titles.size(); ++fileIdx) {
            auto& title = project.titles[fileIdx];

            for (int streamIdx = 0; streamIdx < title.streams.size(); ++streamIdx) {
                auto& stream = title.streams[streamIdx];

                if (stream.action != ConversionAction::Convert) {
                    continue;
                }

                // Generate output filename
                QString baseName = QFileInfo(title.filePath).baseName();
                QString tempDir = Common::getTempDirectory();
                QString outputFile = Common::joinPath(tempDir, QString("cutebd_convert_%1_%2.mkv")
                    .arg(baseName).arg(streamIdx));

                logMessage(QString("Converting %1 stream %2 (type: %3)...")
                    .arg(title.fileName).arg(stream.index)
                    .arg(streamTypeIcon(stream.type)), "blue");

                bool success = converter.convertStream(stream, title.filePath, stream.index, outputFile,
                    [this, fileIdx](const QString& msg) {
                        // Progress callback
                    });

                if (success && Common::fileExists(outputFile)) {
                    stream.convertedPath = outputFile;
                    title.converted = true;
                    logMessage(QString("✓ Stream %1 converted successfully").arg(stream.index), "green");
                } else {
                    stream.action = ConversionAction::Drop;  // Skip on failure
                    logMessage(QString("✗ Failed to convert stream %1").arg(stream.index), "red");
                }

                processedStreams++;
                QMetaObject::invokeMethod(this, [this, processedStreams, totalStreams]() {
                    int percent = (totalStreams > 0) ? (processedStreams * 100) / totalStreams : 0;
                    progressBar->setValue(percent);
                }, Qt::QueuedConnection);
            }
        }

        // Prepare normalized output paths (either converted or original)
        for (auto& title : project.titles) {
            bool allConverted = true;
            QString lastConverted;

            for (auto& stream : title.streams) {
                if (stream.action == ConversionAction::Convert) {
                    if (!stream.convertedPath.isEmpty()) {
                        lastConverted = stream.convertedPath;
                    } else {
                        allConverted = false;
                    }
                }
            }

            // Use first converted file as the normalized output
            // In reality, we'd need to mux streams back together
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
                progressBar->setValue(progressBar->value() + 10);  // Rough progress
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

    // Verify BDMV exists
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

        QString result = builder.generateISO(bdmvPath, isoPath, projectNameEdit->text(),
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

void MainWindow::onOutputFolderBrowse()
{
    QString folder = QFileDialog::getExistingDirectory(
        this,
        "Select Output Folder",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

    if (!folder.isEmpty())
    {
        outputFolderEdit->setText(folder);
    }
}

void MainWindow::onStreamTableContextMenu(const QPoint &pos)
{
    if (selectedStreamIndex < 0)
        return;

    QMenu menu;
    menu.addAction("Keep", this, &MainWindow::onStreamActionKeep);
    menu.addAction("Convert", this, &MainWindow::onStreamActionConvert);
    menu.addAction("Drop", this, &MainWindow::onStreamActionDrop);

    menu.exec(streamTable->mapToGlobal(pos));
}

void MainWindow::onStreamActionKeep()
{
    if (selectedFileIndex >= 0 && selectedStreamIndex >= 0 && selectedFileIndex < project.titles.size())
    {
        project.titles[selectedFileIndex].streams[selectedStreamIndex].action = ConversionAction::Keep;
        updateStreamTable();
        logMessage("Stream action set to: Keep");
    }
}

void MainWindow::onStreamActionConvert()
{
    if (selectedFileIndex >= 0 && selectedStreamIndex >= 0 && selectedFileIndex < project.titles.size())
    {
        project.titles[selectedFileIndex].streams[selectedStreamIndex].action = ConversionAction::Convert;
        updateStreamTable();
        logMessage("Stream action set to: Convert");
    }
}

void MainWindow::onStreamActionDrop()
{
    if (selectedFileIndex >= 0 && selectedStreamIndex >= 0 && selectedFileIndex < project.titles.size())
    {
        project.titles[selectedFileIndex].streams[selectedStreamIndex].action = ConversionAction::Drop;
        updateStreamTable();
        logMessage("Stream action set to: Drop");
    }
}

void MainWindow::loadSettings()
{
    QSettings settings("CuteBD", "CuteBD");

    outputFolderEdit->setText(settings.value("outputFolder", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString());
    projectNameEdit->setText(settings.value("projectName", "My Blu-ray").toString());

    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}

void MainWindow::saveSettings()
{
    QSettings settings("CuteBD", "CuteBD");

    settings.setValue("outputFolder", outputFolderEdit->text());
    settings.setValue("projectName", projectNameEdit->text());
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}
