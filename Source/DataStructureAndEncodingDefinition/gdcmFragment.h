/*=========================================================================

  Program: GDCM (Grass Root DICOM). A DICOM library
  Module:  $URL$

  Copyright (c) 2006-2007 Mathieu Malaterre
  Copyright (c) 1993-2005 CREATIS
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __gdcmFragment_h
#define __gdcmFragment_h

#include "gdcmDataElement.h"
#include "gdcmByteValue.h"
#include "gdcmSmartPointer.h"

namespace gdcm
{
/**
 * \brief Class to represent an Fragment
 */

class GDCM_EXPORT Fragment : public DataElement
{
public:
  Fragment(const Tag &t = Tag(0), uint32_t const &vl = 0) : DataElement(t, vl) , FragmentValue(0) { }
  friend std::ostream &operator<<(std::ostream &os, const Fragment &val);

  void Clear() {
    FragmentValue->Clear();
    }

  Value const &GetValue() const {
    return *FragmentValue;
  }

  VL GetLength() const {
    assert( !ValueLengthField.IsUndefined() );
    assert( !FragmentValue || FragmentValue->GetLength() == ValueLengthField );
    return TagField.GetLength() + ValueLengthField.GetLength() 
      + ValueLengthField;
  }

  Fragment(const Fragment &val):DataElement(val)
    {
    FragmentValue = val.FragmentValue;
    }
  Fragment &operator=(Fragment const &val)
    {
    FragmentValue = val.FragmentValue;
    return *this;
    }

  template <typename TSwap>
  std::istream &Read(std::istream &is)
    {
    // Superclass
    const Tag itemStart(0xfffe, 0xe000);
    const Tag seqDelItem(0xfffe,0xe0dd);
    if( !TagField.Read<TSwap>(is) )
      {
      assert(0 && "Should not happen");
      return is;
      }
    if( !ValueLengthField.Read<TSwap>(is) )
      {
      assert(0 && "Should not happen");
      return is;
      }
#ifdef GDCM_SUPPORT_BROKEN_IMPLEMENTATION
    if( TagField != itemStart && TagField != seqDelItem )
      {
      throw Exception( "Problem" );
      }
#endif
    // Self
    ByteValue *bv = new ByteValue;
    bv->SetLength(ValueLengthField);
    if( !bv->Read<TSwap>(is) )
      {
      assert(0 && "Should not happen");
      return is;
      }
    FragmentValue = bv;
    return is;
    }


  template <typename TSwap>
  std::ostream &Write(std::ostream &os) const
  {
    const Tag itemStart(0xfffe, 0xe000);
    const Tag seqDelItem(0xfffe,0xe0dd);
    if( !TagField.Write<TSwap>(os) )
      {
      assert(0 && "Should not happen");
      return os;
      }
    assert( TagField == itemStart
         || TagField == seqDelItem );
    if( !ValueLengthField.Write<TSwap>(os) )
      {
      assert(0 && "Should not happen");
      return os;
      }
    // Self
    if( !FragmentValue->Write<TSwap>(os) )
      {
      assert(0 && "Should not happen");
      return os;
      }
    return os;
    }

 
private:
  typedef SmartPointer<ByteValue> ByteValuePtr;
  ByteValuePtr FragmentValue;
};
//-----------------------------------------------------------------------------
inline std::ostream &operator<<(std::ostream &os, const Fragment &val)
{
  os << "Tag: " << val.TagField;
  os << "\tVL: " << val.ValueLengthField;
  os << "\t" << *(val.FragmentValue);

  return os;
}


} // end namespace gdcm

#endif //__gdcmFragment_h
