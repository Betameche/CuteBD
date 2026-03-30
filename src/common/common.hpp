#pragma once

#include <QString>
#include <QStringList>
#include <functional>

namespace Common
{

    // Result type for operations that can fail
    template <typename T>
    struct Result
    {
        bool success = false;
        T value = T();
        QString error;

        explicit operator bool() const { return success; }
    };

    // Template specialization for void result
    template <>
    struct Result<void>
    {
        bool success = false;
        QString error;

        explicit operator bool() const { return success; }
    };

    // Process execution with output capture
    struct ProcessResult
    {
        bool success = false;
        int exitCode = -1;
        QString stdOut;
        QString stdErr;
        QString command;
        QStringList args;
        QString error; // System error if process failed to start
    };

    // Run a command and capture output
    // callback is called periodically with output lines
    ProcessResult runCommand(
        const QString &command,
        const QStringList &args = {},
        std::function<void(const QString &)> onOutput = nullptr,
        int timeoutMs = 30000);

    // Simple path utilities
    bool directoryExists(const QString &path);
    bool fileExists(const QString &path);
    bool createDirectoryRecursive(const QString &path);
    QString joinPath(const QString &base, const QString &relative);

    // File I/O
    Result<QString> readFile(const QString &filePath);
    Result<void> writeFile(const QString &filePath, const QString &content);

    // Normalize path separators for current platform
    QString normalizePath(const QString &path);

    // Get safe temporary directory
    QString getTempDirectory();

}
