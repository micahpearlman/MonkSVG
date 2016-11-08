/*
 BSD 3-Clause License - Please see LICENSE file for full license
 */

#ifndef vgCompat_h
#define vgCompat_h

namespace MonkVG
{
    enum VGPaintType
    {
        VG_PAINT_TYPE_COLOR,
        VG_PAINT_TYPE_LINEAR_GRADIENT,
        VG_PAINT_TYPE_RADIAL_GRADIENT,
        VG_PAINT_TYPE_PATTERN
    };
    
    enum VGColorRampSpreadMode
    {
        VG_COLOR_RAMP_SPREAD_PAD,
        VG_COLOR_RAMP_SPREAD_REPEAT,
        VG_COLOR_RAMP_SPREAD_REFLECT
    };
    
    enum VGTilingMode
    {
        VG_TILE_FILL,
        VG_TILE_PAD,
        VG_TILE_REPEAT,
        VG_TILE_REFLECT
    };
    
    enum VGCapStyle
    {
        VG_CAP_BUTT,
        VG_CAP_ROUND,
        VG_CAP_SQUARE
    };
    
    enum VGJoinStyle
    {
        VG_JOIN_MITER,
        VG_JOIN_ROUND,
        VG_JOIN_BEVEL,
        VG_JOIN_FAKE_ROUND,
        VG_JOIN_FLIP_BEVEL
        
    };
    
    enum VGFillRule
    {
        VG_EVEN_ODD,
        VG_NON_ZERO
    };
    
    
    enum VGPathAbsRel
    {
        VG_ABSOLUTE                                 = 0,
        VG_RELATIVE                                 = 1
    };
    
    enum VGPathSegment
    {
        VG_CLOSE_PATH                               = ( 0 << 1),
        VG_MOVE_TO                                  = ( 1 << 1),
        VG_LINE_TO                                  = ( 2 << 1),
        VG_HLINE_TO                                 = ( 3 << 1),
        VG_VLINE_TO                                 = ( 4 << 1),
        VG_QUAD_TO                                  = ( 5 << 1),
        VG_CUBIC_TO                                 = ( 6 << 1),
        VG_SQUAD_TO                                 = ( 7 << 1),
        VG_SCUBIC_TO                                = ( 8 << 1),
        VG_SCCWARC_TO                               = ( 9 << 1),
        VG_SCWARC_TO                                = (10 << 1),
        VG_LCCWARC_TO                               = (11 << 1),
        VG_LCWARC_TO                                = (12 << 1)
    };
    
    enum VGPaintMode
    {
        VG_STROKE_PATH                              = (1 << 0),
        VG_FILL_PATH                                = (1 << 1)
    };
}

#endif /* vgCompat_h */
