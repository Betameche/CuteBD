#include "mainwindow.hpp"
#include "models.hpp"
#include "compliance.hpp"
#include "toolchecker.hpp"

#include <QFileDialog>
#include <QFileInfo>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStandardPaths>
#include <QMenu>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

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
