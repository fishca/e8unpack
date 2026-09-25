// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
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
