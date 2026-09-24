#include "CommandLineParser.h"
#include <QTextStream>
#include <QDebug>          // ← добавить
#include <cstdlib>   // std::exit

namespace v8 {

CommandLineOptions CommandLineParser::parse(const QStringList &args) {
    CommandLineOptions opts;

    for (int i = 1; i < args.size(); ++i) {
        const QString &arg = args[i];

        if (arg == "-U" || arg == "--unpack") {
            if (i + 1 < args.size()) opts.inputFile = args[++i];
            if (i + 1 < args.size()) opts.outputDir = args[++i];
        } else if (arg == "-M" || arg == "--metadata-map") {
            if (i + 1 < args.size()) {
                opts.metadataMapFile = args[++i];
                opts.useMetadata = true;
            }
        } else if (arg == "-L" || arg == "--list") {
            opts.listOnly = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage();
            std::exit(0);
        } else if (arg == "-B" || arg == "--build") {
            if (i + 1 >= args.size()) {
                qWarning() << "Ошибка: -B требует аргумент <dir> <output.cf>";
                std::exit(2);
            }
            opts.outputDir = args[++i];  // у нас уже занят под dir
            // inputFile будет содержать .cf (создаваемый)
            if (i + 1 < args.size()) opts.inputFile = args[++i];
            opts.buildMode = true;
        } else if (arg == "--no-deflate") {
            opts.noDeflate = true;
        }
    }

    return opts;
}

void CommandLineParser::printUsage() {
    QTextStream out(stdout);
    out << "v8unpack-metadata — распаковка контейнеров 1С с раскладкой по метаданным\n"
        << "Использование:\n"
        << "  e8unpack -U <input.cf> <output_dir> [-M metadata_map.json]\n"
        << "  e8unpack -L <input.cf>                  (только список)\n"
        << "Параметры:\n"
        << "  -U, --unpack <file> <dir>   Распаковать контейнер в каталог\n"
        << "  -M, --metadata-map <file>   JSON-карта метаданных (из v8_reader)\n"
        << "  -L, --list                  Вывести список элементов без записи\n"
        << "  -h, --help                  Показать эту справку\n"
        << "  -B, --build <dir> <output.cf>  Собрать контейнер из каталога\n"
        << "  --no-deflate                   Не сжимать данные при сборке\n";


}

} // namespace v8