# Simple render in OpenGL

### Implementation of a renderer that reads a CSV file to load the vertices

Basically the vertices are specified with 3 Positions plus 3 Colors and plus 2 Texture Coordinates, as the image below.

### Vertices layout
![Vertices layout](https://i.imgur.com/L9um8Vz.png)

At the **Application.cpp** file there is a method called **read_csv_file** where the file to be loaded is specified. In this project there are two available files, which are pretty simple, a 3D cube (CuboMulticolorido.csv) and a 3D House (House.csv).

### An example of the result
![An example of the result](https://i.imgur.com/mxrEqoB.png)
