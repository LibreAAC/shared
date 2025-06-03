#include "utils.hpp"
#include <filesystem>

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

#include <objbase.h>
#include <mmreg.h>
#include <mmsystem.h>

// Some required types defined for MSVC/TinyC compiler
#if defined(_MSC_VER) || defined(__TINYC__)
    #include "propidl.h"
#endif
#endif
 
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
char _PATH_BUFF[MAX_PATH] = { 0 };
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
      return nfdResult;
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
      wchar_t *filePath(NULL);
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
      return _PATH_BUF;
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
