/*
 *  mkSVG.h
 *  MonkSVG
 *
 *  Created by Micah Pearlman on 8/2/10.
 *  Copyright 2010 Zero Vision. All rights reserved.
 *
 */

#ifndef __mkSVG_h__
#define __mkSVG_h__

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <memory>
#include <fstream>

// class TiXmlDocument;
// class TiXmlElement;

namespace MonkSVG {

class SVG_Parser_Implementation;

/**
 * @brief Interface for handling SVG elements.
 * See: openvg/mkOpenVG_SVG.h/cpp for an example implementation
 *
 */
class ISVGHandler {
  public:
    typedef std::shared_ptr<ISVGHandler> SmartPtr;

    // transforms
    virtual void onTransformTranslate(float x, float y) = 0;
    virtual void onTransformScale(float s) = 0;
    virtual void onTransformRotate(float r) = 0;
    virtual void onTransformMatrix(float a, float b, float c, float d, float e,
                                   float f) = 0;

    // groups
    virtual void onGroupBegin() = 0;
    virtual void onGroupEnd() = 0;
    virtual void onUseBegin() = 0;
    virtual void onUseEnd() = 0;

    virtual void onId(const std::string &id_) = 0;

    // paths
    virtual void onPathBegin() = 0;
    virtual void onPathEnd() = 0;
    virtual void onPathMoveTo(float x, float y) = 0;
    virtual void onPathClose() = 0;
    virtual void onPathLineTo(float x, float y) = 0;
    virtual void onPathCubic(float x1, float y1, float x2, float y2, float x3,
                             float y3) = 0;
    virtual void onPathSCubic(float x2, float y2, float x3, float y3) = 0;
    virtual void onPathArc(float rx, float ry, float x_axis_rotation,
                           int large_arc_flag, int sweep_flag, float x,
                           float y) = 0;
    virtual void onPathRect(float x, float y, float w, float h) = 0;
    virtual void onPathHorizontalLine(float x) = 0;
    virtual void onPathVerticalLine(float y) = 0;

    virtual void onPathQuad(float x1, float y1, float x2, float y2) = 0;

    // fill
    virtual void onPathFillColor(unsigned int color) = 0;
    virtual void onPathFillOpacity(float o) = 0;
    virtual void onPathFillRule(const std::string &rule) = 0;

    // stroke
    virtual void onPathStrokeColor(unsigned int color) = 0;
    virtual void onPathStrokeOpacity(float o) = 0;
    virtual void onPathStrokeWidth(float width) = 0;

    void setRelative(bool r) { _relative = r; }
    bool relative() const { return _relative; }

    // bounds
    float minX() { return _minX; }
    float minY() { return _minY; }
    float width() { return _width; }
    float height() { return _height; }

    // drawing
    virtual void draw() = 0;
    virtual void dump(void **vertices, size_t *size) = 0;
    virtual void optimize() = 0;

  protected:
    ISVGHandler()
        : _minX(MAXFLOAT), _minY(MAXFLOAT), _width(-MAXFLOAT),
          _height(-MAXFLOAT) {}
    // bounds info
    float _minX;
    float _minY;
    float _width;
    float _height;

    friend class SVG_Parser_Implementation;

  private:
    bool _relative;
};

/**
 * @brief SVG Xml Parser
 *
 */
class SVG_Parser {
  public:
    static SVG_Parser *create(ISVGHandler::SmartPtr handler);
    static void        destroy(SVG_Parser *svg_parser);

    virtual bool parse(const std::string &data) = 0;
    virtual bool parse(const char *data) = 0;

  protected:
    SVG_Parser() {}
};
} // namespace MonkSVG

#endif // __mkSVG_h__