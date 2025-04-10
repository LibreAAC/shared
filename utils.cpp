#include "../include/utils.hpp"
#include <filesystem>
#include "nfd.h"

float XMAX, YMAX;

long FileEditTime(const char* filename)
{
  // i swear, the globglobgabgalab is one of the prettiest princess in
  //  comparison to c++
  const auto ft = 
    std::filesystem::last_write_time(filename);
  return ft.time_since_epoch().count();
  // here we have no idea what unit is used bcz ofc it's not even specified in
  //  the doc...
}

namespace ctrl
{
  fvec2 mpos;
  bool touch_press;
}

[[gnu::const]] bool str_eq(const char* s1, const char* s2)
{
  if (s1 == nullptr || s1 == nullptr)
    return s1 == s2;
  while (*s1 != 0 && *s2 != 0)
  {
    if (*s1 != *s2)
      return false;
    s1++; s2++;
  }
  return *s1 == *s2;
}
[[gnu::const]] bool str_startswith(const char* s, const char* start)
{
  if (s == nullptr)
    return start == nullptr;
  while (*start != 0 && *s != 0)
  {
    if (*start != *s)
      return false;
    start++; s++;
  }
  return *s == 0;
}
[[gnu::const]] bool str_endswith(const char* s, const char* end)
{
  const int sl = str_len(s);
  const int el = str_len(end);
  if (sl < el)
    return false;
  for (int i = 0; i < el; i++)
  {
    if (s[sl - el + i] != end[i])
      return false;
  }
  return true;
}

#if 0 && PLATFORM == PLATFORM_ANDROID
#error THIS PART IS TODO
void BringKeyboard()
{
  todo();
}
#endif


#ifdef __linux__
# ifndef MAX_PATH
#  define MAX_PATH 1024
# endif

char _PATH_BUFF[MAX_PATH] = { 0 };
const char* open_file_dialogue()
{
  nfdchar_t* opath;
  nfdresult_t r = NFD_OpenDialog(nullptr, nullptr, &opath);
  if (r == NFD_OKAY)
  {
    const int len = str_len(opath);
    memcpy(_PATH_BUFF, opath, (len+1)*sizeof(char));
    return _PATH_BUFF;
  }
  else if (r == NFD_CANCEL)
  {
    return 0;
  }
  else
  {
    TraceLog(LOG_ERROR, "[NFD] Error opening file: %s", NFD_GetError());
    return 0;
  }
}
#endif
