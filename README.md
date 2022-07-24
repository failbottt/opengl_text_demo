### OpenGL Text Demo

This is a simple, C-based demo of how to render text to a screen using Opengl.

### Dependencies
* make
* gcc
* cmake (for glfw)
* glfw (included in this repo): for cross-platform Window creation and I/O
* FreeType

### How to build

Prerequisites: 
- You'll need to compile GLFW (located in `./external/glfw`) for your platform. You can read the instructions [here](https://www.glfw.org/docs/3.0/compile.html).
- You'll need to install FreeType2 to your machine, and potentially update the include path for freetype (`-I/usr/include/freetype2`) in the Makefile

Then run `make && ./opengl_text_demo` to compile and run the demo.

### Disclaimer:

I've only run this on Linux. If you're on Windows or Mac you might have to work through your own set of compiler errors.
