#pragma once

#include <QMainWindow>
#include <QVector>
#include <QMap>
#include <QFuture>
#include <QFutureWatcher>
#include "models.hpp"
#include "toolchecker.hpp"

class QTableWidget;
class QTableWidgetItem;
class QLabel;
class QPushButton;
class QLineEdit;
class QProgressBar;
class QComboBox;
class QCheckBox;
class QTextEdit;
class QSplitter;

// Main application window
// Manages file list, stream inspection UI, conversion controls, and pipeline orchestration
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // File operations
    void onAddFiles();
    void onRemoveSelected();
    void onClearAll();

    // Stream inspection
    void onInspectStreams();
    void onFileSelectionChanged();
    void onStreamSelectionChanged();

    // Conversion pipeline
    void onStartConversion();
    void onBuildBDMV();
    void onGenerateISO();
    void onOutputFolderBrowse();

    // Async pipeline completion handlers
    void onInspectionFinished();
    void onConversionFinished();
    void onBDMVBuildFinished();
    void onISOGenerationFinished();

    // Context menu on stream table
    void onStreamTableContextMenu(const QPoint &pos);
    void onStreamActionKeep();
    void onStreamActionConvert();
    void onStreamActionDrop();

private:
    // Initialization
    void initializeUI();
    void connectSignals();
    void checkToolAvailability();
    void loadSettings();
    void saveSettings();

    // UI updates
    void updateFileTable();
    void updateStreamTable();
    void updateToolStatus();
    void updatePipelineButtons();
    void logMessage(const QString &msg, const QString &color = "black");

    // Compliance visualization
    QString complianceBadge(ComplianceStatus status) const;
    QString streamTypeIcon(StreamType type) const;
    QString conversionActionText(ConversionAction action) const;

    // Async operations
    void inspectAllStreamsAsync();

private:
    // Data
    BlurayProject project;
    QMap<QString, Tools::ToolStatus> toolStatus;
    int selectedFileIndex = -1;
    int selectedStreamIndex = -1;

    // UI Widgets
    // File list section
    QTableWidget *fileTable = nullptr;
    QPushButton *addFilesBtn = nullptr;
    QPushButton *removeFileBtn = nullptr;
    QPushButton *clearAllBtn = nullptr;

    // Stream details section
    QTableWidget *streamTable = nullptr;
    QLabel *fileDetailsLabel = nullptr;

    // Tool status section
    QLabel *toolStatusLabel = nullptr;

    // Control buttons
    QPushButton *inspectBtn = nullptr;
    QPushButton *convertBtn = nullptr;
    QPushButton *buildBDMVBtn = nullptr;
    QPushButton *generateISOBtn = nullptr;

    // Output parameters
    QLineEdit *outputFolderEdit = nullptr;
    QPushButton *outputFolderBrowseBtn = nullptr;
    QLineEdit *projectNameEdit = nullptr;

    // Status display
    QProgressBar *progressBar = nullptr;
    QTextEdit *logOutput = nullptr;

    // Async operation tracking
    enum class PipelineState
    {
        Idle,
        Inspecting,
        Converting,
        BuildingBDMV,
        GeneratingISO
    };

    PipelineState pipelineState = PipelineState::Idle;
    QFutureWatcher<void> *inspectionWatcher = nullptr;
    QFutureWatcher<void> *conversionWatcher = nullptr;
    QFutureWatcher<void> *bdmvWatcher = nullptr;
    QFutureWatcher<void> *isoWatcher = nullptr;
};
