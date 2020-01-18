# ribanfblib
A very simple graphics library for the (Linux) framebuffer.

This library provides graphics elements including lines, shapes and text painted onto the framebuffer. It is simple and inefficient so does not scale to high refresh-rate applications but may be suitable for user interfaces, e.g. framebuffers shown on external small TFT displays. The library may be instantiated on any framebuffer and detects resolution and colour format. It does not support all formats but will display correctly on any framebuffer configurations it supports. Colour depths of 8, 16, 24 & 32 bits are supported.

Shapes provided by the library are:

* Pixel
* Straight line
* Triangle
* Rectangle
* Circle
* Text
* Bitmap

Each shape may have a border of configurable thickness and colour and fill colour. Rectangles may have any of its corners curved. Text may use any font supported by FreeType and be sized and rotated.

Coordinates are inverted cartesian, i.e. (0,0) is at the top left of the screen. Coordinate of text is to the bottom left of the start of the text. Text rotation angle is in degrees, anticlockwise from horizontal orientation.

The main purpose of this library is to provide a simple user interface on a small TFT screen. Having searched for an existing toolkit I found there were feature-rich (and hence large and complex) toolkits such as wxWidgets, QT, etc. and there were low-level libraries requiring excessive coding. There were some that might meet my requirements but they were heavy on dependencies or complex to configure. The aim of this library is to be simple to use. It is not optimised for speed and does not purport to be a complete or advance toolkit. I am open to suggestes for improvement but do not intend to extend this library towards the feature set of existing larger libraries.

# Dependencies

* Freetype
* Arash Partow's bitmap library (https://raw.githubusercontent.com/ArashPartow/bitmap/master/bitmap_image.hpp)

To build an application called *fbtest* from a source file called main.cpp use the following command:

` g++ -o fbtest -I/usr/include/freetype2 -lfreetype main.cpp ribanfblib.cpp`

(This assumes freetype2 include files are located in /usr/include/freetype2 and libfreetype.o (or libfreetype.so) is in the linker path. Adjust to suit.)

# Shared Library

The library may be compiled as a shared library:

```
g++ -fpic -c -Wall -Werror -I/usr/include/freetype2 ribanfblib.cpp
g++ -shared ribanfblib.o -lfreetype -lstdc++ -o libribanfb.so
sudo cp libribanfb.so /usr/local/lib
sudo ldconfig /usr/local/lib
```

TODO: Create Makefile to perform these actions and compile test application.
