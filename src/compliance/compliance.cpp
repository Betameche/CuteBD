#include "compliance.hpp"
#include <QMap>
#include <QStringList>
#include <QVector>
#include <cmath>

namespace Compliance
{

    // ISO 639-3 language codes to full names (common ones)
    static const QMap<QString, QString> ISO_639_TO_NAME = {
        {"eng", "English"},
        {"fra", "French"},
        {"deu", "German"},
        {"spa", "Spanish"},
        {"ita", "Italian"},
        {"jpn", "Japanese"},
        {"zho", "Chinese"},
        {"rus", "Russian"},
        {"kor", "Korean"},
        {"por", "Portuguese"},
        {"pol", "Polish"},
        {"nld", "Dutch"},
        {"swe", "Swedish"},
        {"nor", "Norwegian"},
        {"dan", "Danish"},
        {"fin", "Finnish"},
        {"und", "Undetermined"},
    };

    QString languageNameFromCode(const QString &isoCode)
    {
        QString code = isoCode.toLower();
        if (ISO_639_TO_NAME.contains(code))
        {
            return ISO_639_TO_NAME.value(code);
        }
        return code.isEmpty() ? "Undetermined" : code;
    }

    QString isoCodeFromLanguageName(const QString &name)
    {
        for (auto it = ISO_639_TO_NAME.begin(); it != ISO_639_TO_NAME.end(); ++it)
        {
            if (it.value() == name)
            {
                return it.key();
            }
        }
        return "und";
    }

    bool isCompliantAudioCodec(const QString &codecName)
    {
        // Blu-ray audio codecs: AC3, E-AC3, DTS, DTS-MA, LPCM, MPEG-1/2 Audio, AAC
        static const QStringList compliant = {
            "ac3",
            "eac3",   // Dolby
            "dts",    // DTS (base and variants)
            "truehd", // Dolby True HD
            "pcm_s16be",
            "pcm_s24be", // LPCM
            "mp2",       // MPEG Audio Layer 2
            "aac",       // AAC
        };

        QString lower = codecName.toLower();
        for (const auto &c : compliant)
        {
            if (lower.contains(c))
                return true;
        }
        return false;
    }

    bool isCompliantFramerate(const QString &fpsStr)
    {
        // Blu-ray compliant framerates (in Hz): 23.976, 24, 25, 29.97, 30, 50, 59.94, 60
        double fps = fpsStr.toDouble();

        // Allow small tolerance for floating point
        constexpr double TOLERANCE = 0.1;

        static const QVector<double> compliantRates = {
            23.976, 24.0, 25.0, 29.97, 30.0, 50.0, 59.94, 60.0};

        for (double rate : compliantRates)
        {
            if (std::abs(fps - rate) < TOLERANCE)
            {
                return true;
            }
        }

        return false;
    }

    bool isCompliantSubtitleCodec(const QString &codecName)
    {
        // Blu-ray compliant subtitle codecs
        // PGS (bitmap), VOBSUB (image), ASS/SSA (text -> convert to PGS)
        static const QStringList compliant = {
            "hdmv_pgs_subtitle", // Native PGS
            "dvb_subtitle",      // DVB bitmap subtitle
            "vobsub",            // DVD subtitle
            "ass",               // Can be converted to PGS
            "ssa",               // Can be converted to PGS
            "subrip",            // SRT can be converted to PGS
            "webvtt",            // VTT can be converted to PGS
        };

        QString lower = codecName.toLower();
        for (const auto &c : compliant)
        {
            if (lower.contains(c))
                return true;
        }

        return false;
    }

}
