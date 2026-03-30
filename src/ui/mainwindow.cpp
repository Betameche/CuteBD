#include "mainwindow.hpp"
#include "toolchecker.hpp"

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
#include <QSettings>
#include <QGroupBox>
#include <QStatusBar>
#include <QStandardPaths>

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
    QWidget *central = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    QGroupBox *filesGroup = new QGroupBox("Input Files", this);
    QVBoxLayout *filesLayout = new QVBoxLayout(filesGroup);

    fileTable = new QTableWidget(this);
    fileTable->setColumnCount(6);
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

    toolStatusLabel = new QLabel("Checking tools...", this);
    toolStatusLabel->setStyleSheet("color: orange;");

    progressBar = new QProgressBar(this);
    progressBar->setValue(0);
    progressBar->setVisible(false);

    logOutput = new QTextEdit(this);
    logOutput->setReadOnly(true);
    logOutput->setMaximumHeight(150);
    logOutput->setFont(QFont("Courier", 8));

    mainLayout->addWidget(filesGroup, 1);
    mainLayout->addWidget(streamGroup, 1);
    mainLayout->addWidget(pipelineGroup);
    mainLayout->addWidget(outputGroup);
    mainLayout->addWidget(toolStatusLabel);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(logOutput);

    setCentralWidget(central);
    statusBar()->showMessage("Ready");
}

void MainWindow::connectSignals()
{
    connect(addFilesBtn, &QPushButton::clicked, this, &MainWindow::onAddFiles);
    connect(removeFileBtn, &QPushButton::clicked, this, &MainWindow::onRemoveSelected);
    connect(clearAllBtn, &QPushButton::clicked, this, &MainWindow::onClearAll);

    connect(fileTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onFileSelectionChanged);

    connect(streamTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::onStreamTableContextMenu);
    connect(streamTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onStreamSelectionChanged);

    connect(inspectBtn, &QPushButton::clicked, this, &MainWindow::onInspectStreams);
    connect(convertBtn, &QPushButton::clicked, this, &MainWindow::onStartConversion);
    connect(buildBDMVBtn, &QPushButton::clicked, this, &MainWindow::onBuildBDMV);
    connect(generateISOBtn, &QPushButton::clicked, this, &MainWindow::onGenerateISO);

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

    QTextCursor cursor = logOutput->textCursor();
    cursor.movePosition(QTextCursor::End);
    logOutput->setTextCursor(cursor);
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
