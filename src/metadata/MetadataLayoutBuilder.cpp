// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include "MetadataLayoutBuilder.h"
#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>
#include <system_error>

namespace v8 {

namespace fs = std::filesystem;

MetadataLayoutBuilder::MetadataLayoutBuilder(const MetadataMap &map,
                                             const QString &rootDir)
    : m_map(map)
    , m_root(rootDir.toStdString())
{}

QString MetadataLayoutBuilder::sanitizeFileName(const QString &name)
{
    // Windows: нельзя < > : " / \ | ? *  и служебные имена (CON, PRN, ...)
    QString result = name;
    result.replace(QRegularExpression(R"([<>:"/\\|?*\x00-\x1f])"), "_");
    result = result.trimmed();
    if (result.isEmpty())
        result = "unnamed";
    return result;
}

QString MetadataLayoutBuilder::resolveName(const QString &guid) const
{
    auto info = m_map.lookup(guid);
    if (!info.has_value())
        return QString();

    // Формат пути: <Тип>/<Имя>  — например "Catalogs/Номенклатура"
    return info->typeName + "/" + sanitizeFileName(info->objectName);
}

void MetadataLayoutBuilder::processDirectory(const fs::path &dir)
{
    std::error_code ec;

    // Собираем план переименований: сначала читаем, потом меняем.
    // Так избегаем инвалидации итератора при переименовании.
    struct RenamePlan {
        fs::path src;
        fs::path dst;
    };
    std::vector<RenamePlan> plan;

    for (const auto &entry : fs::directory_iterator(dir, ec)) {
        if (ec) break;

        const std::string name = entry.path().filename().string();
        const QString qname = QString::fromStdString(name);

        const QString newRelative = resolveName(qname);
        if (newRelative.isEmpty())
            continue; // не GUID из карты — оставляем как есть

        // newRelative = "Catalogs/Номенклатура" → создаём вложенные каталоги
        fs::path dst = dir / newRelative.toStdString();
        plan.push_back({ entry.path(), dst });
    }

    // Применяем переименования
    for (auto &item : plan) {
        std::error_code renameEc;

        // Создаём промежуточные каталоги (Catalogs/, Documents/, ...)
        fs::create_directories(item.dst.parent_path(), renameEc);

        fs::rename(item.src, item.dst, renameEc);
        if (renameEc) {
            qWarning() << "Не удалось переименовать"
                       << QString::fromStdString(item.src.string())
                       << "->"
                       << QString::fromStdString(item.dst.string())
                       << ":"
                       << QString::fromStdString(renameEc.message());
            continue;
        }
        ++m_renamed;

        // Рекурсивно обрабатываем содержимое переименованного каталога
        std::error_code isDirEc;
        if (fs::is_directory(item.dst, isDirEc)) {
            processDirectory(item.dst);
        }
    }
}

bool MetadataLayoutBuilder::reorganize()
{
    std::error_code ec;
    if (!fs::exists(m_root, ec) || !fs::is_directory(m_root, ec)) {
        return false;
    }
    processDirectory(m_root);
    return true;
}

} // namespace v8