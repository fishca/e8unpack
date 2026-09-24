#include "StructuredPacker.h"
#include "src/core/MetadataTypes.h"
#include "src/metadata/SectionTypes.h"
#include "src/core/V8File.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <system_error>
#include <QRegularExpression>   // ← добавить
#include <QSet>                 // ← на всякий случай (QSet используется в knownFolders)

namespace v8 {
namespace fs = std::filesystem;

StructuredPacker::StructuredPacker(const QString& inputDir,
                                   const QString& outputFile,
                                   bool noDeflate)
    : m_inputDir(inputDir)
    , m_outputFile(outputFile)
    , m_tempDir(QDir(inputDir).filePath(".tmp_pack"))
    , m_noDeflate(noDeflate)
{}

// Собираем множество «человеческих» имён типов и секций, чтобы не спутать их
// с реальными GUID-файлами.
static const QSet<QString>& knownFolders()
{
    static QSet<QString> s;
    if (!s.isEmpty()) return s;

    for (auto it = metadataTypes().begin(); it != metadataTypes().end(); ++it) {
        const QString& path = it.value();
        // Могут быть составные пути "Общие/Языки" — берём только последний сегмент.
        const int slash = path.lastIndexOf(QLatin1Char('/'));
        s.insert(slash >= 0 ? path.mid(slash + 1) : path);
        // И верхний уровень тоже
        if (slash > 0) s.insert(path.left(slash));
    }
    for (auto it = sectionTypes().begin(); it != sectionTypes().end(); ++it)
        s.insert(it.value());

    s.insert(QStringLiteral("Конфигурация"));
    return s;
}

// Проверяем, является ли имя GUID-ом (с учётом возможного суффикса .N)
static bool isGuidLike(const QString& name)
{
    static const QRegularExpression re(
        R"(^[0-9a-fA-F]{8}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{12}(\.\d+)?$)");
    return re.match(name).hasMatch();
}

bool StructuredPacker::flattenToTemp()
{
    std::error_code ec;
    fs::remove_all(m_tempDir.toStdString(), ec);
    fs::create_directories(m_tempDir.toStdString(), ec);

    const fs::path srcRoot(m_inputDir.toStdString());
    const fs::path tmpRoot(m_tempDir.toStdString());

    int copied = 0;

    fs::recursive_directory_iterator it(
        srcRoot, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;

    for (; it != end; it.increment(ec)) {
        if (ec) break;

        if (!it->is_regular_file(ec))
            continue;

        const fs::path& src = it->path();

        // Пропускаем наш временный каталог
        auto rel = fs::relative(src, srcRoot, ec);
        if (ec) continue;
        const QString relStr = QString::fromStdString(rel.generic_string());
        if (relStr.startsWith(QLatin1String(".tmp_pack")))
            continue;

        const QString fileName = QString::fromUtf8(
            src.filename().string().c_str());

        const bool isService = (fileName == QStringLiteral("root")
                             || fileName == QStringLiteral("version")
                             || fileName == QStringLiteral("versions"));

        if (!isService && !isGuidLike(fileName))
            continue;   // не наш файл — пропускаем

        fs::path dst = tmpRoot / fileName.toStdString();

        if (fs::exists(dst, ec)) {
            int n = 1;
            fs::path alt;
            do {
                alt = dst;
                alt += ("." + std::to_string(n++));
            } while (fs::exists(alt, ec));
            dst = alt;
        }

        fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
        if (!ec) {
            ++copied;
            if (isService) {
                qDebug() << "StructuredPacker: служебный файл найден:"
                         << QString::fromStdString(rel.generic_string())
                         << "->" << fileName;
            }
        } else {
            qWarning() << "Не удалось скопировать"
                       << QString::fromStdString(src.string())
                       << "->" << QString::fromStdString(dst.string())
                       << ":" << QString::fromStdString(ec.message());
        }
    }

    qDebug() << "StructuredPacker: скопировано файлов:" << copied;

    // Диагностика: что в итоге лежит в .tmp_pack
    {
        QStringList names;
        for (const auto& e : fs::directory_iterator(tmpRoot, ec))
            names << QString::fromStdString(e.path().filename().string());
        names.sort();
        qDebug() << "StructuredPacker: содержимое .tmp_pack:"
                 << names.join(QStringLiteral(", "));
    }

    const bool hasRoot = fs::exists(tmpRoot / "root", ec);
    if (!hasRoot) {
        qWarning() << "StructuredPacker: в .tmp_pack нет файла root.";
        return false;
    }

    const bool hasVersion = fs::exists(tmpRoot / "version", ec);
    if (!hasVersion) {
        qWarning() << "StructuredPacker: в .tmp_pack нет файла version.";
    }

    return true;
}




void StructuredPacker::cleanupTemp()
{
    std::error_code ec;
    fs::remove_all(m_tempDir.toStdString(), ec);
}

bool StructuredPacker::run()
{
    if (!QFileInfo::exists(m_inputDir)) {
        qWarning() << "StructuredPacker: каталог не найден:" << m_inputDir;
        return false;
    }

    if (!flattenToTemp()) {
        cleanupTemp();
        return false;
    }

    // Вызываем BuildCfFile из v8unpack
    int ret = v8unpack::BuildCfFile(
        m_tempDir.toStdString(),
        m_outputFile.toStdString(),
        m_noDeflate);

    cleanupTemp();

    if (ret != v8unpack::V8UNPACK_OK) {
        qWarning() << "StructuredPacker: BuildCfFile вернул код" << ret;
        return false;
    }

    return true;
}

} // namespace v8