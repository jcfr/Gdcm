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
#include "gdcmMediaStorage.h"
#include "gdcmTag.h"
#include "gdcmByteValue.h"
#include "gdcmDataSet.h"
#include "gdcmFileMetaInformation.h"
#include "gdcmFile.h"

namespace gdcm
{

static const char *MSStrings[] = {
  "1.2.840.10008.1.3.10",
  "1.2.840.10008.5.1.4.1.1.1",    
  "1.2.840.10008.5.1.4.1.1.1.1",  
  "1.2.840.10008.5.1.4.1.1.1.1.1",
  "1.2.840.10008.5.1.4.1.1.1.2",  
  "1.2.840.10008.5.1.4.1.1.1.2.1",
  "1.2.840.10008.5.1.4.1.1.1.3",  
  "1.2.840.10008.5.1.4.1.1.1.3.1",
  "1.2.840.10008.5.1.4.1.1.2",    
  "1.2.840.10008.5.1.4.1.1.2.1",  
  "1.2.840.10008.5.1.4.1.1.3",    
  "1.2.840.10008.5.1.4.1.1.3.1",  
  "1.2.840.10008.5.1.4.1.1.4",    
  "1.2.840.10008.5.1.4.1.1.4.1",  
  "1.2.840.10008.5.1.4.1.1.4.2",  
  "1.2.840.10008.5.1.4.1.1.5",    
  "1.2.840.10008.5.1.4.1.1.6",    
  "1.2.840.10008.5.1.4.1.1.6.1",  
  "1.2.840.10008.5.1.4.1.1.7",    
  "1.2.840.10008.5.1.4.1.1.7.1",  
  "1.2.840.10008.5.1.4.1.1.7.2",  
  "1.2.840.10008.5.1.4.1.1.7.3",  
  "1.2.840.10008.5.1.4.1.1.7.4",  
  "1.2.840.10008.5.1.4.1.1.8",    
  "1.2.840.10008.5.1.4.1.1.9",    
  "1.2.840.10008.5.1.4.1.1.9.1.1",
  "1.2.840.10008.5.1.4.1.1.9.1.2",
  "1.2.840.10008.5.1.4.1.1.9.1.3",
  "1.2.840.10008.5.1.4.1.1.9.2.1",
  "1.2.840.10008.5.1.4.1.1.9.3.1",
  "1.2.840.10008.5.1.4.1.1.9.4.1",
  "1.2.840.10008.5.1.4.1.1.10",   
  "1.2.840.10008.5.1.4.1.1.11",   
  "1.2.840.10008.5.1.4.1.1.11.1", 
  "1.2.840.10008.5.1.4.1.1.12.1", 
  "1.2.840.10008.5.1.4.1.1.12.2", 
  "1.2.840.10008.5.1.4.1.1.12.3", 
  "1.2.840.10008.5.1.4.1.1.20",   
  "1.2.840.10008.5.1.4.1.1.66",   
  "1.2.840.10008.5.1.4.1.1.66.1", 
  "1.2.840.10008.5.1.4.1.1.66.2", 

  // See PETAt001_PT001.dcm
  "1.2.840.10008.5.1.4.1.1.128",
  // SYNGORTImage.dcm
  "1.2.840.10008.5.1.4.1.1.481.1",
  // eclipse_dose.dcm
  "1.2.840.10008.5.1.4.1.1.481.2",
  // exRT_Structure_Set_Storage.dcm
  "1.2.840.10008.5.1.4.1.1.481.3",
  // eclipse_plan.dcm
  "1.2.840.10008.5.1.4.1.1.481.5",
  // exCSA_Non-Image_Storage.dcm
  "1.3.12.2.1107.5.9.1",
  // 3DDCM011.dcm
  "1.2.840.113543.6.6.1.3.10002",
  0
};

const MediaStorage::MSType MediaStorage::GetMSType(const char *str)
{
  assert( std::string(str).find( ' ' ) == std::string::npos ); // no space allowed in UI
  int i = 0;
  while(MSStrings[i] != 0)
    {
    if( strcmp(str, MSStrings[i]) == 0 )
      return (MSType)i;
    ++i;
    }
  return MS_END;
}

const char* MediaStorage::GetMSString(MSType ms)
{
  assert( ms <= MS_END );
  return MSStrings[(int)ms];
}

// FIXME
// Currently the implementation is bogus it only define the TS which
// are associated with an image so indeed the implementation of IsImage 
// is only the verification of TSType is != TS_END
bool MediaStorage::IsImage(const MSType &ms)
{
  if ( ms == MS_END // most frequent first
    // lexicographical order then...
    || ms == BasicVoiceAudioWaveformStorage
    || ms == CSANonImageStorage
    || ms == HemodynamicWaveformStorage
    || ms == MediaStorageDirectoryStorage
    || ms == RTPlanStorage
    || ms == GrayscaleSoftcopyPresentationStateStorageSOPClass
    || ms == CardiacElectrophysiologyWaveformStorage
    || ms == RTStructureSetStorage )
    {
    return false;
    }
  return true;
}

struct MSModalityType
{
  const char *Modality;
  const unsigned int Dimension;
};

static MSModalityType MSModalityTypes[] = {
  {"  ", 2},//MediaStorageDirectoryStorage,
  {"CR", 2},//ComputedRadiographyImageStorage,
  {"  ", 2},//DigitalXRayImageStorageForPresentation,
  {"  ", 2},//DigitalXRayImageStorageForProcessing,
  {"  ", 2},//DigitalMammographyImageStorageForPresentation,
  {"  ", 2},//DigitalMammographyImageStorageForProcessing,
  {"  ", 2},//DigitalIntraoralXrayImageStorageForPresentation,
  {"  ", 2},//DigitalIntraoralXRayImageStorageForProcessing,
  {"CT", 2},//CTImageStorage,
  {"  ", 2},//EnhancedCTImageStorage,
  {"  ", 2},//UltrasoundMultiFrameImageStorageRetired,
  {"  ", 2},//UltrasoundMultiFrameImageStorage,
  {"MR", 2},//MRImageStorage,
  {"MR", 3},//EnhancedMRImageStorage,
  {"  ", 2},//MRSpectroscopyStorage,
  {"NM", 2},//NuclearMedicineImageStorageRetired,
  {"US", 2},//UltrasoundImageStorageRetired,
  {"US", 2},//UltrasoundImageStorage,
  {"OT", 2},//SecondaryCaptureImageStorage,
  {"  ", 2},//MultiframeSingleBitSecondaryCaptureImageStorage,
  {"  ", 2},//MultiframeGrayscaleByteSecondaryCaptureImageStorage,
  {"  ", 2},//MultiframeGrayscaleWordSecondaryCaptureImageStorage,
  {"  ", 2},//MultiframeTrueColorSecondaryCaptureImageStorage,
  {"  ", 2},//StandaloneOverlayStorage,
  {"  ", 2},//StandaloneCurveStorage,
  {"  ", 2},//LeadECGWaveformStorage, // 12-
  {"  ", 2},//GeneralECGWaveformStorage,
  {"  ", 2},//AmbulatoryECGWaveformStorage,
  {"  ", 2},//HemodynamicWaveformStorage,
  {"  ", 2},//CardiacElectrophysiologyWaveformStorage,
  {"  ", 2},//BasicVoiceAudioWaveformStorage,
  {"  ", 2},//StandaloneModalityLUTStorage,
  {"  ", 2},//StandaloneVOILUTStorage,
  {"  ", 2},//GrayscaleSoftcopyPresentationStateStorageSOPClass,
  {"  ", 2},//XRayAngiographicImageStorage,
  {"  ", 2},//XRayRadiofluoroscopingImageStorage,
  {"  ", 2},//XRayAngiographicBiPlaneImageStorageRetired,
  {"  ", 2},//NuclearMedicineImageStorage,
  {"  ", 2},//RawDataStorage,
  {"  ", 2},//SpacialRegistrationStorage,
  {"  ", 2},//SpacialFiducialsStorage,
  {"  ", 2},//PETImageStorage,
  {"  ", 2},//RTImageStorage,
  {"  ", 2},//RTDoseStorage,
  {"  ", 2},//RTStructureSetStorage,
  {"  ", 2},//RTPlanStorage,
  {"  ", 2},//CSANonImageStorage,
  {"  ", 2},//Philips3D,
  {"  ", 2},//MS_END
  {NULL, 0}
};

const char *MediaStorage::GetModality() const
{
  return MSModalityTypes[MSField].Modality;
}

void MediaStorage::SetFromHeader(FileMetaInformation const &fmi)
{
  const Tag mediastoragesopclassuid(0x0002, 0x0002);
  if( fmi.FindDataElement( mediastoragesopclassuid ) )
    {
    const ByteValue *sopclassuid = fmi.GetDataElement( mediastoragesopclassuid ).GetByteValue();
    std::string sopclassuid_str(
      sopclassuid->GetPointer(),
      sopclassuid->GetLength() );
    assert( sopclassuid_str.find( ' ' ) == std::string::npos );
    MediaStorage ms = MediaStorage::GetMSType(sopclassuid_str.c_str());
    if( ms == MS_END )
      {
      // weird something was found, but we not find the MS anyway...
      gdcmWarningMacro( "Does not know what: " << sopclassuid_str << " is..." );
      }
    MSField = ms;
    }
}

void MediaStorage::GuessFromModality(const char *modality, unsigned int dim)
{
  if( !modality ) return;
  if( strlen(modality) != 2 ) return;
  int i = 0;
  while( MSModalityTypes[i].Modality && 
    (strcmp(modality, MSModalityTypes[i].Modality) != 0 || MSModalityTypes[i].Dimension < dim ))
    {
    ++i;
    }
  if( MSModalityTypes[i].Modality )
    {
    // Ok we found something...
    MSField = (MSType)i;
    }
}

void MediaStorage::SetFromDataSet(DataSet const &ds, bool guess)
{
  const Tag tsopclassuid(0x0008, 0x0016);
  if( ds.FindDataElement( tsopclassuid ) )
    {
    const ByteValue *sopclassuid = ds.GetDataElement( tsopclassuid ).GetByteValue();
    std::string sopclassuid_str(
      sopclassuid->GetPointer(),
      sopclassuid->GetLength() );
    if( sopclassuid_str.find( ' ' ) != std::string::npos )
      {
      gdcmWarningMacro( "UI contains a space character discarding" );
      std::string::size_type pos = sopclassuid_str.find_last_of(' ');
      sopclassuid_str = sopclassuid_str.substr(0,pos);
      }
    MediaStorage ms = MediaStorage::GetMSType(sopclassuid_str.c_str());
    assert( ms != MS_END );
    MSField = ms;
    }
  else if( guess )
    {
    // If user ask to guess MediaStorage, let's try again
    assert( MSField == MediaStorage::MS_END );
    SetFromModality( ds );
    }
}

void MediaStorage::SetFromModality(DataSet const &ds)
{
  // Ok let's try againg with little luck it contains a pixel data...
  if( ds.FindDataElement( Tag(0x7fe0,0x0010) ) )
    {
    // Pixel Data found !
    // Attempt to recover from the modality (0008,0060):
    if( ds.FindDataElement( Tag(0x0008,0x0060) ) )
      {
      // gdcm-CR-DCMTK-16-NonSamplePerPix.dcm
      // Someone defined the Transfer Syntax but I have no clue what
      // it is. Since there is Pixel Data element, let's try to read
      // that as a buggy DICOM Image file...
      const ByteValue *bv = ds.GetDataElement( Tag(0x0008,0x0060) ).GetByteValue();
      if( bv )
        {
        std::string modality = std::string( bv->GetPointer(), bv->GetLength() );
        GuessFromModality( modality.c_str() );
        }
      }
    // We know there is a Pixel Data element, so make sure not to return without a default
    // to SC Object:
    if( MSField == MediaStorage::MS_END )
      {
      MSField = MediaStorage::SecondaryCaptureImageStorage;
      }
    }
}

void MediaStorage::SetFromFile(File const &file)
{
  const DataSet &ds = file.GetDataSet();
  const FileMetaInformation &header = file.GetHeader();
  SetFromDataSet( ds );
  if( MSField == MediaStorage::MS_END ) // Nothing found...
    {
    // try again but from header this time:
    SetFromHeader( header );
    if( MSField == MediaStorage::MS_END ) // Nothing found...
      {
      // Attempt to read what's in Modality:
      SetFromModality( ds );
      }
    }
}

} // end namespace gdcm
