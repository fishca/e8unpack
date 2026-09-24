/*----------------------------------------------------------
This Source Code Form is subject to the terms of the
Mozilla Public License, v.2.0. If a copy of the MPL
was not distributed with this file, You can obtain one
at http://mozilla.org/MPL/2.0/.
----------------------------------------------------------*/
#include "V8File.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>
#include <cstring>
#include <cctype>
#include "zlib.h"

#define CHUNK 16384
#ifndef DEF_MEM_LEVEL
#  if MAX_MEM_LEVEL >= 8
#    define DEF_MEM_LEVEL 8
#  else
#    define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#  endif
#endif

namespace v8unpack {

template<typename T, int length>
T hex_to_int(const char *hextext)
{
    auto s = hextext;
    auto i = length;
    T value = 0;
    for (; i; i--, s++) {

        auto lower_s = std::tolower(static_cast<unsigned char>(*s));
        if (lower_s >= '0' && lower_s <= '9') {
            value <<= 4;
            value += lower_s - '0';
        }
        else if (lower_s >= 'a' && lower_s <= 'f') {
            value <<= 4;
            value += lower_s - 'a' + 10;
        }
        else
            break;
    }
    return value;
}

uint32_t _httoi(const char *value)   { return hex_to_int<uint32_t, 8>(value); }
uint64_t _httoi64(const char *value) { return hex_to_int<uint64_t, 16>(value); }

static const char hex[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

template<typename T, int N>
void int_to_hex(T value, char *buf)
{
    for (int i = 2 * N; i; i--) {
        buf[i - 1] = hex[value & 0xf];
        value >>= 4;
    }
}

void _itoht(uint32_t value, char *ht)   { int_to_hex<uint32_t, 4>(value, ht); }
void _itoht64(uint64_t value, char *ht) { int_to_hex<uint64_t, 8>(value, ht); }

// ---- Файловые обёртки --------------------------------------------------

int Inflate(const std::string &in_filename, const std::string &out_filename)
{
    std::ifstream in_file;
    std::istream *input = nullptr;

    if (in_filename == "-") {
        input = &std::cin;
    } else {
        in_file.open(std::filesystem::path(in_filename), std::ios_base::binary);
        if (!in_file)
            return V8UNPACK_DEFLATE_IN_FILE_NOT_FOUND;
        input = &in_file;
    }

    std::ofstream out_file;
    std::ostream *output = nullptr;

    if (out_filename == "-") {
        output = &std::cout;
    } else {
        out_file.open(std::filesystem::path(out_filename), std::ios_base::binary);
        if (!out_file)
            return V8UNPACK_INFLATE_OUT_FILE_NOT_CREATED;
        output = &out_file;
    }

    try_inflate(*input, *output);
    return V8UNPACK_OK;
}

int Deflate(const std::string &in_filename, const std::string &out_filename)
{
    std::ifstream in_file;
    std::istream *input = nullptr;

    if (in_filename == "-") {
        input = &std::cin;
    } else {
        in_file.open(std::filesystem::path(in_filename), std::ios_base::binary);
        if (!in_file)
            return V8UNPACK_DEFLATE_IN_FILE_NOT_FOUND;
        input = &in_file;
    }

    std::ofstream out_file;
    std::ostream *output = nullptr;

    if (out_filename == "-") {
        output = &std::cout;
    } else {
        out_file.open(std::filesystem::path(out_filename), std::ios_base::binary);
        if (!out_file)
            return V8UNPACK_INFLATE_OUT_FILE_NOT_CREATED;
        output = &out_file;
    }

    int ret = Deflate(*input, *output);
    if (ret)
        return V8UNPACK_DEFLATE_ERROR;
    return 0;
}

// Эта перегрузка — именно та, на которую ругался линкер
int Deflate(std::istream &source, const std::string &out_filename)
{
    std::ofstream dest(std::filesystem::path(out_filename), std::ios_base::binary);
    return Deflate(source, dest);
}

// ---- Потоковые Deflate / Inflate ---------------------------------------

int Deflate(std::istream &source, std::ostream &dest)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;

    ret = deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED,
                       -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK)
        return ret;

    do {
        strm.avail_in = static_cast<uInt>(
            source.read(reinterpret_cast<char*>(in), CHUNK).gcount());
        if (source.bad()) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }

        flush = source.eof() ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        do {
            strm.avail_out = CHUNK;
            strm.next_out  = out;
            ret = deflate(&strm, flush);
            assert(ret != Z_STREAM_ERROR);
            have = CHUNK - strm.avail_out;

            dest.write(reinterpret_cast<char*>(out), have);

            if (dest.bad()) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);

    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);

    (void)deflateEnd(&strm);
    return Z_OK;
}

int Inflate(std::istream &source, std::ostream &dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    strm.zalloc   = Z_NULL;
    strm.zfree    = Z_NULL;
    strm.opaque   = Z_NULL;
    strm.avail_in = 0;
    strm.next_in  = Z_NULL;

    ret = inflateInit2(&strm, -MAX_WBITS);
    if (ret != Z_OK)
        return ret;

    do {
        strm.avail_in = static_cast<uInt>(
            source.read(reinterpret_cast<char*>(in), CHUNK).gcount());
        if (source.bad()) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;

        strm.next_in = in;

        do {
            strm.avail_out = CHUNK;
            strm.next_out  = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR;
                    [[fallthrough]];
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    return ret;
            }
            have = CHUNK - strm.avail_out;
            dest.write(reinterpret_cast<char*>(out), have);
            if (dest.bad()) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

    } while (ret != Z_STREAM_END);

    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

// ---- Буферные Deflate / Inflate ----------------------------------------

int Inflate(const char* in_buf, char** out_buf, uint32_t in_len, uint32_t* out_len)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char out[CHUNK];

    unsigned long out_buf_len = in_len + CHUNK;
    *out_buf = static_cast<char*>(realloc(*out_buf, out_buf_len));
    *out_len = 0;

    strm.zalloc   = Z_NULL;
    strm.zfree    = Z_NULL;
    strm.opaque   = Z_NULL;
    strm.avail_in = 0;
    strm.next_in  = Z_NULL;

    ret = inflateInit2(&strm, -MAX_WBITS);
    if (ret != Z_OK)
        return ret;

    strm.avail_in = in_len;
    strm.next_in  = reinterpret_cast<unsigned char*>(const_cast<char*>(in_buf));

    do {
        strm.avail_out = CHUNK;
        strm.next_out  = out;
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);
        switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;
                [[fallthrough]];
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
        }
        have = CHUNK - strm.avail_out;
        if (*out_len + have > out_buf_len) {
            out_buf_len += sizeof(out);
            *out_buf = static_cast<char*>(realloc(*out_buf, out_buf_len));
            if (!*out_buf) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        }
        std::memcpy(*out_buf + *out_len, out, have);
        *out_len += have;
    } while (strm.avail_out == 0);

    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int Deflate(const char* in_buf, char** out_buf, uint32_t in_len, uint32_t* out_len)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char out[CHUNK];

    unsigned long out_buf_len = in_len + CHUNK;
    *out_buf = static_cast<char*>(realloc(*out_buf, out_buf_len));
    *out_len = 0;

    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;

    ret = deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED,
                       -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK)
        return ret;

    flush = Z_FINISH;
    strm.next_in  = reinterpret_cast<unsigned char*>(const_cast<char*>(in_buf));
    strm.avail_in = in_len;

    do {
        strm.avail_out = sizeof(out);
        strm.next_out  = out;
        ret = deflate(&strm, flush);
        assert(ret != Z_STREAM_ERROR);
        have = sizeof(out) - strm.avail_out;
        if (*out_len + have > out_buf_len) {
            out_buf_len += sizeof(out);
            *out_buf = static_cast<char*>(realloc(*out_buf, out_buf_len));
            if (!*out_buf) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }
        }
        std::memcpy(*out_buf + *out_len, out, have);
        *out_len += have;
    } while (strm.avail_out == 0);
    assert(strm.avail_in == 0);
    assert(ret == Z_STREAM_END);

    (void)deflateEnd(&strm);
    return Z_OK;
}

// ---- try_inflate -------------------------------------------------------

bool try_inflate(std::vector<char> &data)
{
    char     *inflated_data      = nullptr;
    uint32_t  inflated_data_size = 0;

    auto ret = Inflate(data.data(), &inflated_data, static_cast<uint32_t>(data.size()),
                       &inflated_data_size);
    if (ret == Z_OK) {
        data.assign(inflated_data, inflated_data + inflated_data_size);
        free(inflated_data);
        return true;
    }
    if (inflated_data != nullptr)
        free(inflated_data);
    return false;
}

bool try_inflate(std::istream &source, std::ostream &dest)
{
    auto gpos = source.tellg();
    auto ppos = dest.tellp();

    auto ret = Inflate(source, dest);

    if (ret != Z_OK) {
        source.clear();
        source.seekg(gpos, std::ios_base::beg);
        dest.seekp(ppos, std::ios_base::beg);
        full_copy(source, dest);
        return false;
    }
    return true;
}

bool try_inflate(const std::filesystem::path &source,
                 const std::filesystem::path &dest)
{
    std::ifstream inf(source, std::ios_base::binary);
    std::ofstream out(dest, std::ios_base::binary);
    return try_inflate(inf, out);
}

} // namespace v8unpack