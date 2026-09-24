#ifndef V8FILE_H
#define V8FILE_H

/*----------------------------------------------------------
This Source Code Form is subject to the terms of the
Mozilla Public License, v.2.0. If a copy of the MPL
was not distributed with this file, You can obtain one
at http://mozilla.org/MPL/2.0/.
----------------------------------------------------------*/
/////////////////////////////////////////////////////////////////////////////
//	Author:			disa_da
//	E-mail:			disa_da2@mail.ru
/////////////////////////////////////////////////////////////////////////////

/**
    2014-2021       dmpas       sergey(dot)batanov(at)dmpas(dot)ru
    2019-2020       fishca      fishcaroot(at)gmail(dot)com
 */


#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))


#include <cstdint>
#include <cstring>
#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <memory>
#include <filesystem>
#include <algorithm>

namespace v8unpack {

const size_t   V8_DEFAULT_PAGE_SIZE  = 512;
const uint32_t V8_FF_SIGNATURE       = 0x7fffffff;

// Для конфигурации старше 8.3.16, без режима совместимости
const uint64_t V8_FF64_SIGNATURE     = 0xffffffffffffffff;
const size_t   V8_OFFSET_8316        = 0x1359;

const int V8UNPACK_OK                             = 0;
const int V8UNPACK_ERROR                          = -50;
const int V8UNPACK_NOT_V8_FILE                    = V8UNPACK_ERROR - 1;
const int V8UNPACK_HEADER_ELEM_NOT_CORRECT        = V8UNPACK_ERROR - 2;
const int V8UNPACK_SOURCE_DOES_NOT_EXIST          = V8UNPACK_ERROR - 3;
const int V8UNPACK_ERROR_CREATING_OUTPUT_FILE     = V8UNPACK_ERROR - 4;
const int V8UNPACK_INFLATE_ERROR                  = V8UNPACK_ERROR - 20;
const int V8UNPACK_INFLATE_IN_FILE_NOT_FOUND      = V8UNPACK_INFLATE_ERROR - 1;
const int V8UNPACK_INFLATE_OUT_FILE_NOT_CREATED   = V8UNPACK_INFLATE_ERROR - 2;
const int V8UNPACK_INFLATE_DATAERROR              = V8UNPACK_INFLATE_ERROR - 3;
const int V8UNPACK_DEFLATE_ERROR                  = V8UNPACK_ERROR - 30;
const int V8UNPACK_DEFLATE_IN_FILE_NOT_FOUND      = V8UNPACK_ERROR - 1;
const int V8UNPACK_DEFLATE_OUT_FILE_NOT_CREATED   = V8UNPACK_ERROR - 2;
const int V8UNPACK_SHOW_USAGE                     = -22;

uint32_t _httoi(const char *value);
uint64_t _httoi64(const char *value);
void     _itoht(uint32_t value, char *ht);
void     _itoht64(uint64_t value, char *ht);

// ---- Заголовки форматов ------------------------------------------------

struct stFileHeader
{
    uint32_t next_page_addr = 0;
    uint32_t page_size      = 0;
    uint32_t storage_ver    = 0;
    uint32_t reserved       = 0;

    static size_t Size() { return 4 + 4 + 4 + 4; }
};

struct stFileHeader64
{
    uint64_t next_page_addr = 0;
    uint32_t page_size      = 0;
    uint32_t storage_ver    = 0;
    uint32_t reserved       = 0;

    static size_t Size() { return 8 + 4 + 4 + 4; }
};

struct stElemAddr
{
    uint32_t elem_header_addr;
    uint32_t elem_data_addr;
    uint32_t fffffff;

    static size_t Size() { return 4 + 4 + 4; }
    static const uint32_t UNDEFINED_VALUE = 0x7fffffff;
};

struct stElemAddr64
{
    uint64_t elem_header_addr;
    uint64_t elem_data_addr;
    uint64_t fffffff;

    static size_t Size() { return 8 + 8 + 8; }
    static const uint64_t UNDEFINED_VALUE = 0xffffffffffffffff;
};

struct stBlockHeader
{
    char EOL_0D          = 0x0d;
    char EOL_0A          = 0x0a;
    char data_size_hex[8]  = {'0','0','0','0','0','0','0','0'};
    char space1          = ' ';
    char page_size_hex[8]  = {'0','0','0','0','0','0','0','0'};
    char space2          = ' ';
    char next_page_addr_hex[8] = {'7','f','f','f','f','f','f','f'};
    char space3          = ' ';
    char EOL2_0D         = 0x0d;
    char EOL2_0A         = 0x0a;

    static stBlockHeader create(uint32_t block_data_size, uint32_t page_size, uint32_t next_page_addr);
    static stBlockHeader create(uint32_t block_data_size, uint32_t page_size);

    static size_t Size() { return 1+1+8+1+8+1+8+1+1+1; }

    bool IsCorrect() const
    {
        return EOL_0D == 0x0d && EOL_0A == 0x0a
            && space1 == 0x20 && space2 == 0x20 && space3 == 0x20
            && EOL2_0D == 0x0d && EOL2_0A == 0x0a;
    }

    uint32_t data_size()      const { return _httoi(data_size_hex); }
    uint32_t page_size()      const { return _httoi(page_size_hex); }
    uint32_t next_page_addr() const { return _httoi(next_page_addr_hex); }

    void set_data_size(uint32_t v)      { _itoht(v, data_size_hex); }
    void set_page_size(uint32_t v)      { _itoht(v, page_size_hex); }
    void set_next_page_addr(uint32_t v) { _itoht(v, next_page_addr_hex); }

    static const uint32_t UNDEFINED_VALUE = 0x7fffffff;
};

struct stBlockHeader64
{
    char EOL_0D          = 0x0d;
    char EOL_0A          = 0x0a;
    char data_size_hex[16]  = { ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
    char space1          = ' ';
    char page_size_hex[16]  = { ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
    char space2          = ' ';
    char next_page_addr_hex[16] = { ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ' };
    char space3          = ' ';
    char EOL2_0D         = 0x0d;
    char EOL2_0A         = 0x0a;

    static stBlockHeader64 create(uint64_t block_data_size, uint64_t page_size, uint64_t next_page_addr);
    static stBlockHeader64 create(uint64_t block_data_size, uint64_t page_size);

    static size_t Size() { return 1+1+16+1+16+1+16+1+1+1; }

    bool IsCorrect() const
    {
        return EOL_0D == 0x0d && EOL_0A == 0x0a
            && space1 == 0x20 && space2 == 0x20 && space3 == 0x20
            && EOL2_0D == 0x0d && EOL2_0A == 0x0a;
    }

    uint64_t data_size()      const { return _httoi64(data_size_hex); }
    uint64_t page_size()      const { return _httoi64(page_size_hex); }
    uint64_t next_page_addr() const { return _httoi64(next_page_addr_hex); }

    void set_data_size(uint64_t v)      { _itoht64(v, data_size_hex); }
    void set_page_size(uint64_t v)      { _itoht64(v, page_size_hex); }
    void set_next_page_addr(uint64_t v) { _itoht64(v, next_page_addr_hex); }

    static const uint64_t UNDEFINED_VALUE = 0xffffffffffffffff;
};

struct Format15
{
    typedef stFileHeader  file_header_t;
    typedef stBlockHeader block_header_t;
    typedef stElemAddr    elem_addr_t;

    static const uint32_t UNDEFINED_VALUE  = 0x7fffffff;
    static const std::streamoff BASE_OFFSET = 0;
    static const uint32_t DEFAULT_PAGE_SIZE = 512;

    template <class E, class T>
    static std::basic_ostream<E, T>& placeholder(std::basic_ostream<E, T>& o) { return o; }
};

struct Format16
{
    typedef stFileHeader64  file_header_t;
    typedef stBlockHeader64 block_header_t;
    typedef stElemAddr64    elem_addr_t;

    static const uint64_t UNDEFINED_VALUE   = 0xffffffffffffffff;
    static const std::streamoff BASE_OFFSET = 0x1359;
    static const uint64_t DEFAULT_PAGE_SIZE = 512;

    static std::basic_ostream<char>& placeholder(std::basic_ostream<char>& o);
};

// ---- Классы ------------------------------------------------------------

class CV8Elem;

class CV8File
{
public:
    CV8File();
    CV8File(const CV8File &src);
    virtual ~CV8File() = default;

    void Dispose();

    int GetData(std::vector<char> &data);
    int LoadFileFromFolder(const std::string &dirname);

private:
    stFileHeader                FileHeader;
    std::vector<stElemAddr>     ElemsAddrs;
    std::vector<CV8Elem>        Elems;
    bool                        IsDataPacked = false;
};

class CV8Elem
{
public:
    struct stElemHeaderBegin
    {
        uint64_t date_creation;
        uint64_t date_modification;
        uint32_t res;

        static size_t Size() { return 8 + 8 + 4; }
    };

    CV8Elem() = default;
    CV8Elem(const CV8Elem &src) = default;
    explicit CV8Elem(const std::string &name);
    ~CV8Elem() = default;

    int         Pack(bool deflate = true);
    int         SetName(const std::string &ElemName);
    std::string GetName() const;

    void Dispose();

    std::vector<char>   header;
    std::vector<char>   data;
    CV8File             UnpackedData;
    bool                IsV8File = false;
    bool                NeedUnpack = false;

private:
    void resizeHeader(size_t newSize);
};

// ---- Публичный API -----------------------------------------------------

int  PackFromFolder   (const std::string &dirname, const std::string &filename);
int  BuildCfFile      (const std::string &dirname, const std::string &filename, bool dont_deflate);
int  UnpackToFolder   (const std::string &filename, const std::string &dirname,
                       const std::string &block_name, bool print_progress = false);
int  Parse            (const std::string &filename, const std::string &dirname,
                       const std::vector<std::string> &filter);
int  ListFiles        (const std::string &filename);

bool IsV8File         (std::basic_istream<char> &file);
bool IsV8File16       (std::basic_istream<char> &file);

int  Deflate(std::istream &source, const std::string &out_filename);
int  Deflate(std::istream &source, std::ostream &dest);
int  Inflate(std::istream &source, std::ostream &dest);
int  Deflate(const std::string &in_filename, const std::string &out_filename);
int  Inflate(const std::string &in_filename, const std::string &out_filename);
int  Deflate(const char* in_buf, char** out_buf, uint32_t in_len, uint32_t* out_len);
int  Inflate(const char* in_buf, char** out_buf, uint32_t in_len, uint32_t* out_len);

bool try_inflate(std::vector<char> &data);
bool try_inflate(std::istream &source, std::ostream &dest);
bool try_inflate(const std::filesystem::path &source,
                 const std::filesystem::path &dest);

template <typename T>
void full_copy(std::basic_istream<T> &in_file, std::basic_ostream<T> &out_file)
{
    std::copy(std::istreambuf_iterator<T>(in_file),
              std::istreambuf_iterator<T>(),
              std::ostreambuf_iterator<T>(out_file));
}

} // namespace v8unpack

#endif // V8FILE_H
