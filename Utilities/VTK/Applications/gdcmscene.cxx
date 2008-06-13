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
#include "vtkGDCMPolyDataReader.h"

#include "vtkAppendPolyData.h"
#include "vtkPolyDataWriter.h"

int main(int argc, char *argv[])
{
  if( argc < 2 )
    {
    std::cerr << argv[0] << " filename1.dcm [filename2.dcm ...]\n";
    return 1;
    }
  const char * filename = argv[1];

  vtkGDCMPolyDataReader * reader = vtkGDCMPolyDataReader::New();
  reader->SetFileName( filename );
  reader->Update();

  reader->Print( std::cout );
  reader->GetOutput()->Print( std::cout );


  vtkPolyDataWriter * writer = vtkPolyDataWriter::New();
  writer->SetInput( reader->GetOutput() );
  writer->SetFileName( "rtstruct.vtk" );
  writer->Write();
  
  reader->Delete();
  writer->Delete();

  return 0;
}

