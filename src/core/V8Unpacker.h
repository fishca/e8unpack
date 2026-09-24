#ifndef V8UNPACKER_H
#define V8UNPACKER_H


#include "V8Container.h"
#include <QString>

namespace v8 {

/**
 * Высокоуровневый фасад для распаковки контейнера.
 * В дальнейшем сюда можно добавить логику обхода вложенных контейнеров,
 * декодирования заголовков форм, извлечения модулей и т.д.
 */
class V8Unpacker {
public:
    explicit V8Unpacker(const QString &inputFile);
    bool unpackTo(const QString &outputDir);

private:
    QString m_inputFile;
};

} // namespace v8


#endif // V8UNPACKER_H
