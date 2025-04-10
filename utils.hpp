#pragma once
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <raylib.h>
#include <cstdio>
#ifndef H_UTILS
#define H_UTILS

#define todo() {dblog(LOG_FATAL, "TODO REACHED %s:%i", __FILE__, __LINE__); abort();}
#define dblog(WARN, ...) TraceLog(WARN, __VA_ARGS__)
#define assertm(cond, ...) if (!(cond)) { dblog(LOG_FATAL, __VA_ARGS__); assert((cond));}
#define poff(ptr, byte_offset) ((decltype(ptr))(((uint8_t*)(ptr)) + (byte_offset)))

using isize = int64_t;
using index_t = int32_t;
using u64 = uint64_t;
using i64 = int64_t;
using i32 = int32_t;
using u32 = uint32_t;
using u8 = uint8_t;
using i8 = char;

inline void memcpy_lin(
  void* const dst, int dst_a,
  const void* const src, int src_a,
  int count
) {
  u8 *dst_ = (u8*)dst,
     *src_ = (u8*)src;
  for (int i = 0; i < count; i++)
  {
    int b;
    for (b = 0; b < std::min(dst_a, src_a); b++)
    {
      *dst_ = *src_;
    }
    for (; b < std::max(dst_a, src_a); b++)
    {
      *dst_ = 0;
    }
    dst_ += dst_a;
    src_ += src_a;
  }
}

// A half dynamic heap buffer
struct ByteBuffer
{
  u8* data;
  u64 len : 63;
  bool owned : 1;
  static inline ByteBuffer init() { return {nullptr, 0, 0}; }
  static inline ByteBuffer hold(u8* src, u32 size) { return {src, size, 1}; }
  static inline ByteBuffer ref(u8* src, u32 size) { return {src, size, 0}; }
  void alloc(int size)
  {
    if (data == nullptr)
      data = (u8*)malloc(size);
    else
      data = (u8*)realloc(data, size);
    len = size;
  }
  void dyn_begin(u64& out_cap)
  {
    out_cap = len;
  }
  void dyn_push(u64& io_cap, const ByteBuffer src)
  {
    len += src.len;
    if (len > io_cap)
    {
      do
        io_cap = io_cap + io_cap/2 + 1;
      while (len > io_cap);
      if (data == nullptr)
        data = (u8*)malloc(io_cap);
      else
        data = (u8*)realloc(data, io_cap);
      owned = true;
    }
    memcpy(data+len-src.len, src.data, src.len);
  }
  void dyn_end(u64& io_cap)
  {  }
  void serialize(FILE* f)
  { fwrite(data, len, 1, f); }
  void deserialize(FILE* f)
  { assert(data != nullptr); fread(data, len, 1, f); }
  void destroy()
  { if (data) { free(data); data = nullptr; } }
};

/*
'Fixed' is used here as an opposite to dynamic. Allocated once when deserialized.
Works with variable length utf8 encoding.
*/
struct FixedString
{
  using Char = char;
  u64 _byte_len;
  Char* _data;
  inline i64 len() const
  {
    return _byte_len;
  }
  inline i64 mem_size() const
  {
    return sizeof(Char)*_byte_len;
  }
  inline void deserialize(FILE* f)
  {
    fread(&_byte_len, sizeof(u64), 1, f);
    assert(_byte_len >= 0);
    if (_byte_len == 0)
    {
      _data = nullptr;
      return;
    }
    _data = (Char*)malloc(sizeof(Char)*(_byte_len+1));
    fread(_data, _byte_len, 1, f);
    _data[_byte_len] = 0;
  }
  inline void to_ascii_buffer(char* buff, u64 max_len, char err)
  {
    int i;
    for (i = 0; i < std::min(max_len-1, _byte_len); i++)
    {
      if (isprint(_data[i]))
      {
        buff[i] = _data[i];
      }
      else
      {
        buff[i] = err;
      }
    }
    buff[i] = 0;
  }
  inline Char& operator [] (int i)
  {
    return _data[i];
  }
  void destroy()
  {
    if (_data != nullptr)
    {
      free(_data);
      _data = nullptr;
      _byte_len = 0;
    }
  }
};


long FileEditTime(const char* filename);
// Returns the exact return value of the given command
int shell(const char* command);

const char* open_file_dialogue();
const char* fix2var_utf8(const FixedString& s, char* ob);

[[gnu::const]] bool str_eq(const char* s1, const char* s2);
[[gnu::const]] bool str_startswith(const char* s, const char* start);
[[gnu::const]] bool str_endswith(const char* s, const char* end);
[[gnu::const]] inline constexpr int str_len(const char* s)
{
  if (s == nullptr) return 0;
  if (*s == 0) return 0;
  int c = 1;
  while (*(++s) != 0) c++;
  return c;
}

// TODO: Remove if compiles perfectly without
// inline constexpr int len(const char* s)
// {
//   return str_len(s);
// }

template <class T> using Own = T;
template <class T> using Ref = T;

struct Stream
{
  FILE* _f;
  inline void write_anchor(const char* const anchor4bytes)
  {
    fwrite(anchor4bytes, 4, 1, _f);
  }
  inline void check_anchor(const char* const anchor4bytes)
  {
    char buf[4];
    fread(buf, 4, 1, _f);
    if (!(memcmp(buf, anchor4bytes, 4) == 0))
    {
      dblog(LOG_ERROR, "Failed to find the following anchor:");
      for (int i = 0; i < 4; i++)
        putchar(anchor4bytes[i]);
      puts("");
      abort();
    }
  }
  inline void align_until_anchor(const char* const anchor4bytes)
  {
    char buf[4];
    fread(buf, 4, 1, _f);
    while (memcmp(buf, anchor4bytes, 4) != 0)
    {
      buf[0] = buf[1];
      buf[1] = buf[2];
      buf[2] = buf[3];
      buf[3] = fgetc(_f);
      if (buf[3] == EOF)
      {
        dblog(LOG_ERROR, "Failed to find the following anchor:");
        for (int i = 0; i < 4; i++)
          putchar(anchor4bytes[i]);
        puts("");
        abort();
      }
    }
  }
};
template <class T>
Stream& operator << (Stream& s, T& data)
{ fwrite(&data, sizeof(T), 1, s._f);  return s; }
template <class T>
Stream& operator >> (Stream& s, T& data)
{ fread(&data, sizeof(T), 1, s._f);  return s; }

using fvec2 = Vector2;

extern bool edit_mode;
extern float XMAX, YMAX, dt;
extern bool edit_mode;

namespace ctrl
{
  extern fvec2 mpos;
  extern bool touch_press;
  extern char read;
}

#endif
