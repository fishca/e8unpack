#ifndef NODETYPES_H
#define NODETYPES_H

// Типы узлов дерева (из оригинального NodeTypes.h)
enum node_type {
    nd_empty = 0,       // пустое значение
    nd_list,            // список (контейнер)
    nd_string,          // строка в кавычках
    nd_number,          // целое число
    nd_number_exp,      // число с плавающей точкой / экспонентой
    nd_guid,            // GUID
    nd_binary,          // #base64:...
    nd_binary2,         // просто base64 без префикса
    nd_link,            // ссылка вида "123:ABCDEF..."
    nd_binary_d,        // #data:...
    nd_unknown          // неизвестный тип
};

#endif // NODETYPES_H