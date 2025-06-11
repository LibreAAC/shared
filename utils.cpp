#include "utils.hpp"
#include <filesystem>
 
#ifdef FILE_DIALOG
# ifndef _WIN32
#  include "nfd.h"
# endif
#endif


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

[[gnu::const]] bool str_eq(const char* s1, const char* s2)
{
  if (s1 == nullptr || s2 == nullptr)
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
  return *start == 0;
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

#ifdef FILE_DIALOG
#include "raylib.h"
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
#else // _WIN32
# ifndef MAX_PATH
#  define MAX_PATH 2048
# endif

#ifdef FILE_DIALOG
#include "raylib.h"
#include "ifiledialogfix.h"
char _PATH_BUFF[MAX_PATH] = { 0 };
#define COM_INITFLAGS COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE
struct IFileOpenDialog;
// const GUID CLSID_FileOpenDialog = {0xDC1C5A9C,0xE88A,0x4DDE,0xA5,0xA1,0x60,0xF8,0x2A,0x20,0xAE,0xF7};
// const GUID IID_IFileDialog = {0x42F85136,0xDB7E,0x439C,0x85,0xF1,0xE4,0x07,0x5D,0x13,0x5F,0xC8};
// ALL OF THIS FUNCTION IS COPIED AND MODIFIED FROM nativefiledialog
// (this is because nfd could not compile properly on windows with cmake for
// some reason)
const char* open_file_dialogue()
{
  constexpr int OK = 0;
  constexpr int ERROR = 1;
  int nfdResult = ERROR;

  
  HRESULT coResult = CoInitializeEx(NULL, COM_INITFLAGS);
  if (coResult != RPC_E_CHANGED_MODE)
  {        
      TraceLog(LOG_ERROR, "[open_file_dialog()] Could not initialize COM.");
      return nullptr;
  }

  // Create dialog
  IFileOpenDialog *fileOpenDialog = nullptr;    
  HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, NULL,
                                      CLSCTX_ALL, IID_IFileOpenDialog,
                                      reinterpret_cast<void**>(&fileOpenDialog) );
                              
  if ( !SUCCEEDED(result) )
  {
      TraceLog(LOG_ERROR, "[open_file_dialog()] Could not create dialog.");
      goto end;
  }

  // Show the dialog.
  result = fileOpenDialog->Show(NULL);
  if ( SUCCEEDED(result) )
  {
      // Get the file name
      IShellItem *shellItem(NULL);
      result = fileOpenDialog->GetResult(&shellItem);
      if ( !SUCCEEDED(result) )
      {
        TraceLog(LOG_ERROR, "[open_file_dialog()] Could not get shell item from dialog.");
        goto end;
      }
      wchar_t *filePath = nullptr;
      result = shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
      if ( !SUCCEEDED(result) )
      {
          TraceLog(LOG_ERROR, "[open_file_dialog()] Could not get file path for selected.");
          shellItem->Release();
          goto end;
      }

      wcsncpy(outPath, filePath, MAX_PATH);
      CoTaskMemFree(filePath);
      if ( !*outPath )
      {
          /* error is malloc-based, error message would be redundant */
          shellItem->Release();
          goto end;
      }

      nfdResult = OK;
      shellItem->Release();
      return _PATH_BUFF;
  }
  else if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED) )
  {
      return nullptr;
  }
  else
  {
      TraceLog(LOG_ERROR, "[open_file_dialog()] File dialog box show failed.");
      return nullptr;
  }

end:
  
  return nullptr;
}
#endif
#endif
