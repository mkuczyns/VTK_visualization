/****************************************************************************
*   vtkVisualization.cxx
*
*   Created by:     Michael Kuczynski
*   Created on:     16/02/2019
*   Version:        1.0
*   Description:    Reads DICOM or NIfTI images and can display the 3D volume
*                   as either a surface volume using the Marching Cubes algorithm
*                   or as a ray-cast volume using ray-casting.
****************************************************************************/

#include "helperFunctions.hxx"

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
    std::cout << "Filtering input image with a 3D Gaussian filter: Standard Deviation = 1.0 \n";

    vtkSmartPointer<vtkImageGaussianSmooth> gaussian = vtkSmartPointer<vtkImageGaussianSmooth>::New();
    gaussian->SetStandardDeviation( 0.8 );
    gaussian->SetDimensionality( 3 );
    gaussian->SetInputData( volume );
    gaussian->Update();

    volume = gaussian->GetOutput();

    // Create the renderer and render window
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetBackground( 0, 0, 0 );

    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer( renderer );
    vtkSmartPointer<vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow( renderWindow );

    // Convert the render type input parameter to a lowercase string
    std::string renderType( argv[2] );
    std::transform( renderType.begin(), renderType.end(), renderType.begin(), tolower );

    if ( renderType== "surface" )
    {
        std::cout << "\nStarting surface rendering...\n";

        // Perform segmentation to extract bone
        int lowerThresh = 0, upperThresh = 0;
        double isoValue = 0.0;

        // Get the threshold and isovalue parameters from the user
        std::cout << "Performing image segmentation \n";
        std::cout << "Please enter upper and lower threshold values: \n";
        std::cout << "Lower Threshold = ";
        std::cin >> lowerThresh;
        std::cout << "Upper Threshold = ";
        std::cin >> upperThresh;

        std::cout << "Please enter the desired isovalue for the Marching Cubes algortihm: ";
        std::cin >> isoValue;

        // Apply the global threshold
        vtkSmartPointer<vtkImageThreshold> globalThresh = vtkSmartPointer<vtkImageThreshold>::New();
        globalThresh->SetInputData( volume );
        globalThresh->ThresholdBetween( lowerThresh, upperThresh );
        globalThresh->ReplaceInOn();
        globalThresh->SetInValue( isoValue + 1 );
        globalThresh->ReplaceOutOn();
        globalThresh->SetOutValue(0);
        globalThresh->SetOutputScalarTypeToFloat();
        globalThresh->Update();

        volume = globalThresh->GetOutput();

        // Use the Marching cubes algorithm to generate the surface
        std::cout << "Generating surface using Marching cubes \n";

        vtkSmartPointer<vtkMarchingCubes> surface = vtkSmartPointer<vtkMarchingCubes>::New();
        surface->SetInputData( volume );
        surface->ComputeNormalsOn();
        surface->SetValue( 0, isoValue );

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection( surface->GetOutputPort() );
        mapper->ScalarVisibilityOff();

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper( mapper );

        renderer->AddActor( actor );
    }
    else if ( renderType == "volume" )
    {
        std::cout << "\nStarting volume rendering...\n";

        // Create a ray-cast mapper for the ray-casting of the volume
        vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
        volumeMapper->SetInputData( volume );

        // Create a colour transfer function to map voxel intensities to colours.
        // For this assignment, we colour tissues in the range of 0 -> 70, 70 -> 400, and 400 -> 1000 (each with a different colour)
        vtkSmartPointer<vtkColorTransferFunction> colour = vtkSmartPointer<vtkColorTransferFunction>::New();
        colour->AddRGBPoint( 0,    0.0, 0.0, 0.0 );
        colour->AddRGBPoint( 200,   1.0, 0.0, 0.0 );
        colour->AddRGBPoint( 500,  0.0, 1.0, 0.0 );
        colour->AddRGBPoint( 1000, 0.0, 0.0, 1.0 );

        // Create a piecewise function to control the opactiy of each tissue type we're showing
        vtkSmartPointer<vtkPiecewiseFunction> scalarOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
        scalarOpacity->AddPoint( 0,    0.00 );
        scalarOpacity->AddPoint( 200,   0.15 );
        scalarOpacity->AddPoint( 500,  0.15 );
        scalarOpacity->AddPoint( 1000, 0.85 );

        // Set the gradient opacity to keep the opacity high at the boundaries between tissues
        vtkSmartPointer<vtkPiecewiseFunction> gradientOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
        gradientOpacity->AddPoint( 0,   0.0 );
        gradientOpacity->AddPoint( 100, 0.8 );

        // Set the parameters for the volume, including shading for the volume (ambient, diffuse, and specular)
        vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
        volumeProperty->SetColor( colour );
        volumeProperty->SetScalarOpacity( scalarOpacity );
        volumeProperty->SetGradientOpacity( gradientOpacity );
        volumeProperty->SetInterpolationTypeToLinear();
        volumeProperty->ShadeOn();
        volumeProperty->SetAmbient( 0.2 );
        volumeProperty->SetDiffuse( 0.8 );
        volumeProperty->SetSpecular( 0.9 );

        // Create a vtk volume and add the volume mapper and property we defined above
        vtkSmartPointer<vtkVolume> vtk_volume = vtkSmartPointer<vtkVolume>::New();
        vtk_volume->SetMapper( volumeMapper );
        vtk_volume->SetProperty( volumeProperty );

        renderer->AddViewProp( vtk_volume );
    }

    renderWindow->Render();
    interactor->Start();

    return EXIT_SUCCESS;
}