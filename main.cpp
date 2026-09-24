#include <QCoreApplication>

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



    /*
    // ── Этап 1: распаковка через v8unpack ──────────────────────────────
    {
        std::vector<std::string> filter;
        int ret = v8unpack::Parse(
            opts.inputFile.toStdString(),
            opts.outputDir.toStdString(),
            filter);

        if (ret != v8unpack::V8UNPACK_OK) {
            err << "Ошибка распаковки, код: " << ret << "\n";
            return 3;
        }
    }

    out << "Распаковка завершена: " << opts.outputDir << "\n";

    */

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
    v8::StructuredUnpacker unpacker(
        opts.inputFile,
        opts.outputDir,
        mapLoaded ? &map : nullptr);

    if (!unpacker.run()) {
        err << "Ошибка распаковки.\n";
        return 3;
    }




    /*
    // ── Этап 2: раскладка по метаданным (если указана карта) ──────────
    if (opts.useMetadata && !opts.metadataMapFile.isEmpty()) {
        v8::MetadataMap map;
        if (!map.loadFromJson(opts.metadataMapFile)) {
            err << "Предупреждение: не удалось загрузить карту метаданных. "
                << "Структура остаётся в виде GUID.\n";
            return 0;
        }

        v8::MetadataLayoutBuilder builder(map, opts.outputDir);
        if (!builder.reorganize()) {
            err << "Ошибка при раскладке по метаданным.\n";
            return 4;
        }

        out << "Раскладка по метаданным: переименовано "
            << builder.renamedCount() << " записей.\n";
    } else {
        out << "Карта метаданных не указана — структура осталась по GUID.\n";
    }
    */


    out << "Готово. Перемещено записей: " << unpacker.renamedCount() << "\n";
        out.flush();
    return 0;

}
