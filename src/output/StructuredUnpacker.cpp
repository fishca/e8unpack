#include "StructuredUnpacker.h"
#include "src/core/MetadataTypes.h"
#include "src/core/V8File.h"   // v8unpack::Parse
#include "src/metadata/ConfigStructureReader.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDebug>
#include <system_error>
#include <vector>

namespace v8 {
namespace fs = std::filesystem;

static bool looksLikeGuid(const QString &name)
{
    static const QRegularExpression re(
        R"(^[0-9a-fA-F]{8}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{4}-?[0-9a-fA-F]{12}$)");
    return re.match(name).hasMatch();
}

StructuredUnpacker::StructuredUnpacker(const QString &inputFile,
                                       const QString &outputDir,
                                       const MetadataMap *map)
    : m_inputFile(inputFile)
    , m_outputDir(outputDir)
    , m_tempDir(QDir(outputDir).filePath(".tmp_unpack"))
    , m_map(map)
{}

bool StructuredUnpacker::isServiceName(const QString &name)
{
    static const QStringList kService = {
        "root", "version", "versions",
        "_config", "config", "configinfo",
        "metadata", "ConfigDumpInfo"
    };
    if (!looksLikeGuid(name))
        return true;   // всё, что не GUID — служебное
    return kService.contains(name);
}

bool StructuredUnpacker::loadObjectNames()
{
    QString configDir = m_tempDir;

    const QString serviceDir = QDir(m_tempDir).filePath(serviceFolderName());
    const QString rootInService = QDir(serviceDir).filePath(QStringLiteral("root"));
    const QString rootInTemp    = QDir(m_tempDir).filePath(QStringLiteral("root"));

    if (QFileInfo::exists(rootInService))
        configDir = serviceDir;
    else if (QFileInfo::exists(rootInTemp))
        configDir = m_tempDir;
    else {
        configDir = findRootDir();
        if (configDir.isEmpty()) {
            qWarning() << "StructuredUnpacker: файл root не найден в" << m_tempDir;
            return false;
        }
    }

    qDebug() << "StructuredUnpacker: файлы структуры в" << configDir;

    ConfigStructureReader reader(configDir.toStdString());

    if (!reader.loadRoot()) { qWarning() << "... root fail"; return false; }
    if (!reader.loadObjectGroups()) { qWarning() << "... groups fail"; return false; }

    m_configGuid = reader.configGuid();
    m_nameCache  = reader.resolveAllNames();

    m_groups_cache = reader.groups();

    // Заполняем m_typeOf из групп
    m_typeOf.clear();
    for (const auto& g : reader.groups()) {
        for (const auto& objGuid : g.objectGuids) {
            m_typeOf.insert(objGuid.toLower(), g.typeName);
        }
    }

    qDebug() << "StructuredUnpacker: имён:" << m_nameCache.size()
             << "типов:" << m_typeOf.size()
             << "configGuid:" << m_configGuid;

    return !m_nameCache.isEmpty();
}





QString StructuredUnpacker::objectName(const QString &guid) const
{
    const QString norm = normalizeGuid(guid);

    // 1) Из кэша, полученного разбором конфигурации
    auto it = m_nameCache.find(norm);
    if (it != m_nameCache.end() && !it.value().isEmpty())
        return it.value();

    // 2) Из внешней карты метаданных (если есть)
    if (m_map && !m_map->isEmpty()) {
        auto info = m_map->lookup(guid);
        if (info.has_value())
            return info->objectName;
    }

    // 3) Fallback — сам GUID
    return guid;
}

QString StructuredUnpacker::findRootDir() const
{
    std::error_code ec;
    const fs::path tempRoot(m_tempDir.toStdString());

    if (!fs::exists(tempRoot, ec))
        return {};

    fs::recursive_directory_iterator it(
        tempRoot, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;

    for (; it != end; it.increment(ec)) {
        if (ec) break;

        if (it->is_regular_file(ec)
            && it->path().filename() == "root")
        {
            return QString::fromStdString(
                it->path().parent_path().string());
        }
    }
    return {};
}

QString StructuredUnpacker::baseGuidFromFileName(const QString &fileName)
{
    // Отрезаем суффикс .N, если он есть и является числом
    const int dotPos = fileName.lastIndexOf(QLatin1Char('.'));
    if (dotPos > 0) {
        bool ok = false;
        fileName.mid(dotPos + 1).toInt(&ok);
        if (ok)
            return fileName.left(dotPos);
    }
    return fileName;
}

bool StructuredUnpacker::indexChildElements()
{
    m_childIndex.clear();

    if (m_configGuid.isEmpty()) {
        qWarning() << "StructuredUnpacker: configGuid не определён";
        return false;
    }

    // configDir — там, где мы нашли root
    const QString configDir = QDir(m_tempDir).filePath(serviceFolderName());
    const QString configDirAlt = m_tempDir;

    const QString effectiveDir =
        QFileInfo::exists(QDir(configDir).filePath("root"))
            ? configDir
            : configDirAlt;

    ConfigStructureReader reader(effectiveDir.toStdString());

    // Ищем все верхнеуровневые объекты
    for (const auto& g : m_groups_cache) {         // см. ниже
        for (const QString& objGuid : g.objectGuids) {
            const auto sections = reader.resolveSections(objGuid);

            for (const auto& sec : sections) {
                for (const QString& childGuid : sec.elementGuids) {
                    ChildInfo info;
                    info.parentGuid  = objGuid;
                    info.parentName  = m_nameCache.value(objGuid.toLower(), objGuid);
                    info.parentType  = g.typeName;
                    info.sectionName = sec.sectionName;

                    m_childIndex.insert(childGuid.toLower(), info);
                }
            }
        }
    }

    qDebug() << "StructuredUnpacker: дочерних элементов проиндексировано:"
             << m_childIndex.size();
    return !m_childIndex.isEmpty();

}

QString StructuredUnpacker::targetRelativePath(
    const fs::path &rel, const QString &guid, const QString &parentGuid) const
{
    const int depth = static_cast<int>(std::distance(rel.begin(), rel.end()));

    // ── 1. Верхний уровень: всё не-GUID → Конфигурация/, GUID типа → ИмяТипа/
    if (depth == 1) {
        if (guid.isEmpty())
            return serviceFolderName() + "/" + QString::fromStdString(rel.filename().string());

        const auto &types = metadataTypes();
        auto it = types.find(normalizeGuid(guid));
        if (it != types.end())
            return it.value();
        return serviceFolderName() + "/" + guid;
    }

    // ── 2. Второй уровень: GUID объекта внутри типа
    if (depth == 2 && !parentGuid.isEmpty()) {
        const auto &types = metadataTypes();
        auto typeIt = types.find(normalizeGuid(parentGuid));
        const QString typeName =
            (typeIt != types.end()) ? typeIt.value() : serviceFolderName();

        return typeName + "/" + objectName(guid);
    }

    // ── 3+ Глубже: определяем корень (тип/объект) и к нему клеим хвост
    // Идём по rel и находим первые два компонента — это <type_guid>/<obj_guid>.
    auto it = rel.begin();
    const std::string typeGuid = it->string(); ++it;
    const std::string objGuid  = (it != rel.end()) ? it->string() : std::string();

    if (objGuid.empty())
        return QString::fromStdString(rel.generic_string());

    const auto &types = metadataTypes();
    auto typeIt = types.find(normalizeGuid(QString::fromStdString(typeGuid)));
    const QString typeName =
        (typeIt != types.end()) ? typeIt.value() : serviceFolderName();

    const QString objName = objectName(QString::fromStdString(objGuid));

    // Собираем хвост пути от третьего компонента и дальше
    fs::path tail;
    for (++it; it != rel.end(); ++it)
        tail /= *it;

    QString result = typeName + "/" + objName;
    if (!tail.empty())
        result += "/" + QString::fromStdString(tail.generic_string());

    return result;
}

bool StructuredUnpacker::unpackToTemp()
{
    std::error_code ec;
    fs::remove_all(m_tempDir.toStdString(), ec);   // чистим, если что-то осталось

    QDir().mkpath(m_tempDir);

    std::vector<std::string> filter;
    int ret = v8unpack::Parse(
        m_inputFile.toStdString(),
        m_tempDir.toStdString(),
        filter);

    return ret == v8unpack::V8UNPACK_OK;
}

bool StructuredUnpacker::buildAndApplyPlan()
{
    std::error_code ec;
    const fs::path tempRoot(m_tempDir.toStdString());
    const fs::path targetRoot(m_outputDir.toStdString());

    if (!fs::exists(tempRoot, ec))
        return false;

    // ── 0. Заранее создаём все каталоги верхнего уровня из справочника ──
    // Это гарантирует, что структура будет одинаковой независимо от того,
    // какие метаданные реально есть в конфигурации.
    {
        // Конфигурация — всегда
        fs::create_directories(targetRoot / serviceFolderName().toStdString(), ec);

        // Все типы из metadataTypes()
        const auto& types = metadataTypes();
        for (auto it = types.begin(); it != types.end(); ++it) {
            const QString& typeName = it.value();
            if (typeName.isEmpty())
                continue;

            fs::create_directories(
                targetRoot / typeName.toStdString(), ec);
        }
    }


    // Проходим по всем файлам в корне .tmp_unpack/ (не рекурсивно!)
    struct MoveItem { fs::path src; fs::path dst; };
    std::vector<MoveItem> plan;

    for (const auto& entry : fs::directory_iterator(tempRoot, ec)) {
        if (ec) break;

        const fs::path& src = entry.path();
        const QString name = QString::fromStdString(src.filename().string());

        QString targetRel;
        const QString baseGuid = baseGuidFromFileName(name);
        const QString key = baseGuid.toLower();

        // ── 1. Служебные файлы конфигурации
        if (name == QLatin1String("root")
            || name == QLatin1String("version")
            || name == QLatin1String("versions")
            || name == m_configGuid)
        {
            targetRel = serviceFolderName() + "/" + name;
        }
        // ── 2. Верхнеуровневый объект метаданных
        else if (m_typeOf.contains(key)) {
            const QString typeName = m_typeOf.value(key);
            const QString objName  = m_nameCache.value(key, baseGuid);
            targetRel = typeName + "/" + objName + "/" + name;
        }
        // ── 3. Дочерний элемент (форма, реквизит, ...) — знаем родителя
        else if (m_childIndex.contains(key)) {
            const ChildInfo info = m_childIndex.value(key);
            targetRel = info.parentType + "/" + info.parentName + "/"
                      + info.sectionName + "/" + name;
        }
        // ── 4. Всё остальное — в Конфигурация/
        else {
            targetRel = serviceFolderName() + "/" + name;
        }

        fs::path dst = targetRoot / targetRel.toStdString();
        plan.push_back({ src, dst });
    }
    // Применяем
    for (const auto& item : plan) {
        std::error_code e2;
        fs::create_directories(item.dst.parent_path(), e2);

        if (fs::exists(item.dst, e2)) {
            fs::path alt = item.dst;
            int n = 1;
            while (fs::exists(alt, e2)) {
                alt = item.dst;
                alt += ("." + std::to_string(n++));
            }
            fs::rename(item.src, alt, e2);
        } else {
            fs::rename(item.src, item.dst, e2);
            if (e2) {
                fs::copy(item.src, item.dst,
                         fs::copy_options::recursive |
                         fs::copy_options::overwrite_existing, e2);
                if (!e2)
                    fs::remove_all(item.src, e2);
            }
        }

        if (!e2)
            ++m_moved;
        else
            qWarning() << "Не удалось перенести"
                       << QString::fromStdString(item.src.string())
                       << "->" << QString::fromStdString(item.dst.string())
                       << ":" << QString::fromStdString(e2.message());
    }

    return true;
}




void StructuredUnpacker::cleanupTemp()
{
    std::error_code ec;
    fs::remove_all(m_tempDir.toStdString(), ec);
}

bool StructuredUnpacker::run()
{
    QDir().mkpath(m_outputDir);

    if (!unpackToTemp())
        return false;

    if (!loadObjectNames())
        qWarning() << "StructuredUnpacker: имена объектов не загружены";

    // ↓ новый шаг: индексируем содержимое объектов
    if (!indexChildElements())
        qWarning() << "StructuredUnpacker: дочерние элементы не проиндексированы";

    if (!buildAndApplyPlan()) {
        cleanupTemp();
        return false;
    }

    cleanupTemp();
    return true;
}

} // namespace v8