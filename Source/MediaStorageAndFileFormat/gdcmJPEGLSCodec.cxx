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
#include "gdcmJPEGLSCodec.h"
#include "gdcmTransferSyntax.h"
#include "gdcmDataElement.h"
#include "gdcmFilename.h"
#include "gdcmSystem.h"
#include "gdcmSequenceOfFragments.h"
#include "gdcmPNMCodec.h"

namespace gdcm
{

JPEGLSCodec::JPEGLSCodec():BufferLength(0)
{
}

JPEGLSCodec::~JPEGLSCodec()
{
}

bool JPEGLSCodec::CanDecode(TransferSyntax const &ts) const
{
  return ts == TransferSyntax::JPEGLSLossless 
    || ts == TransferSyntax::JPEGLSNearLossless;
}

bool JPEGLSCodec::CanCode(TransferSyntax const &ts) const
{
  return ts == TransferSyntax::JPEGLSLossless 
    || ts == TransferSyntax::JPEGLSNearLossless;
}

bool JPEGLSCodec::Decode(DataElement const &in, DataElement &out)
{
  // First thing create a jpegls file from the fragment:
  const gdcm::SequenceOfFragments *sf = in.GetSequenceOfFragments();
  assert(sf);

  char *input  = tempnam(0, "gdcminjpegls");
  char *output = tempnam(0, "gdcmoutjpegls");
  if( !input || !output ) 
    {
    //free(input);
    //free(output);
    return false;
    }

  std::ofstream outfile(input, std::ios::binary);
  sf->WriteBuffer(outfile);
  outfile.close(); // flush !

  gdcm::Filename fn( System::GetCurrentProcessFileName() );
  std::string executable_path = fn.GetPath();
  std::string locod_command = executable_path + "/gdcmlocod ";
  locod_command += "-i";
  locod_command += input; // no space !
  locod_command += " -o";
  locod_command += output; // no space !

  std::cerr << locod_command << std::endl;
  int ret = system(locod_command.c_str());
  //std::cerr << "system: " << ret << std::endl;

  PNMCodec pnm;
  pnm.SetBufferLength( GetBufferLength() );
  bool b = pnm.Read( output, out );

  if( !System::RemoveFile(input) )
    {
    gdcmErrorMacro( "Could not delete input: " << input );
    }

  if( !System::RemoveFile(output) )
    {
    gdcmErrorMacro( "Could not delete output: " << output );
    }

  free(input);
  free(output);

  return true;
}

// Compress into JPEG
bool JPEGLSCodec::Code(DataElement const &in, DataElement &out)
{
  out = in;
  // First thing create a pnm file from the fragment:
  PNMCodec pnm;
  pnm.SetDimensions( this->GetDimensions() );
  char *input  = tempnam(0, "gdcminjpegls");
  char *output = tempnam(0, "gdcmoutjpegls");
  if( !input || !output ) 
    {
    //free(input);
    //free(output);
    return false;
    }
  pnm.Write( input, in );

  gdcm::Filename fn( System::GetCurrentProcessFileName() );
  std::string executable_path = fn.GetPath();
  std::string locoe_command = executable_path + "/gdcmlocoe ";
  locoe_command += "-i";
  locoe_command += input; // no space !
  locoe_command += " -o";
  locoe_command += output; // no space !

  std::cerr << locoe_command << std::endl;
  int ret = system(locoe_command.c_str());
  //std::cerr << "system: " << ret << std::endl;

  size_t len = gdcm::System::FileSize(output);
  std::ifstream is(output);
  char *buf = new char[len];
  is.read(buf, len);

  // Create a Sequence Of Fragments:
  SmartPointer<SequenceOfFragments> sq = new SequenceOfFragments;
  const Tag itemStart(0xfffe, 0xe000);
  sq->GetTable().SetTag( itemStart );

    Fragment frag;
    frag.SetTag( itemStart );
    frag.SetByteValue( buf, len );
    sq->AddFragment( frag );
  out.SetValue( *sq );

  delete[] buf;

 
  if( !System::RemoveFile(input) )
    {
    gdcmErrorMacro( "Could not delete input: " << input );
    }

  if( !System::RemoveFile(output) )
    {
    gdcmErrorMacro( "Could not delete output: " << output );
    }

  free(input);
  free(output);

  return true;

  return true;
}

} // end namespace gdcm
