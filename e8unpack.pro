QT = core

CONFIG += c++17 cmdline

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Добавляем путь к заголовочным файлам zlib
INCLUDEPATH += $$PWD/third_party/zlib-1.3.2

# Добавляем исходные файлы zlib в сборку
SOURCES += \
    src/core/MetadataTypes.cpp \
    src/core/V8File.cpp \
    src/core/placeholder216.cpp \
    src/core/utils.cpp \
    src/core/versionfile.cpp \
    src/metadata/ConfigStructureReader.cpp \
    src/metadata/MetadataLayoutBuilder.cpp \
    src/metadata/SectionTypes.cpp \
    src/output/StructuredPacker.cpp \
    src/output/StructuredUnpacker.cpp \
    src/parser/Parse_tree.cpp \
    third_party/zlib-1.3.2/adler32.c \
    third_party/zlib-1.3.2/compress.c \
    third_party/zlib-1.3.2/crc32.c \
    third_party/zlib-1.3.2/deflate.c \
    third_party/zlib-1.3.2/gzclose.c \
    third_party/zlib-1.3.2/gzlib.c \
    third_party/zlib-1.3.2/gzread.c \
    third_party/zlib-1.3.2/gzwrite.c \
    third_party/zlib-1.3.2/infback.c \
    third_party/zlib-1.3.2/inffast.c \
    third_party/zlib-1.3.2/inflate.c \
    third_party/zlib-1.3.2/inftrees.c \
    third_party/zlib-1.3.2/trees.c \
    third_party/zlib-1.3.2/uncompr.c \
    third_party/zlib-1.3.2/zutil.c

# (Опционально) Добавляем заголовочные файлы для отображения в IDE
HEADERS += \
    src/core/MetadataTypes.h \
    src/core/V8File.h \
    src/core/versionfile.h \
    src/metadata/ConfigStructureReader.h \
    src/metadata/MetadataLayoutBuilder.h \
    src/metadata/SectionTypes.h \
    src/output/StructuredPacker.h \
    src/output/StructuredUnpacker.h \
    src/parser/NodeTypes.h \
    src/parser/Parse_tree.h \
    third_party/zlib-1.3.2/zlib.h \
    third_party/zlib-1.3.2/zconf.h \
    third_party/zlib-1.3.2/zutil.h \
    third_party/zlib-1.3.2/crc32.h \
    third_party/zlib-1.3.2/deflate.h \
    third_party/zlib-1.3.2/gzguts.h \
    third_party/zlib-1.3.2/inffast.h \
    third_party/zlib-1.3.2/inffixed.h \
    third_party/zlib-1.3.2/inflate.h \
    third_party/zlib-1.3.2/inftrees.h \
    third_party/zlib-1.3.2/trees.h

SOURCES += \
        main.cpp \
        src/cli/CommandLineParser.cpp \
        src/core/V8Container.cpp \
        src/core/V8FileHeader.cpp \
        src/core/V8Unpacker.cpp \
        src/metadata/MetadataMap.cpp \
        src/metadata/MetadataMapper.cpp \
        src/output/FileLayoutBuilder.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    src/cli/CommandLineParser.h \
    src/core/V8Container.h \
    src/core/V8FileHeader.h \
    src/core/V8Unpacker.h \
    src/metadata/MetadataMap.h \
    src/metadata/MetadataMapper.h \
    src/output/FileLayoutBuilder.h


