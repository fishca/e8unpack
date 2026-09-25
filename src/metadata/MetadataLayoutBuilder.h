#ifndef METADATALAYOUTBUILDER_H
#define METADATALAYOUTBUILDER_H

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "MetadataMap.h"
#include <QString>
#include <filesystem>

namespace v8 {

/**
 * Обходит дерево, созданное v8unpack::Parse, и переименовывает
 * каталоги/файлы с GUID-именами в человекочитаемые имена из MetadataMap.
 *
 * Работает in-place: переименовывает прямо в дереве.
 */
class MetadataLayoutBuilder {
public:
    MetadataLayoutBuilder(const MetadataMap &map, const QString &rootDir);

    /// Обойти дерево и переименовать всё, что найдено в карте.
    /// @return false, если корневой каталог недоступен
    bool reorganize();

    /// Сколько имён удалось сопоставить (для отчёта).
    int renamedCount() const { return m_renamed; }

private:
    void processDirectory(const std::filesystem::path &dir);

    /// Найти новое имя для GUID. Пустая строка = не найдено.
    QString resolveName(const QString &guid) const;

    /// Заменить недопустимые в имени файла символы.
    static QString sanitizeFileName(const QString &name);

    const MetadataMap &m_map;
    std::filesystem::path m_root;
    int m_renamed = 0;
};

} // namespace v8

#endif // METADATALAYOUTBUILDER_H