#ifndef FILELAYOUTBUILDER_H
#define FILELAYOUTBUILDER_H

#include "src/metadata/MetadataMapper.h"
#include <QString>

namespace v8 {

/**
 * Строит файловую структуру на диске согласно метаданным.
 */
class FileLayoutBuilder {
public:
    explicit FileLayoutBuilder(const QString &outputRoot);

    /// Записать один MappedEntry в файловую систему.
    /// Автоматически создаёт подкаталоги.
    bool writeEntry(const MetadataMapper::MappedEntry &entry);

    /// Записать все записи.
    bool writeAll(const QVector<MetadataMapper::MappedEntry> &entries);

private:
    QString m_root;
};

} // namespace v8

#endif // FILELAYOUTBUILDER_H
