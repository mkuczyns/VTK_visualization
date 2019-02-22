/****************************************************************************
*   vtkVisualization.cxx
*
*   Created by:     Michael Kuczynski
*   Created on:     16/02/2019
*   Version:        1.0
*   Description:    
****************************************************************************/

#include "interactorStyler.hxx"

int main(int argc, char* argv[])
{
  // Check if the provided input arguement is a file or directory.
  // Assign the correct value to tissueType variable.
  if ( argc != 3 )
  {
      std::cout << "ERROR: Incorrect program usage." << std:: endl;
      std::cout << "Correct usage: " << argv[0] << " <DICOM_Folder_Name/Filename.nii> <Render_Type>" << std::endl;
      return EXIT_FAILURE;
  }

  vtkSmartPointer<vtkImageData> volume = vtkSmartPointer<vtkImageData>::New();

  vtkSmartPointer<vtkDICOMImageReader> DICOM_reader;
  vtkSmartPointer<vtkNIFTIImageReader> NIFTI_reader;

  fileType_t inputFiletype( checkInputs( argv[1] ) );

  // Create a variable for the input arguement (if valid type).
  std::string inputFile = "";

  if ( inputFiletype.type == 0 || inputFiletype.type == 1)
  {
      inputFile = argv[1];
  }

  // Assign values to appropriate variables depending on if we have a DICOM series or NIfTI file.
  if ( inputFiletype.type == -1 )
  {
      std::cout << "ERROR: Invalid input arguement(s)." << std::endl;
      std::cout << "Correct usage: " << argv[0] << " <TISSUE_TYPE> <MORPHOLOGICAL_OPERATOR> <DICOM_Folder_Name/Filename.nii>" << std::endl;
      return EXIT_FAILURE;
  }

  switch( inputFiletype.type )
  {
      case 0:     // Directory with DICOM series
      {
          // Read all files from the DICOM series in the specified directory.
          DICOM_reader = vtkSmartPointer<vtkDICOMImageReader>::New();
          DICOM_reader->SetDirectoryName( inputFile.c_str() );
          DICOM_reader->Update();

          volume->DeepCopy( DICOM_reader->GetOutput() );

          break;
      }

      case 1:     // NIfTI
      {
          // Create a reader and check if the input file is readable.
          NIFTI_reader = vtkSmartPointer<vtkNIFTIImageReader>::New();

          if ( !( NIFTI_reader->CanReadFile( inputFile.c_str() ) ) )
          {
              std::cout << "ERROR: vtk NIFTI image reader cannot read the provided file: " << inputFile << std::endl;
              return EXIT_FAILURE;
          }

          NIFTI_reader->SetFileName( inputFile.c_str() );
          NIFTI_reader->Update();

          volume->DeepCopy( NIFTI_reader->GetOutput() );

          break;
      }

      default:
      {
          return EXIT_FAILURE;
      }
  } 

  // Apply a Guassian filter before segmentation
  // std::cout << "Filtering input image with a 3D Gaussian filter: Standard Deviation = 1.0 \n";

  // vtkSmartPointer<vtkImageGaussianSmooth> gaussian = vtkSmartPointer<vtkImageGaussianSmooth>::New();
  // gaussian->SetStandardDeviation( 1.5 );
  // gaussian->SetDimensionality( 3 );
  // gaussian->SetInputData( volume );
  // gaussian->Update();

  // volume = gaussian->GetOutput();

  // // Perform segmentation to extract bone
  // std::cout << "Performing image segmentation for bone \n";

  // vtkSmartPointer<vtkImageThreshold> globalThresh = vtkSmartPointer<vtkImageThreshold>::New();
  // globalThresh->SetInputData( volume );
  // globalThresh->ThresholdBetween( 200, 1000 );
  // globalThresh->ReplaceInOn();
  // globalThresh->SetInValue(1000);
  // globalThresh->ReplaceOutOn();
  // globalThresh->SetOutValue(0);
  // globalThresh->SetOutputScalarTypeToFloat();
  // globalThresh->Update();

  // volume = globalThresh->GetOutput();

  // // Use the Marching cubes algorithm to generate the surface
  // std::cout << "Generating surface using Marching cubes \n";

  // vtkSmartPointer<vtkMarchingCubes> surface = vtkSmartPointer<vtkMarchingCubes>::New();
  // surface->SetInputData( volume );
  // surface->ComputeNormalsOn();
  // surface->SetValue( 0, 50.0 );

  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->SetBackground( 0, 0, 0 );

  vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->AddRenderer( renderer );
  vtkSmartPointer<vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  interactor->SetRenderWindow( renderWindow );

  // vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  // mapper->SetInputConnection( surface->GetOutputPort() );
  // mapper->ScalarVisibilityOff();

  // vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  // actor->SetMapper( mapper );

  // renderer->AddActor( actor );

  // The volume will be displayed by ray-cast alpha compositing.
  // A ray-cast mapper is needed to do the ray-casting.
  vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
  volumeMapper->SetInputData( volume );

  // The color transfer function maps voxel intensities to colors.
  // It is modality-specific, and often anatomy-specific as well.
  // The goal is to one color for flesh (between 500 and 1000)
  // and another color for bone (1150 and over).
  vtkSmartPointer<vtkColorTransferFunction>volumeColor = vtkSmartPointer<vtkColorTransferFunction>::New();
  volumeColor->AddRGBPoint(0,    0.0, 0.0, 0.0);
  volumeColor->AddRGBPoint(70,  1.0, 0.5, 0.3);
  volumeColor->AddRGBPoint(500, 1.0, 0.5, 0.3);
  volumeColor->AddRGBPoint(1150, 1.0, 1.0, 0.9);

  // The opacity transfer function is used to control the opacity
  // of different tissue types.
  vtkSmartPointer<vtkPiecewiseFunction> volumeScalarOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
  volumeScalarOpacity->AddPoint(0,    0.00);
  volumeScalarOpacity->AddPoint(70,  0.15);
  volumeScalarOpacity->AddPoint(500, 0.15);
  volumeScalarOpacity->AddPoint(1150, 0.85);

  // The gradient opacity function is used to decrease the opacity
  // in the "flat" regions of the volume while maintaining the opacity
  // at the boundaries between tissue types.  The gradient is measured
  // as the amount by which the intensity changes over unit distance.
  // For most medical data, the unit distance is 1mm.
  vtkSmartPointer<vtkPiecewiseFunction> volumeGradientOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
  volumeGradientOpacity->AddPoint(0,   0.0);
  volumeGradientOpacity->AddPoint(90,  0.5);
  volumeGradientOpacity->AddPoint(100, 1.0);

  // The VolumeProperty attaches the color and opacity functions to the
  // volume, and sets other volume properties.  The interpolation should
  // be set to linear to do a high-quality rendering.  The ShadeOn option
  // turns on directional lighting, which will usually enhance the
  // appearance of the volume and make it look more "3D".  However,
  // the quality of the shading depends on how accurately the gradient
  // of the volume can be calculated, and for noisy data the gradient
  // estimation will be very poor.  The impact of the shading can be
  // decreased by increasing the Ambient coefficient while decreasing
  // the Diffuse and Specular coefficient.  To increase the impact
  // of shading, decrease the Ambient and increase the Diffuse and Specular.
  vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
  volumeProperty->SetColor(volumeColor);
  volumeProperty->SetScalarOpacity(volumeScalarOpacity);
  volumeProperty->SetGradientOpacity(volumeGradientOpacity);
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->ShadeOn();
  volumeProperty->SetAmbient(0.4);
  volumeProperty->SetDiffuse(0.6);
  volumeProperty->SetSpecular(0.2);

  // The vtkVolume is a vtkProp3D (like a vtkActor) and controls the position
  // and orientation of the volume in world coordinates.
  vtkSmartPointer<vtkVolume> vol = vtkSmartPointer<vtkVolume>::New();
  vol->SetMapper(volumeMapper);
  vol->SetProperty(volumeProperty);

  // Finally, add the volume to the renderer
  renderer->AddViewProp(vol);






  renderWindow->Render();
  interactor->Start();
  return EXIT_SUCCESS;
}