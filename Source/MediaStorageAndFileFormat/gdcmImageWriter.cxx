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
#include "gdcmImageWriter.h"
#include "gdcmTrace.h"
#include "gdcmDataSet.h"
#include "gdcmDataElement.h"
#include "gdcmAttribute.h"
#include "gdcmUIDGenerator.h"
#include "gdcmSystem.h"
#include "gdcmSpacingHelper.h"
#include "gdcmLookupTable.h"
#include "gdcmItem.h"
#include "gdcmSequenceOfItems.h"

namespace gdcm
{

ImageWriter::ImageWriter():PixelData()
{
}

ImageWriter::~ImageWriter()
{
}

void ImageWriter::SetImage(Image const &img)
{
  //assert( Stream.is_open() );
  const ImageValue &iv = dynamic_cast<const ImageValue&>( img );
  PixelData = iv;
  //assert( Stream.is_open() );
}

bool ImageWriter::Write()
{
  //assert( Stream.is_open() );
  File& file = GetFile();
  DataSet& ds = file.GetDataSet();

  // col & rows:
  Attribute<0x0028, 0x0011> columns;
  columns.SetValue( PixelData.GetDimension(0) );
  ds.Replace( columns.GetAsDataElement() );

  Attribute<0x0028, 0x0010> rows;
  rows.SetValue( PixelData.GetDimension(1) );
  ds.Replace( rows.GetAsDataElement() );

  // (0028,0008) IS [12]                                     #   2, 1 NumberOfFrames
  if( PixelData.GetNumberOfDimensions() == 3 )
    {
    Attribute<0x0028, 0x0008> numberofframes;
    assert( PixelData.GetDimension(2) > 1 );
    numberofframes.SetValue( PixelData.GetDimension(2) );
    ds.Replace( numberofframes.GetAsDataElement() );
    }

  PixelFormat pf = PixelData.GetPixelFormat();
  PhotometricInterpretation pi = PixelData.GetPhotometricInterpretation();
  // FIXME HACK !
  if( pf.GetBitsAllocated() == 24 )
    {
    pi = PhotometricInterpretation::RGB;
    pf.SetBitsAllocated( 8 );
    pf.SetBitsStored( 8 );
    pf.SetHighBit( 7 );
    pf.SetSamplesPerPixel( 3 );
    }
  // Pixel Format :
  // (0028,0100) US 8                                        #   2, 1 BitsAllocated
  // (0028,0101) US 8                                        #   2, 1 BitsStored
  // (0028,0102) US 7                                        #   2, 1 HighBit
  // (0028,0103) US 0                                        #   2, 1 PixelRepresentation
  Attribute<0x0028, 0x0100> bitsallocated;
  bitsallocated.SetValue( pf.GetBitsAllocated() );
  ds.Replace( bitsallocated.GetAsDataElement() );

  Attribute<0x0028, 0x0101> bitsstored;
  bitsstored.SetValue( pf.GetBitsStored() );
  ds.Replace( bitsstored.GetAsDataElement() );

  Attribute<0x0028, 0x0102> highbit;
  highbit.SetValue( pf.GetHighBit() );
  ds.Replace( highbit.GetAsDataElement() );

  Attribute<0x0028, 0x0103> pixelrepresentation;
  pixelrepresentation.SetValue( pf.GetPixelRepresentation() );
  ds.Replace( pixelrepresentation.GetAsDataElement() );

  Attribute<0x0028, 0x0002> samplesperpixel;
  samplesperpixel.SetValue( pf.GetSamplesPerPixel() );
  ds.Replace( samplesperpixel.GetAsDataElement() );

  // Overlay Data 60xx
  unsigned int nOv = PixelData.GetNumberOfOverlays();
  for( unsigned int ovidx = 0; ovidx < nOv; ++ovidx )
    {
    // (6000,0010) US 484                                      #   2, 1 OverlayRows
    // (6000,0011) US 484                                      #   2, 1 OverlayColumns
    // (6000,0015) IS [1]                                      #   2, 1 NumberOfFramesInOverlay
    // (6000,0022) LO [Siemens MedCom Object Graphics]         #  30, 1 OverlayDescription
    // (6000,0040) CS [G]                                      #   2, 1 OverlayType
    // (6000,0050) SS 1\1                                      #   4, 2 OverlayOrigin
    // (6000,0051) US 1                                        #   2, 1 ImageFrameOrigin
    // (6000,0100) US 1                                        #   2, 1 OverlayBitsAllocated
    // (6000,0102) US 0                                        #   2, 1 OverlayBitPosition
    // (6000,3000) OW 0000\0000\0000\0000\0000\0000\0000\0000\0000\0000\0000\0000\0000... # 29282, 1 OverlayData
    DataElement de;
    const Overlay &ov = PixelData.GetOverlay(ovidx);
    Attribute<0x6000,0x0010> overlayrows;
    overlayrows.SetValue( ov.GetRows() );
    de = overlayrows.GetAsDataElement();
    de.GetTag().SetGroup( ov.GetGroup() );
    ds.Insert( de );
    Attribute<0x6000,0x0011> overlaycolumns;
    overlaycolumns.SetValue( ov.GetColumns() );
    de = overlaycolumns.GetAsDataElement();
    de.GetTag().SetGroup( ov.GetGroup() );
    ds.Insert( de );
    if( ov.GetDescription() ) // Type 3
      {
      Attribute<0x6000,0x0022> overlaydescription;
      overlaydescription.SetValue( ov.GetDescription() );
      de = overlaydescription.GetAsDataElement();
      de.GetTag().SetGroup( ov.GetGroup() );
      ds.Insert( de );
      }
    Attribute<0x6000,0x0040> overlaytype; // 'G' or 'R'
    overlaytype.SetValue( ov.GetType() );
    de = overlaytype.GetAsDataElement();
    de.GetTag().SetGroup( ov.GetGroup() );
    ds.Insert( de );
    Attribute<0x6000,0x0050> overlayorigin;
    overlayorigin.SetValues( ov.GetOrigin() );
    de = overlayorigin.GetAsDataElement();
    de.GetTag().SetGroup( ov.GetGroup() );
    ds.Insert( de );
    Attribute<0x6000,0x0100> overlaybitsallocated;
    overlaybitsallocated.SetValue( ov.GetBitsAllocated() );
    de = overlaybitsallocated.GetAsDataElement();
    de.GetTag().SetGroup( ov.GetGroup() );
    ds.Insert( de );
    Attribute<0x6000,0x0102> overlaybitposition;
    overlaybitposition.SetValue( ov.GetBitPosition() );
    de = overlaybitposition.GetAsDataElement();
    de.GetTag().SetGroup( ov.GetGroup() );
    ds.Insert( de );

    // FIXME: for now rewrite 'Overlay in pixel data' still in the pixel data element...
    if( !ov.IsInPixelData() )
      {
      const ByteValue & overlaydatabv = ov.GetOverlayData();
      DataElement overlaydata( Tag(0x6000,0x3000) );
      overlaydata.SetByteValue( overlaydatabv.GetPointer(), overlaydatabv.GetLength() );
      overlaydata.SetVR( VR::OW ); // FIXME
      overlaydata.GetTag().SetGroup( ov.GetGroup() );
      ds.Insert( overlaydata );
      }
    }

  // Pixel Data
  DataElement de( Tag(0x7fe0,0x0010) );
  const Value &v = PixelData.GetDataElement().GetValue();
  de.SetValue( v );
  const ByteValue *bv = de.GetByteValue();
  const TransferSyntax &ts = PixelData.GetTransferSyntax();
  assert( ts.IsExplicit() || ts.IsImplicit() );
  VL vl;
  if( bv )
    {
    // if ts is explicit -> set VR
    vl = bv->GetLength();
    }
  else
    {
    // if ts is explicit -> set VR
    vl.SetToUndefined();
    }
  if( ts.IsExplicit() )
    {
    switch ( pf.GetBitsAllocated() )
      {
      case 8:
        de.SetVR( VR::OB );
        break;
      case 16:
        de.SetVR( VR::OW );
        break;
      default:
        assert( 0 && "should not happen" );
      }
    }
  else
    {
    de.SetVR( VR::OB );
    }
  de.SetVL( vl );
  ds.Replace( de );
  // PhotometricInterpretation
  // const Tag tphotometricinterpretation(0x0028, 0x0004);
  if( !ds.FindDataElement( Tag(0x0028, 0x0004) ) )
    {
    const char *pistr = PhotometricInterpretation::GetPIString(pi);
    DataElement de( Tag(0x0028, 0x0004 ) );
    de.SetByteValue( pistr, strlen(pistr) );
    de.SetVR( Attribute<0x0028,0x0004>::GetVR() );
    ds.Insert( de );
    if( pi == PhotometricInterpretation::RGB
      || pi == PhotometricInterpretation::YBR_FULL ) // FIXME
      {
      Attribute<0x0028, 0x0006> planarconfiguration;
      planarconfiguration.SetValue( PixelData.GetPlanarConfiguration() );
      ds.Replace( planarconfiguration.GetAsDataElement() );
      }
    else if ( pi == PhotometricInterpretation::PALETTE_COLOR )
      {
      const LookupTable &lut = PixelData.GetLUT();
      assert( pf.GetBitsAllocated() == 8 && pf.GetPixelRepresentation() == 0 );
      // lut descriptor:
      // (0028,1101) US 256\0\16                                 #   6, 3 RedPaletteColorLookupTableDescriptor
      // (0028,1102) US 256\0\16                                 #   6, 3 GreenPaletteColorLookupTableDescriptor
      // (0028,1103) US 256\0\16                                 #   6, 3 BluePaletteColorLookupTableDescriptor
      // lut data:
      unsigned short length, subscript, bitsize;
      unsigned short rawlut[256];
      unsigned int l;

      // FIXME: should I really clear rawlut each time ?
      // RED
      memset(rawlut,0,256*2);
      lut.GetLUT(LookupTable::RED, (unsigned char*)rawlut, l);
      DataElement redde( Tag(0x0028, 0x1201) );
      redde.SetVR( VR::OW );
      redde.SetByteValue( (char*)rawlut, l);
      ds.Replace( redde );
      // descriptor:
      Attribute<0x0028, 0x1101, VR::US, VM::VM3> reddesc;
      lut.GetLUTDescriptor(LookupTable::RED, length, subscript, bitsize);
      reddesc.SetValue(length,0); reddesc.SetValue(subscript,1); reddesc.SetValue(bitsize,2);
      ds.Replace( reddesc.GetAsDataElement() );

      // GREEN
      memset(rawlut,0,256*2);
      lut.GetLUT(LookupTable::GREEN, (unsigned char*)rawlut, l);
      DataElement greende( Tag(0x0028, 0x1202) );
      greende.SetVR( VR::OW );
      greende.SetByteValue( (char*)rawlut, l);
      ds.Replace( greende );
      // descriptor:
      Attribute<0x0028, 0x1102, VR::US, VM::VM3> greendesc;
      lut.GetLUTDescriptor(LookupTable::GREEN, length, subscript, bitsize);
      greendesc.SetValue(length,0); greendesc.SetValue(subscript,1); greendesc.SetValue(bitsize,2);
      ds.Replace( greendesc.GetAsDataElement() );

      // BLUE
      memset(rawlut,0,256*2);
      lut.GetLUT(LookupTable::BLUE, (unsigned char*)rawlut, l);
      DataElement bluede( Tag(0x0028, 0x1203) );
      bluede.SetVR( VR::OW );
      bluede.SetByteValue( (char*)rawlut, l);
      ds.Replace( bluede );
      // descriptor:
      Attribute<0x0028, 0x1103, VR::US, VM::VM3> bluedesc;
      lut.GetLUTDescriptor(LookupTable::BLUE, length, subscript, bitsize);
      bluedesc.SetValue(length,0); bluedesc.SetValue(subscript,1); bluedesc.SetValue(bitsize,2);
      ds.Replace( bluedesc.GetAsDataElement() );
      }
    }

  MediaStorage ms;
  ms.SetFromFile( GetFile() );
  assert( ms != MediaStorage::MS_END );
  const char* msstr = MediaStorage::GetMSString(ms);
  if( !ds.FindDataElement( Tag(0x0008, 0x0016) ) )
    {
    DataElement de( Tag(0x0008, 0x0016 ) );
    de.SetByteValue( msstr, strlen(msstr) );
    de.SetVR( Attribute<0x0008, 0x0016>::GetVR() );
    ds.Insert( de );
    }
  else
    {
    const ByteValue *bv = ds.GetDataElement( Tag(0x0008,0x0016) ).GetByteValue();
    assert( strncmp( bv->GetPointer(), msstr, bv->GetLength() ) == 0 );
    assert( bv->GetLength() == strlen( msstr ) || bv->GetLength() == strlen(msstr) + 1 );
    }

  // (re)Compute MediaStorage:
  if( !ds.FindDataElement( Tag(0x0008, 0x0060) ) )
    {
    const char *modality = ms.GetModality();
    DataElement de( Tag(0x0008, 0x0060 ) );
    de.SetByteValue( modality, strlen(modality) );
    de.SetVR( Attribute<0x0008, 0x0060>::GetVR() );
    ds.Insert( de );
    }
  else
    {
    const ByteValue *bv = ds.GetDataElement( Tag(0x0008, 0x0060 ) ).GetByteValue();
    std::string modality2 = std::string( bv->GetPointer(), bv->GetLength() );
    if( modality2 != ms.GetModality() )
      {
      DataElement de( Tag(0x0008, 0x0060 ) );
      de.SetByteValue( ms.GetModality(), strlen(ms.GetModality()) );
      de.SetVR( Attribute<0x0008, 0x0060>::GetVR() );
      ds.Replace( de );
      }
    }
  if( !ds.FindDataElement( Tag(0x0008, 0x0064) ) )
    {
    if( ms == MediaStorage::SecondaryCaptureImageStorage )
      {
      // (0008,0064) CS [SI]                                     #   2, 1 ConversionType
      const char conversion[] = "SI"; // FIXME
      DataElement de( Tag(0x0008, 0x0064 ) );
      de.SetByteValue( conversion, strlen(conversion) );
      de.SetVR( Attribute<0x0008, 0x0064>::GetVR() );
      ds.Insert( de );
      }
    }

  // Spacing:
  std::vector<double> sp;
  sp.resize(2); // important !
  sp[0] = PixelData.GetSpacing(0);
  sp[1] = PixelData.GetSpacing(1);
  if( ms != MediaStorage::SecondaryCaptureImageStorage ) 
    {
    sp.resize(3);
    sp[2] = PixelData.GetSpacing(2); // might be a dummy value...
    }
  SpacingHelper::SetSpacingValue(ds, sp);

  // UIDs:
  // (0008,0018) UI [1.3.6.1.4.1.5962.1.1.1.1.3.20040826185059.5457] #  46, 1 SOPInstanceUID
  // (0020,000d) UI [1.3.6.1.4.1.5962.1.2.1.20040826185059.5457] #  42, 1 StudyInstanceUID
  // (0020,000e) UI [1.3.6.1.4.1.5962.1.3.1.1.20040826185059.5457] #  44, 1 SeriesInstanceUID
  UIDGenerator uid;

  // Be careful with the SOP Instance UID:
  if( ds.FindDataElement( Tag(0x0008, 0x0018) ) )
    {
    // We are comming from a real DICOM image, we need to reference it...
    //assert( 0 && "TODO FIXME" );
    const Tag tsourceImageSequence(0x0008,0x2112);
    assert( ds.FindDataElement( tsourceImageSequence ) == false );
    SequenceOfItems *sq = new SequenceOfItems;
    sq->SetLengthToUndefined();
    Item item( Tag(0xfffe,0xe000) );
    de.SetVLToUndefined();
    //DataSet sourceimageds;
    // (0008,1150) UI =MRImageStorage                          #  26, 1 ReferencedSOPClassUID
    // (0008,1155) UI [1.3.6.1.4.17434.1.1.5.2.1160650698.1160650698.0] #  48, 1 ReferencedSOPInstanceUID
    DataElement referencedSOPClassUID = ds.GetDataElement( Tag(0x0008,0x0016) );
    referencedSOPClassUID.SetTag( Tag(0x0008,0x1150 ) );
    DataElement referencedSOPInstanceUID = ds.GetDataElement( Tag(0x0008,0x0018) );
    referencedSOPInstanceUID.SetTag( Tag(0x0008,0x1155) );
    //item.SetNestedDataSet( sourceimageds );
    item.SetVLToUndefined();
    item.InsertDataElement( referencedSOPClassUID );
    item.InsertDataElement( referencedSOPInstanceUID );
    sq->AddItem( item );
    DataElement de( tsourceImageSequence );
    de.SetVR( VR::SQ );
    de.SetValue( *sq );
    de.SetVLToUndefined();
    std::cout << de << std::endl;
    ds.Insert( de );
    }
    {
    const char *sop = uid.Generate();
    DataElement de( Tag(0x0008,0x0018) );
    de.SetByteValue( sop, strlen(sop) );
    de.SetVR( Attribute<0x0008, 0x0018>::GetVR() );
    ds.Insert( de );
    }

  // Are we on a particular Study ? If not create a new UID
  if( !ds.FindDataElement( Tag(0x0020, 0x000d) ) )
    {
    const char *study = uid.Generate();
    DataElement de( Tag(0x0020,0x000d) );
    de.SetByteValue( study, strlen(study) );
    de.SetVR( Attribute<0x0020, 0x000d>::GetVR() );
    ds.Insert( de );
    }

  // Are we on a particular Series ? If not create a new UID
  if( !ds.FindDataElement( Tag(0x0020, 0x000e) ) )
    {
    const char *series = uid.Generate();
    DataElement de( Tag(0x0020,0x000e) );
    de.SetByteValue( series, strlen(series) );
    de.SetVR( Attribute<0x0020, 0x000e>::GetVR() );
    ds.Insert( de );
    }

  FileMetaInformation &fmi = file.GetHeader();

  //assert( ts == TransferSyntax::ImplicitVRLittleEndian );
    {
    const char *tsuid = TransferSyntax::GetTSString( ts );
    DataElement de( Tag(0x0002,0x0010) );
    de.SetByteValue( tsuid, strlen(tsuid) );
    de.SetVR( Attribute<0x0002, 0x0010>::GetVR() );
    fmi.Insert( de );
    fmi.SetDataSetTransferSyntax(ts);
    }
  fmi.FillFromDataSet( ds );

  // Some Type 2 Element:
  // PatientName
  if( !ds.FindDataElement( Tag(0x0010,0x0010) ) )
    {
    DataElement de( Tag(0x0010,0x0010) );
    de.SetVR( Attribute<0x0010,0x0010>::GetVR() );
    ds.Insert( de );
    }
  // PatientID
  if( !ds.FindDataElement( Tag(0x0010,0x0020) ) )
    {
    DataElement de( Tag(0x0010,0x0020) );
    de.SetVR( Attribute<0x0010,0x0020>::GetVR() );
    ds.Insert( de );
    }
  // PatientBirthDate
  if( !ds.FindDataElement( Tag(0x0010,0x0030) ) )
    {
    DataElement de( Tag(0x0010,0x0030) );
    de.SetVR( Attribute<0x0010,0x0030>::GetVR() );
    ds.Insert( de );
    }
  // PatientSex
  if( !ds.FindDataElement( Tag(0x0010,0x0040) ) )
    {
    DataElement de( Tag(0x0010,0x0040) );
    de.SetVR( Attribute<0x0010,0x0040>::GetVR() );
    ds.Insert( de );
    }
  // StudyDate
  char date[18];
  const size_t datelen = 8;
  int res = System::GetCurrentDateTime(date);
  assert( res );
  if( !ds.FindDataElement( Tag(0x0008,0x0020) ) )
    {
    DataElement de( Tag(0x0008,0x0020) );
    // Do not copy the whole cstring:
    de.SetByteValue( date, datelen );
    de.SetVR( Attribute<0x0008,0x0020>::GetVR() );
    ds.Insert( de );
    }
  // StudyTime
  const size_t timelen = 6; // get rid of milliseconds
  if( !ds.FindDataElement( Tag(0x0008,0x0030) ) )
    {
    DataElement de( Tag(0x0008,0x0030) );
    // Do not copy the whole cstring:
    de.SetByteValue( date+datelen, timelen );
    de.SetVR( Attribute<0x0008,0x0030>::GetVR() );
    ds.Insert( de );
    }
  // ReferringPhysicianName
  if( !ds.FindDataElement( Tag(0x0008,0x0090) ) )
    {
    DataElement de( Tag(0x0008,0x0090) );
    de.SetVR( Attribute<0x0008,0x0090>::GetVR() );
    ds.Insert( de );
    }
  // StudyID
  if( !ds.FindDataElement( Tag(0x0020,0x0010) ) )
    {
    // FIXME: this one is actually bad since the value is needed for DICOMDIR construction
    DataElement de( Tag(0x0020,0x0010) );
    de.SetVR( Attribute<0x0020,0x0010>::GetVR() );
    ds.Insert( de );
    }
  // AccessionNumber
  if( !ds.FindDataElement( Tag(0x0008,0x0050) ) )
    {
    DataElement de( Tag(0x0008,0x0050) );
    de.SetVR( Attribute<0x0008,0x0050>::GetVR() );
    ds.Insert( de );
    }
  // SeriesNumber
  if( !ds.FindDataElement( Tag(0x0020,0x0011) ) )
    {
    DataElement de( Tag(0x0020,0x0011) );
    de.SetVR( Attribute<0x0020,0x0011>::GetVR() );
    ds.Insert( de );
    }
  // InstanceNumber
  if( !ds.FindDataElement( Tag(0x0020,0x0013) ) )
    {
    DataElement de( Tag(0x0020,0x0013) );
    de.SetVR( Attribute<0x0020,0x0013>::GetVR() );
    ds.Insert( de );
    }
  // Patient Orientation
  if( !ds.FindDataElement( Tag(0x0020,0x0020) ) )
    {
    DataElement de( Tag(0x0020,0x0020) );
    de.SetVR( Attribute<0x0020,0x0020>::GetVR() );
    ds.Insert( de );
    }
  //const char dummy[] = "dummy";
  //assert( Stream.is_open() );
  //Stream << dummy;


  assert( Stream.is_open() );
  if( !Writer::Write() )
    {
    return false;
    }
  return true;
}

} // end namespace gdcm
