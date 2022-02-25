# MonkSVG

![Tiger](tiger.png)
## Overview

MonkSVG is an SVG parsing and rendering library.  It is designed to use various vector graphics backends with the current backend implementation using [MonkVG](https://github.com/micahpearlman/MonkVG).

## Installation & Build

NOTE: It's important to use `--recursive` when when cloning to clone any submodules.

```
# Clone the project. USE `--recursive`
git clone --recursive git@github.com:micahpearlman/MonkSVG.git
cd MonkSVG

# Get the latest from the submodules
git submodule update --recursive --remote

# Build
mkdir build \
    && cd build \
    && cmake .. \
    &&  cmake --build . 

```

## Use

Initialize and load an SVG:

```
    // Initialize MonkVG using GLES 2.0 rendering
    vgCreateContextMNK(WINDOW_WIDTH, WINDOW_HEIGHT,
                        VG_RENDERING_BACKEND_TYPE_OPENGLES20);


    // create an OpenVG (MonkVG) handler	
    MonkSVG::ISVGHandler::SmartPtr svg_handler =  MonkSVG::OpenVG_SVGHandler::create();

    // load an example
    std::string svgFilePath = "./data/tiger.svg";
    std::fstream is( svgFilePath.c_str(), std::fstream::in );
    std::stringstream ss;
    ss << is.rdbuf();

    // run it through the svg parser
    MonkSVG::SVG_Parser* svg_parser = MonkSVG::SVG_Parser::create(svg_handler);
    svg_parser->parse(ss.str());

```

Draw:

```
    // Clear the screen.
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // set OpenVG matrix
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
    vgLoadIdentity();

    // draw svg
    svg_handler->draw();

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();

```

Cleanup:

```
    // cleanup 
    svg_handler.reset();
    MonkSVG::SVG_Parser::destroy(svg_parser);

    // destroy MonkVG
    vgDestroyContextMNK();
```