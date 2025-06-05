#pragma once
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#ifndef H_UTILS
#define H_UTILS

#define todo() {dblog(LOG_FATAL, "TODO REACHED %s:%i", __FILE__, __LINE__); abort();}
#define dblog(WARN, ...) fprintf(stderr, __VA_ARGS__)
#define assertm(cond, ...) if (!(cond)) { dblog(LOG_FATAL, __VA_ARGS__); assert((cond));}
#define poff(ptr, byte_offset) ((decltype(ptr))(((uint8_t*)(ptr)) + (byte_offset)))

#if defined(_WIN32)
// To avoid conflicting windows.h symbols with raylib, some flags are defined
// WARNING: Those flags avoid inclusion of some Win32 headers that could be required
// by user at some point and won't be included...
//-------------------------------------------------------------------------------------

// If defined, the following flags inhibit definition of the indicated items.
#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define NOVIRTUALKEYCODES // VK_*
#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS           // MF_*
#define NOICONS           // IDI_*
#define NOKEYSTATES       // MK_*
#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
#define NOSHOWWINDOW      // SW_*
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // Screen colors
#define NOCTLMGR          // Control and Dialog routines
#define NODRAWTEXT        // DrawText() and DT_*
#define NOGDI             // All GDI defines and routines
#define NOKERNEL          // All KERNEL defines and routines
#define NOUSER            // All USER defines and routines
//#define NONLS             // All NLS defines and routines
#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions

// Type required before windows.h inclusion
typedef struct tagMSG *LPMSG;

#include <windows.h>

// Type required by some unused function...
typedef struct tagBITMAPINFOHEADER {
  DWORD biSize;
  LONG  biWidth;
  LONG  biHeight;
  WORD  biPlanes;
  WORD  biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG  biXPelsPerMeter;
  LONG  biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

#define NOPLAYSOUND
#include <objbase.h>
#include <mmreg.h>
#include <mmsystem.h>

// Some required types defined for MSVC/TinyC compiler
#if defined(_MSC_VER) || defined(__TINYC__)
    #include "propidl.h"
#endif
#ifdef PlaySound
# undef PlaySound
#endif
#endif

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
  void serialize(FILE* f)
  { fwrite(data, len, 1, f); }
  void deserialize(FILE* f)
  { assert(data != nullptr); fread(data, len, 1, f); }
  void destroy()
  { if (data) { free(data); data = nullptr; } }
};
struct DynByteBuffer {
  ByteBuffer& buffer;
  u64 cap;
  static inline DynByteBuffer from(ByteBuffer& buf)
  { return {buf, buf.len}; }
  template <class T>
  T* data() { return (T*)buffer.data; }
  u64 len() { return buffer.len; }
  void set_len(u64 size) { buffer.len = size; }
  void clear() { buffer.len = 0; }
  void prealloc_at_least(int size)
  {
    if (cap < size)
    {
      if (buffer.data == nullptr)
        buffer.data = (u8*)calloc(size,1);
      else
        buffer.data = (u8*)realloc(buffer.data, size);
      cap = size;
      buffer.owned = true;
    }
  }
  void push(const ByteBuffer src)
  { buffer.dyn_push(cap, src); }
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
{ fwrite(&data, sizeof(T), 1, s._f); return s; }
template <>
inline Stream& operator << <ByteBuffer>(Stream& s, ByteBuffer& data)
{ fwrite(data.data, data.len, 1, s._f); return s; }
template <class T>
Stream& operator >> (Stream& s, T& data)
{ fread(&data, sizeof(T), 1, s._f); return s; }


#endif
