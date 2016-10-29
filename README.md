SakaSVG -- Speed-optimized SVG rendering for iOS OpenGLES 2.x+
==============================================================

__This branch is under heavy modification and is not safe to be used in any project yet (not even mine!)__

## About

SakaSVG is a rendering system allowing to read SVG files, parse them and push them into OpenGLES 2.0 compatible <b>V</b>ertex <b>A</b>rray <b>O</b>bjects, <b>V</b>ertex <b>B</b>uffer <b>O</b>bjects and <b>E</b>lement <b>B</b>uffer <b>O</b>bjects. The goal is not to provide a complete SVG rendering system, but to provide as many features as I can while keeping to a single rendering instruction.

### Project scope limitation examples

- There probably will never be font support unless they are converted to outlines and pushed back to the same VBO.
- Adding textures would need to create on-the-spot atlases so we can draw them in one pass.
- Although SVG supports external files, this system's goal is to be quick and efficient. Getting a second file beats that purpose.
- Some very complex edge cases might not render properly. The goal is not to provide rendering for everything on the market including the dreaded polygon edge cases, but to provide adequate rendering for objects made specifically for a project (e.g. a game). If something doesn't render adequately, it's expected the source can be modified to suit the renderer's limitations.

### Multipass system

- As designed, getting a SVG into a VAO, VBO and EBO is relatively quick to do. However, many optimizations take a lot of time and cannot be provided immediately. As such, there is a possibility to ask the system to do a two-pass optimization in a secondary thread in order to reduce the number of triangles to draw and improve the tesselation quality.

### Final goals

- To be able to draw simple SVG from files in less than 1/60th of a second.
- To be able to reduce CPU and GPU load on the device
- To have a way to save the final VAO, VBO & EBO into a pre-rendered file, improving subsequent load-times.


## What's new

- 2016-10-29 Branching to SakaSVG as it's now a total different beast!
- 2016-10-13 iOS only, uses latest tool versions. Cleaned up potential license conflict.
- 2015-07-21 Now supports embedded SVG CSS style sheets.


## Project TODO (in priority order)

- Merging SVG and VG to remove OpenVG code and reduce object complexities
- Add SVG animations
- Add gradients to tesselation
- Add polygon intersection before tesselation
- Finalize miter code


## Project fork & major contributors

### MonkSVG and MonkVG original projects

__This version is a major fork from the original MonkSVG and MonkVG project by Micah Pearlman. Please go to https://github.com/micahpearlman/MonkSVG and https://github.com/micahpearlman/MonkVG for the original projects. Original code is still Copyright (C) Micah Pearlman and contributors. Attribution is being kept even if code is very heavily modified.__

Use git to clone the official original versions:
- Official MonkSVG: <tt>$ git clone https://github.com/micahpearlman/MonkSVG.git</tt>
- Official MonkVG: <tt>$ git clone https://github.com/micahpearlman/MonkVG.git</tt>

### Embedded SVG CSS Parsing

Added by Sean Batson 
<tt>https://github.com/adozenlines</tt>


## Warning & Caveat Emptor

__This branch is under heavy modification and is not safe to be used in any project yet (not even mine!)__. It's much preferable to use the working original MonkSVG and MonkVG projects. However, you can also get many merges, pulls, optimizations, adaptations and fixes, and cherry-pick my changes at : https://github.com/sakamura/SakaSVG/tree/monksvg-master where the original MonkSVG base is being kept as easier backport. My final official branch for MonkVG is kept here: https://github.com/sakamura/MonkVG. Even there, these branches and forks are heavily modified so YMMV and bugs were introduced. It can serve as a great "cleaner" modern starting point for further development, such as the one I'm doing with SakaSVG. If Micah eventually wishes to pull back the changes to his original fork and revive his project, I'll be happy to help spread the love.
