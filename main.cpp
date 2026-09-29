// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include <QCoreApplication>

#include <QStringConverter>
#include <QTextStream>
#include "src/cli/CommandLineParser.h"
#include "src/core/V8Container.h"
#include "src/core/V8Unpacker.h"
#include "src/core/V8File.h"

#include "src/metadata/MetadataMap.h"
#include "src/metadata/MetadataMapper.h"
#include "src/output/FileLayoutBuilder.h"
#include "src/metadata/MetadataLayoutBuilder.h"

#include "src/output/StructuredUnpacker.h"
#include "src/output/StructuredPacker.h"


#include <clocale>
#include <locale>
#include <iostream>

#ifdef _WIN32
#  define NOMINMAX
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <winnls.h>
#  ifndef CP_UTF8
#    define CP_UTF8 65001
#  endif
#endif

static void consoleMessageHandler(QtMsgType type,
                                  const QMessageLogContext&,
                                  const QString& msg)
{
    const char* prefix = "";
    switch (type) {
        case QtDebugMsg:    prefix = "[D] "; break;
        case QtInfoMsg:     prefix = "[I] "; break;
        case QtWarningMsg:  prefix = "[W] "; break;
        case QtCriticalMsg: prefix = "[C] "; break;
        case QtFatalMsg:    prefix = "[F] "; break;
    }
    QTextStream(stderr) << prefix << msg << '\n';
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    qInstallMessageHandler(consoleMessageHandler);
    // Устанавливаем системную локаль для C/C++ потоков
    std::setlocale(LC_ALL, "");
    QCoreApplication app(argc, argv);
    QTextStream err(stderr);
    QTextStream out(stdout);

    err.setEncoding(QStringConverter::Utf8);
    out.setEncoding(QStringConverter::Utf8);

    v8::CommandLineOptions opts = v8::CommandLineParser::parse(app.arguments());

    if (opts.inputFile.isEmpty()) {
        v8::CommandLineParser::printUsage();
        return 1;
    }

    if (opts.outputDir.isEmpty()) {
        err << "Ошибка: не указан каталог вывода.\n";
        return 2;
    }

    // ── Режим сборки ─────────────────────────────────────────────
    if (opts.buildMode) {
        if (opts.outputDir.isEmpty() || opts.inputFile.isEmpty()) {
            err << "Ошибка: -B требует <input_dir> <output.cf>\n";
            return 2;
        }

        out << "Сборка контейнера: " << opts.outputDir
            << " -> " << opts.inputFile << "\n";

        v8::StructuredPacker packer(opts.outputDir, opts.inputFile, opts.noDeflate);
        if (!packer.run()) {
            err << "Ошибка сборки.\n";
            return 5;
        }

        out << "Готово: " << opts.inputFile << "\n";
        out.flush();
        return 0;
    }

    // ── Загрузка карты метаданных ────────────────────────────────
        v8::MetadataMap map;
        bool mapLoaded = false;
        if (opts.useMetadata && !opts.metadataMapFile.isEmpty()) {
            mapLoaded = map.loadFromJson(opts.metadataMapFile);
            if (!mapLoaded)
                err << "Предупреждение: карта метаданных не загружена. "
                    << "Имена объектов будут по GUID.\n";
        }

    // ── Распаковка с раскладкой по метаданным ────────────────────
    v8::StructuredUnpacker unpacker(opts.inputFile, opts.outputDir, mapLoaded ? &map : nullptr);
    if (!unpacker.run()) {
        err << "Ошибка распаковки.\n";
        return 3;
    }


    out << "Готово. Перемещено записей: " << unpacker.renamedCount() << "\n";
        out.flush();
    return 0;

}
