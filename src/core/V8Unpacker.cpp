#include "V8Unpacker.h"
#include <QDir>

namespace v8 {

V8Unpacker::V8Unpacker(const QString &inputFile) : m_inputFile(inputFile) {}

bool V8Unpacker::unpackTo(const QString &outputDir) {
    QDir().mkpath(outputDir);

    V8Container container;
    if (!container.open(m_inputFile))
        return false;

    for (const auto &entry : container.entries()) {
        QByteArray data = container.extractEntryData(entry);
        QFile out(QDir(outputDir).filePath(entry.header.name + ".bin"));
        if (!out.open(QIODevice::WriteOnly))
            return false;
        if (out.write(data) != data.size())
            return false;
    }

    return true;
}

} // namespace v8