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
#include "gdcmStringFilter.h"
#include "gdcmGlobal.h"
#include "gdcmElement.h"
#include "gdcmByteValue.h"

namespace gdcm
{

//-----------------------------------------------------------------------------
StringFilter::StringFilter()
{
}
//-----------------------------------------------------------------------------
StringFilter::~StringFilter()
{
}

void StringFilter::SetDicts(const Dicts &dicts)
{
  abort(); // FIXME
}

#define StringFilterCase(type) \
  case VR::type: \
    { \
      Element<VR::type,VM::VM1_n> el; \
      if( !de.IsValueEmpty() ) { \
      el.Set( de.GetValue() ); \
      if( el.GetLength() ) { \
      os << el.GetValue(); \
      for(unsigned long i = 1; i < el.GetLength(); ++i) os << "\\" << el.GetValue(i); \
      ret.second = os.str(); } } \
    } break

std::pair<std::string, std::string> StringFilter::ToStringPair(const DataElement& de) const
{
  std::pair<std::string, std::string> ret;
  const Global &g = GlobalInstance;
  const Dicts &dicts = g.GetDicts();
  if( de.GetTag().IsPrivate() )
    {
    return ret;
    }
  assert( de.GetTag().IsPublic() );
  const DictEntry &entry = dicts.GetDictEntry(de.GetTag());
  if( entry.GetVR() == VR::INVALID )
    {
    // FIXME This is a public element we do not support...
    //throw Exception();
    return ret;
    }

  VR vr = entry.GetVR();
  // If Explicit override with coded VR:
  if( de.GetVR() != VR::INVALID && de.GetVR() != VR::UN )
    {
    vr = de.GetVR();
    }
  assert( vr != VR::UN && vr != VR::INVALID );
  //std::cerr << "Found " << vr << " for " << de.GetTag() << std::endl;
  ret.first = entry.GetName();
  if( VR::IsASCII( vr ) )
    {
    const ByteValue *bv = de.GetByteValue();
    if( de.GetVL() )
      {
      assert( bv /*|| bv->IsEmpty()*/ );
      ret.second = std::string( bv->GetPointer(), bv->GetLength() );
      // Let's remove any trailing \0 :
      ret.second.resize( std::min( ret.second.size(), strlen( ret.second.c_str() ) ) ); // strlen is garantee to be lower or equal to ::size()
      }
    else
      {
      //assert( bv == NULL );
      ret.second = ""; // ??
      }
    }
  else
    {
    assert( vr & VR::VRBINARY );
    const ByteValue *bv = de.GetByteValue();
    std::ostringstream os;
    if( bv )
      {
      VM::VMType vm = entry.GetVM();
      //assert( vm == VM::VM1 );
      std::ostringstream os;
      switch(vr)
        {
        StringFilterCase(AT);
        StringFilterCase(FL);
        StringFilterCase(FD);
        //StringFilterCase(OB);
        StringFilterCase(OF);
        //StringFilterCase(OW);
        StringFilterCase(SL);
        //StringFilterCase(SQ);
        StringFilterCase(SS);
        StringFilterCase(UL);
        //StringFilterCase(UN);
        StringFilterCase(US);
        StringFilterCase(UT);
      case VR::OB:
      case VR::OW:
      case VR::OB_OW:
      case VR::US_SS:
      case VR::SQ:
      case VR::UN:
        gdcmWarningMacro( "Unhandled: " << vr << " for tag " << de.GetTag() );
        ret.second = "";
        break;
      default:
        abort();
        break;
        }
      }
    }
  return ret;
}

}
