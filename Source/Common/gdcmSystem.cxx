/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2008 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "gdcmSystem.h"
#include "gdcmTrace.h"
#include "gdcmFilename.h"
#include "gdcmException.h"

#include <iostream>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h> // PATH_MAX

// gettimeofday
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>
#ifdef HAVE_WINSOCK_H
#include <winsock.h>
#endif
#include <stdio.h> // snprintf
#if defined(HAVE_SNPRINTF)
// ok nothing to do
#elif defined(HAVE__SNPRINTF)
#define snprintf _snprintf
#endif
#ifdef __APPLE__
#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFBundle.h>
#include <CoreFoundation/CFURL.h>
#endif // __APPLE__

#if defined(_WIN32) && (defined(_MSC_VER) || defined(__WATCOMC__) ||defined(__BORLANDC__) || defined(__MINGW32__))
#include <io.h>
#include <direct.h>
#define _unlink unlink
#else
//#include <features.h>	// we want GNU extensions
#include <dlfcn.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h> // strncasecmp
#endif 

// TODO: WIN32 replacement for C99 stuff:
// #if defined(_WIN32) || defined(_WIN64)
// #define snprintf _snprintf
// #define vsnprintf _vsnprintf
// #define strcasecmp _stricmp
// #define strncasecmp _strnicmp
// #endif

namespace gdcm
{

#if defined(_WIN32) && (defined(_MSC_VER) || defined(__WATCOMC__) || defined(__BORLANDC__) || defined(__MINGW32__)) 
inline int Mkdir(const char* dir)
{
  return _mkdir(dir);
}
inline int Rmdir(const char* dir)
{
  return _rmdir(dir);
}
inline const char* Getcwd(char* buf, unsigned int len)
{
  const char* ret = _getcwd(buf, len);
  return ret;
}

#else
inline int Mkdir(const char* dir)
{
  return mkdir(dir, 00777);
}
inline int Rmdir(const char* dir)
{
  return rmdir(dir);
}
inline const char* Getcwd(char* buf, unsigned int len)
{
  const char* ret = getcwd(buf, len);
  return ret;
}
#endif

/*
// 1.14 How can I find a process' executable file?
// http://www.faqs.org/faqs/unix-faq/programmer/faq/
static std::string Argv0;

void System::SetArgv0(const char *argv0)
{
  Argv0 = argv0;
//std::cout << "Set:" << Argv0 << std::endl;
}

const char* System::GetArgv0()
{
//std::cout << "Get:" << Argv0 << std::endl;
  return Argv0.c_str();
}
*/

const char * System::GetCWD()
{
  static char buf[2048];
  const char* cwd = Getcwd(buf, 2048);
  return cwd;
/*
  std::string path;
  if ( cwd )
    {
    path = cwd;
    }
  return path;
*/
}

bool System::MakeDirectory(const char *path)
{
  if(System::FileExists(path))
    {
    return true;
    }
  Filename fn(path);
  std::string dir = fn.ToUnixSlashes();

  std::string::size_type pos = dir.find(':');
  if(pos == std::string::npos)
    {
    pos = 0;
    }
  std::string topdir;
  while((pos = dir.find('/', pos)) != std::string::npos)
    {
    topdir = dir.substr(0, pos);
    Mkdir(topdir.c_str());
    pos++;
    }
  if(dir[dir.size()-1] == '/')
    {
    topdir = dir.substr(0, dir.size());
    }
  else
    {
    topdir = dir;
    }
  if(Mkdir(topdir.c_str()) != 0)
    {
    // There is a bug in the Borland Run time library which makes MKDIR
    // return EACCES when it should return EEXISTS
    // if it is some other error besides directory exists
    // then return false
    if( (errno != EEXIST)
#ifdef __BORLANDC__
        && (errno != EACCES)
#endif
      )
      {
      return false;
      }
    }
  return true;
}

// return true if the file exists
bool System::FileExists(const char* filename)
{
#ifdef _MSC_VER
# define access _access
#endif
#ifndef R_OK
# define R_OK 04
#endif
  if ( access(filename, R_OK) != 0 )
    {
    return false;
    }
  else
    {
    //assert( !FileIsDirectory(filename) );
    return true;
    }
}

bool System::FileIsDirectory(const char* name)
{
  struct stat fs;
  if(stat(name, &fs) == 0)
    {
#if _WIN32
    return ((fs.st_mode & _S_IFDIR) != 0);
#else
    return S_ISDIR(fs.st_mode);
#endif
    }
  else
    {
    return false;
    }
}

const char *System::GetLastSystemError()
{
  int e = errno;
  return strerror(e);
}

bool System::GetPermissions(const char* file, unsigned short& mode)
{
  if ( !file )
    {
    return false;
    }

  struct stat st;
  if ( stat(file, &st) < 0 )
    {
    return false;
    }
  mode = st.st_mode;
  return true;
}

bool System::SetPermissions(const char* file, unsigned short mode)
{
  if ( !file )
    {
    return false;
    }
  if ( !System::FileExists(file) )
    {
    return false;
    }
  if ( chmod(file, mode) < 0 )
    {
    return false;
    }

  return true;
}

bool System::RemoveFile(const char* source)
{
#ifdef _WIN32
  unsigned short mode;
  if ( !System::GetPermissions(source, mode) )
    {
    return false;
    }
  /* Win32 unlink is stupid --- it fails if the file is read-only  */
  System::SetPermissions(source, S_IWRITE);
#endif
  bool res = unlink(source) != 0 ? false : true;
#ifdef _WIN32
  if ( !res )
    {
    System::SetPermissions(source, mode);
    }
#endif
  return res;
}

// return size of file; also returns zero if no file exists
size_t System::FileSize(const char* filename)
{
  struct stat fs;
  if (stat(filename, &fs) != 0) 
    {
    return 0;
    }
  else
    {
    return fs.st_size;
    }
}

#if 0
const char *System::GetCurrentDataDirectory()
{
#ifdef _WIN32
  static char path[MAX_PATH];
  gdcm::Filename fn( GetCurrentProcessFileName() );
  if ( !fn.IsEmpty() )
    {
    std::string str = fn.GetPath();
    str += "/../" GDCM_INSTALL_DATA_DIR;
    strcpy(path, str.c_str());
    return path;
    }
#else

  static char path[PATH_MAX];

#ifdef __APPLE__
  Boolean success = false;
  CFURLRef pathURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
  if (pathURL != NULL)
    {
    success = CFURLGetFileSystemRepresentation(pathURL, true /*resolveAgainstBase*/, (unsigned char*) path, PATH_MAX);
    CFRelease(pathURL);
    }
  if (success)
    {
    strncat(path, "/" GDCM_INSTALL_DATA_DIR, PATH_MAX);
    return path;
    }
#endif
    
  gdcm::Filename fn( GetCurrentProcessFileName() );
  if ( !fn.IsEmpty() )
    {
    std::string str = fn.GetPath();
    str += "/../" GDCM_INSTALL_DATA_DIR;
    strcpy(path, str.c_str());
    return path;
    }
#endif
  return 0;
}
#endif

/* 
 * TODO:
 * check cygwin
 * check beos : get_next_image_info
 * check solaris
 * check hpux
 * check os2: DosGetInfoBlocks / DosQueryModuleName
 * check macosx : 
 *  ProcessSerialNumber psn = {kNoProcess, kCurrentProcess};
 *  GetProcessInformation -> FSMakeFSSpec
 * ...
 */
const char *System::GetCurrentProcessFileName()
{
#ifdef _WIN32
  static char buf[MAX_PATH];
  if ( ::GetModuleFileName(0, buf, sizeof(buf)) )
    {
    return buf;
    }
#elif defined(__APPLE__)
  static char buf[PATH_MAX];
  Boolean success = false;
  CFURLRef pathURL = CFBundleCopyExecutableURL(CFBundleGetMainBundle());
  if ( pathURL)
    {
    success = CFURLGetFileSystemRepresentation(pathURL, true /*resolveAgainstBase*/, (unsigned char*) buf, PATH_MAX);
    CFRelease(pathURL);
    }
  if (success)
    {
    return buf;
    }
#else
  static char path[PATH_MAX];
  if ( readlink ("/proc/self/exe", path, sizeof(path)) > 0) // Technically 0 is not an error, but that would mean
                                                            // 0 byte were copied ... thus considered it as an error
    {
    return path;
    }
#endif
   return 0;
}

static void where_am_i() {}

const char *System::GetCurrentModuleFileName()
{
#ifdef __USE_GNU
  static char path[PATH_MAX];
  Dl_info info;
  if (dladdr( (void*)&where_am_i, &info ) == 0)
    {
    strcpy(path,info.dli_fname);
    return path; 
    }
#elif defined(_WIN32)
  // GetModuleFileName works the same on Win32 for library AFAIK
  return System::GetCurrentProcessFileName();
#endif

  return 0;
}

const char *System::GetCurrentResourcesDirectory()
{
#ifdef __APPLE__
  static char path[PATH_MAX];
  Boolean success = false;
  CFURLRef pathURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
  if (pathURL != NULL)
    {
    success = CFURLGetFileSystemRepresentation(pathURL, true /*resolveAgainstBase*/, (unsigned char*) path, PATH_MAX);
    CFRelease(pathURL);
    }
  if (success)
    {
    strncat(path, "/" GDCM_INSTALL_DATA_DIR, PATH_MAX);
    return path;
    }
#endif
  // Is there such beast on *any* other system but APPLE ?
  return 0;
}

/**
 * \brief Encode the mac address on a fixed length string of 15 characters.
 * we save space this way.
 */
inline int getlastdigit(unsigned char *data, unsigned long size)
{
  int extended, carry = 0;
  for(unsigned int i=0;i<size;i++)
    {
    extended = (carry << 8) + data[i];
    data[i] = extended / 10;
    carry = extended % 10;
    }
  return carry;
}

size_t System::EncodeBytes(char *out, const unsigned char *data, int size)
{
  bool zero = false;
  int res;
  std::string sres;
  unsigned char buffer[32];
  unsigned char *addr = buffer;
  memcpy(addr, data, size);
  while(!zero)
    {
    res = getlastdigit(addr, size);
    sres.insert(sres.begin(), '0' + res);
    zero = true;
    for(int i = 0; i < size; ++i)
      {
      zero = zero && (addr[i] == 0);
      }
    }

  //return sres;
  strcpy(out, sres.c_str()); //, sres.size() );
  return sres.size();
}

bool System::GetHardwareAddress(unsigned char addr[6])
{
  int stat = 0; //uuid_get_node_id(addr);
  memset(addr,0,6);
  /*
  // For debugging you need to consider the worse case where hardware addres is max number:
  addr[0] = 255;
  addr[1] = 255;
  addr[2] = 255;
  addr[3] = 255;
  addr[4] = 255;
  addr[5] = 255;
  */
  if (stat == 1) // success
    {
    return true;
    }
  // else
  //gdcmWarningMacro("Problem in finding the MAC Address");
  return false;
}

#if defined(_WIN32) && !defined(HAVE_GETTIMEOFDAY)
#include <stdio.h>
static int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  const uint64_t c1 = 27111902;
  const uint64_t c2 = 3577643008UL;
  const uint64_t OFFSET = (c1 << 32) + c2;
  uint64_t filetime;
  GetSystemTimeAsFileTime(&ft);

  filetime = ft.dwHighDateTime;
  filetime = filetime << 32;
  filetime += ft.dwLowDateTime;
  filetime -= OFFSET;

  memset(tv,0, sizeof(*tv));
  assert( sizeof(*tv) == sizeof(struct timeval));
  tv->tv_sec = (time_t)(filetime / 10000000); /* seconds since epoch */
  tv->tv_usec = (uint32_t)((filetime % 10000000) / 10);

  return 0;
}
#endif

bool System::FormatDateTime(char date[18], time_t timep, long milliseconds)
{
  if(!date) return false;
  const size_t maxsize = 40;
  char tmp[maxsize];
  // Obtain the time of day, and convert it to a tm struct.
  struct tm *ptm = localtime (&timep);
  // Format the date and time, down to a single second.
  size_t ret = strftime (tmp, sizeof (tmp), "%Y%m%d%H%M%S", ptm);
  if( ret == 0 || ret >= maxsize )
    {
    return false;
    }

  // Add milliseconds
  const size_t maxsizall = 18;
  //char tmpAll[maxsizall];
  int ret2 = snprintf(date,maxsizall,"%s%03ld",tmp,milliseconds);
  assert( ret2 >= 0 );
  if( (unsigned int)ret2 >= maxsizall )
    {
    return false;
    }

  // Ok !
  return true;
}

bool System::GetCurrentDateTime(char date[18])
{
  long milliseconds;
  time_t timep;

  struct timeval tv;
  gettimeofday (&tv, NULL);
  timep = tv.tv_sec;
  // Compute milliseconds from microseconds.
  milliseconds = tv.tv_usec / 1000;

  return FormatDateTime(date, timep, milliseconds);
}

int System::StrNCaseCmp(const char *s1, const char *s2, size_t n)
{
#if defined(HAVE_STRNCASECMP)
  return strncasecmp(s1,s2,n);
#elif defined(HAVE__STRNICMP)
  return _strnicmp(s1,s2,n);
#else // default implementation
#error
  assert( n ); // TODO
  while (--n && *s1 && (tolower(*s1) == tolower(*s2)))
    {
    s1++;
    s2++;
    }

 return tolower(*s1) - tolower(*s2);
#endif
}

int System::StrCaseCmp(const char *s1, const char *s2)
{
#if defined(HAVE_STRCASECMP)
  return strcasecmp(s1,s2);
#elif defined(HAVE__STRNICMP)
  return _stricmp(s1,s2);
#else // default implementation
#error
  while (*s1 && (tolower(*s1) == tolower(*s2)))
    {
    s1++;
    s2++;
    }

 return tolower(*s1) - tolower(*s2);
#endif
}

} // end namespace gdcm
