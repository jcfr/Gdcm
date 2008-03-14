/*=========================================================================

  Program: GDCM (Grass Root DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2008 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __gdcmByteValue_h
#define __gdcmByteValue_h

#include "gdcmValue.h"
#include "gdcmTrace.h"
#include "gdcmVL.h"

#include <vector>
#include <iterator>
#include <iomanip>

namespace gdcm
{
/**
 * \brief Class to represent binary value (array of bytes)
 * \note
 */
class GDCM_EXPORT ByteValue : public Value
{
public:
  ByteValue(const char* array = 0, VL const &vl = 0):
    Internal(array, array+vl),Length(vl) {
      if( vl.IsOdd() )
        {
        gdcmDebugMacro( "Odd length" );
        Internal.resize(vl+1);
        Length++;
        }
  }
  ByteValue(std::vector<char> &v):Internal(v),Length(v.size()) {}
  ByteValue(std::ostringstream const &os) {
	   abort(); // TODO
  }
  ~ByteValue() {
    Internal.clear();
  }

  // When 'dumping' dicom file we still have some information from
  // Either the VR: eg LO (private tag)
  void PrintASCII(std::ostream &os, VL maxlength ) const {
    VL length = std::min(maxlength, Length);
    // Special case for VR::UI, do not print the trailing \0
    if( length && length == Length )
      {
      if( *Internal.rbegin() == 0 )
        {
        length = length - 1;
        }
      }
    // I cannot check IsPrintable some file contains \2 or \0 in a VR::LO element
    // See: acr_image_with_non_printable_in_0051_1010.acr 
    //assert( IsPrintable(length) );
    std::vector<char>::const_iterator it = Internal.begin();
    for(; it != Internal.begin()+length; ++it)
      {
      const char &c = *it;
      if ( !( isprint((int)c) || isspace((int)c) ) ) os << ".";
      else os << c;
      }
  }

  void PrintHex(std::ostream &os, VL maxlength ) const {
    VL length = std::min(maxlength, Length);
    // WARNING: Internal.end() != Internal.begin()+Length
    std::vector<char>::const_iterator it = Internal.begin();
    for(; it != Internal.begin()+length; ++it)
      {
      const char &c = *it;
      os << "\\" << (int)c;
      }
  }

  // Either from Element Number (== 0x0000)
  void PrintGroupLength(std::ostream &os) {
    assert( Length == 2 );
    (void)os;
  }

  bool IsEmpty() const { assert( Length == 0 ); return Internal.empty(); }
  VL GetLength() const { return Length; }
  // Does a reallocation
  void SetLength(VL vl) {
    VL l(vl);
    assert( !l.IsUndefined() );
    if ( l.IsOdd() ) {
      gdcmDebugMacro(
        "BUGGY HEADER: Your dicom contain odd length value field." );
      ++l;
      }
    // I cannot use reserve for now. I need to implement:
    // STL - vector<> and istream
    // http://groups.google.com/group/comp.lang.c++/msg/37ec052ed8283e74
//#define SHORT_READ_HACK
    try
      {
#ifdef SHORT_READ_HACK
    if( l <= 0xff )
#endif
      Internal.resize(l);
      //Internal.reserve(l);
      }
    catch(...)
      {
      exit(1);
      }
    // Keep the exact length
    Length = vl;
  }

  operator const std::vector<char>& () const { return Internal; }

  ByteValue &operator=(const ByteValue &val) {
    Internal = val.Internal;
    return *this;
    }

  bool operator==(const ByteValue &val) const {
    if( Length != val.Length )
      return false;
    if( Internal == val.Internal )
      return true;
    return false;
    }

  void Clear() {
    Internal.clear();
  }
  // Use that only if you understand what you are doing
  const char *GetPointer() const {
    if(!Internal.empty()) return &Internal[0];
	return 0;
  }
  bool GetBuffer(char *buffer, unsigned long length) const {
    // SIEMENS_GBS_III-16-ACR_NEMA_1.acr has a weird pixel length
    // so we need an inequality
    if( length <= Internal.size() )
      {
      memcpy(buffer, &Internal[0], length);
      return true;
      }
    abort();
    return false;
    }
  bool WriteBuffer(std::ostream &os) const {
    assert( Internal.size() <= Length );
    assert( !(Internal.size() % 2) );
    os.write(&Internal[0], Internal.size() );
    return true;
  }

  template <typename TSwap, typename TType>
  std::istream &Read(std::istream &is) {
    // If Length is odd we have detected that in SetLength
    // and calling std::vector::resize make sure to allocate *AND* 
    // initialize values to 0 so we are sure to have a \0 at the end
    // even in this case
    if(Length)
      {
      is.read(&Internal[0], Length);
      assert( Internal.size() == Length || Internal.size() == Length + 1 );
      TSwap::SwapArray((TType*)&Internal[0], Internal.size() / sizeof(TType) );
      }
    return is;
  }

  template <typename TSwap>
  std::istream &Read(std::istream &is) {
    return Read<TSwap,uint8_t>(is);
  }


  template <typename TSwap, typename TType>
  std::ostream const &Write(std::ostream &os) const {
    assert( !(Internal.size() % 2) );
    if( !Internal.empty() ) {
      //os.write(&Internal[0], Internal.size());
      std::vector<char> copy = Internal;
      TSwap::SwapArray((TType*)&copy[0], Internal.size() / sizeof(TType) );
      os.write(&copy[0], copy.size());
      }
    return os;
  }

  template <typename TSwap>
  std::ostream const &Write(std::ostream &os) const {
    return Write<TSwap,uint8_t>(os);
  }

  /**
   * \brief  Checks whether a 'ByteValue' is printable or not (in order
   *         to avoid corrupting the terminal of invocation when printing)
   *         I dont think this function is working since it does not handle
   *         UNICODE or character set...
   */
  bool IsPrintable(VL length) const {
    assert( length <= Length );
    for(unsigned int i=0; i<length; i++)
      {
      if ( i == (length-1) && Internal[i] == '\0') continue;
      if ( !( isprint((int)Internal[i]) || isspace((int)Internal[i]) ) )
        {
        //gdcmWarningMacro( "Cannot print :" << i );
        return false;
        }
      }
    return true;
    }

protected:
  void Print(std::ostream &os) const {
  // This is perfectly valid to have a Length = 0 , so we cannot check
  // the length for printing
  if( !Internal.empty() )
    {
    if( IsPrintable(Length) )
      {
      // WARNING: Internal.end() != Internal.begin()+Length
      std::vector<char>::size_type length = Length;
      if( Internal.back() == 0 ) --length;
      std::copy(Internal.begin(), Internal.begin()+length,
        std::ostream_iterator<char>(os));
      }
    else
      os << "Loaded:" << Internal.size();
    }
  else
    {
    //os << "Not Loaded";
    os << "(no value available)";
    }
  }


private:
  std::vector<char> Internal;

  // WARNING Length IS NOT Internal.size() some *featured* DICOM
  // implementation define odd length, we always load them as even number
  // of byte, so we need to keep the right Length
  VL Length;
};

} // end namespace gdcm

#endif //__gdcmByteValue_h

