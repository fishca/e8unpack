#ifndef COMMANDLINEPARSER_H
#define COMMANDLINEPARSER_H

#include <QString>
#include <QStringList>

namespace v8 {

struct CommandLineOptions {
    QString inputFile;        // .cf / .cfe / .epf
    QString outputDir;        // каталог для распаковки
    QString metadataMapFile;  // JSON-карта метаданных (опционально)
    bool    useMetadata = false;
    bool    listOnly = false;
    bool    parseMode = false;
    bool    buildMode = false;   // <-- новый
    bool    noDeflate = false;   // <-- новый (--no-deflate)
};

class CommandLineParser {
public:
    static CommandLineOptions parse(const QStringList &args);
    static void printUsage();
};

} // namespace v8

#endif // COMMANDLINEPARSER_H
