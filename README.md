# VTK_visualization

# Features
1. Reads and displays 3D renderings of DICOM or NIfTI images.
2. Applies a Gaussian filter for smoothing.
3. You have the option to use the Marching Cubes algorithm to generate a surface rendering, or use a ray-casting method to create a volume rendering with different tissue types shown in different colours.
4. Rotate the volume by clicking and dragging with the left mouse button.
5. Zoom in and out by clicking and dragging the right mouse buttom.

# How to Run
1. Create a folder for the build (e.g. bin, build, etc.)
2. Build with CMake and your favorite compiler.
3. Run the executable that is generated in the bin\Debug folder from the command line
   
    ```
    vtkVisualization.exe <PATH_TO_DICOM_FOLDER> <RENDER_TYPE>
    ```
    OR:

    ```
    myImageViewer.exe <NIfTI_IMAGE_FILE>.nii <RENDER_TYPE>
    ```
    
4. Render types include surface (Marching Cubes) or volume (Ray-casting). This arguement can be any combination of uppercase and lowercase letters.