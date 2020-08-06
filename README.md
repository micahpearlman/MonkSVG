SakaSVG -- Speed-optimized SVG rendering for iOS OpenGLES 2.x+
==============================================================

__This branch is under heavy modification and is not safe to be used in any project yet (not even mine!)__

## About

SakaSVG is a rendering system allowing to read SVG files, parse them and push them into OpenGLES 2.0 compatible <b>V</b>ertex <b>A</b>rray <b>O</b>bjects, <b>V</b>ertex <b>B</b>uffer <b>O</b>bjects and <b>E</b>lement <b>B</b>uffer <b>O</b>bjects. The goal is not to provide a complete SVG rendering system, but to provide as many features as I can while keeping to a single rendering instruction.

### Project scope limitation examples

These are true, unless someone or a company actually wishes to champion these changes.

- There probably will never be font support unless they are converted to outlines and pushed back to the same VBO.
- Adding textures would need to create on-the-spot atlases so we can draw them in one pass.
- Although SVG supports external files, this system's goal is to be quick and efficient. Getting a second file beats that purpose.
- Some very complex edge cases might not render properly. The goal is not to provide rendering for everything on the market including the dreaded polygon edge cases, but to provide adequate rendering for objects made specifically for a project (e.g. a game). If something doesn't render adequately, it's expected the source can be modified to suit the renderer's limitations.

### Final goals

- To be able to draw simple SVG from files in less than 1/60th of a second.
- To be able to reduce CPU and GPU load on the device
- To have a way to save the final VAO, VBO & EBO into a pre-rendered file, improving subsequent load-times.


## What's new

- 2019-01-12 Now with a radically improved speed for tesselation. Getting there for final refactoring before release.
- 2016-10-29 Branching to SakaSVG as it's now a total different beast!
- 2016-10-13 iOS only, uses latest tool versions. Cleaned up potential license conflict.
- 2015-07-21 Now supports embedded SVG CSS style sheets.


## Project TODO (in priority order)

- Refactoring drawing operations to support slow with b-trees, quick points, single frame, static animation and fully dynamic animation
- Finalize refactoring to remove now strange hierarchies and old Monk* hierarchies
- Cleaning and dusting code
- Make sure it works with Metal (as GLES is now obsolete)
- Add SVG animations. That includes adding a quick and efficient JS engine.
- Add gradients to tesselation
- Add polygon intersection (occlusion & culling) before tesselation
- Add multipass optimizations for software that wants to do something ASAP, and then refine the result
- Scissoring and masking
- Finalize miter code
- Pattern fills and strokes
- Various blending modes
- Add unit testing
- Making sure we are supporting something else than iOS natively


## Project fork & major contributors

### MonkSVG and MonkVG original projects

__This version is a major fork from the original MonkSVG and MonkVG project by Micah Pearlman. Please go to https://github.com/micahpearlman/MonkSVG and https://github.com/micahpearlman/MonkVG for the original projects. Original code is still Copyright (C) Micah Pearlman and contributors. Attribution is being kept even if code is very heavily modified.__

Use git to clone the official original versions:
- Official MonkSVG: <tt>$ git clone https://github.com/micahpearlman/MonkSVG.git</tt>
- Official MonkVG: <tt>$ git clone https://github.com/micahpearlman/MonkVG.git</tt>

#### MonkSVG Additional contributors

- Embedded SVG CSS Parsing: Added by Sean Batson https://github.com/adozenlines

#### MonkVG Additional contributors

- Smooth line, Miter, Cap: Sean Batson

#### Removed from SakaSVG. Was there initially in MonkVG/MonkSVG

- Initial Android Port: Paul Holden
- Windows Port: Vincent Richomme
- Android and Linux Port: Gav Wood
- math.hpp, vec.hpp: Mapbox https://github.com/mapbox/mapbox-gl-native
- OpenVG VGU extension: Khronos Group

### SakaSVG's third parties

SakaSVG uses the following 3rd party libraries:
- __cpp_btree__ https://github.com/sakamura/cpp-btree (Apache 2.0 License)
- __glm__ https://github.com/g-truc/glm (Happy Bunny License / MIT License)
- __libTess3__ https://github.com/sakamura/libtess3 (SGI Free Software License B Version 2.0)
- __StyleSheet__ https://github.com/sakamura/StyleSheet (Clean room implementation using the MIT License)
- __TinyXML2__ https://github.com/sakamura/tinyxml2 (ZLib License)

__Please inform yourself on the different third parties licensing before using SakaSVG! Please abide by their rules!__

## Warning & Caveat Emptor

__This branch is under heavy modification and is not safe to be used in any project yet (not even mine!)__. It's much preferable to use the working original MonkSVG and MonkVG projects. However, you can also get many merges, pulls, optimizations, adaptations and fixes, and cherry-pick my changes at : https://github.com/sakamura/SakaSVG/tree/monksvg-master where the original MonkSVG base is being kept as easier backport. My final official branch for MonkVG is kept here: https://github.com/sakamura/MonkVG. Even there, these branches and forks are heavily modified so YMMV and bugs were introduced. It can serve as a great "cleaner" modern starting point for further development, such as the one I'm doing with SakaSVG. If Micah eventually wishes to pull back the changes to his original fork and revive his project, I'll be happy to help spread the love.

## Other OpenVG and SVG related projects

- https://github.com/SVGKit/SVGKit Display and interact with SVG Images on iOS / OS X, using native rendering (CoreAnimation) (currently only supported for iOS - OS X code needs updating)
