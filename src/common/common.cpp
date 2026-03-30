#include "common.hpp"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QDebug>

namespace Common
{

    ProcessResult runCommand(
        const QString &command,
        const QStringList &args,
        std::function<void(const QString &)> onOutput,
        int timeoutMs)
    {
        ProcessResult result;
        result.command = command;
        result.args = args;

        QProcess process;
        process.setProcessChannelMode(QProcess::SeparateChannels);

        process.start(command, args);

        if (!process.waitForStarted())
        {
            result.success = false;
            result.error = QString("Failed to start process: %1").arg(command);
            return result;
        }

        if (!process.waitForFinished(timeoutMs))
        {
            process.kill();
            result.success = false;
            result.error = QString("Process timeout after %1ms").arg(timeoutMs);
            return result;
        }

        result.exitCode = process.exitCode();
        result.success = (result.exitCode == 0);
        result.stdOut = process.readAllStandardOutput();
        result.stdErr = process.readAllStandardError();

        if (onOutput && !result.stdOut.isEmpty())
        {
            for (const auto &line : result.stdOut.split('\n'))
            {
                if (!line.isEmpty())
                {
                    onOutput(line);
                }
            }
        }

        return result;
    }

    bool directoryExists(const QString &path)
    {
        return QDir(path).exists();
    }

    bool fileExists(const QString &path)
    {
        return QFile::exists(path);
    }

    bool createDirectoryRecursive(const QString &path)
    {
        return QDir().mkpath(path);
    }

    QString joinPath(const QString &base, const QString &relative)
    {
        return QDir(base).filePath(relative);
    }

    Result<QString> readFile(const QString &filePath)
    {
        Result<QString> result;
        QFile file(filePath);

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            result.error = QString("Cannot open file: %1").arg(filePath);
            return result;
        }

        result.value = file.readAll();
        result.success = true;
        file.close();

        return result;
    }

    Result<void> writeFile(const QString &filePath, const QString &content)
    {
        Result<void> result;
        QFile file(filePath);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            result.error = QString("Cannot open file for writing: %1").arg(filePath);
            return result;
        }

        file.write(content.toUtf8());
        file.close();
        result.success = true;

        return result;
    }

    QString normalizePath(const QString &path)
    {
        return QDir::toNativeSeparators(path);
    }

    QString getTempDirectory()
    {
        return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    }

}
