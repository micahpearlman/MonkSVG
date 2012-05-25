//
//  MonkSVG_OpenGL_Test_iOSViewController.h
//  MonkSVG-OpenGL-Test-iOS
//
//  Created by Micah Pearlman on 6/22/11.
//  Copyright 2011 Zero Vision. All rights reserved.
//

#import <UIKit/UIKit.h>

#import <OpenGLES/EAGL.h>

#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#include <openvg/mkOpenVG_SVG.h>

@interface MonkSVG_OpenGL_Test_iOSViewController : UIViewController {
@private
    EAGLContext *context;
    GLuint program;
    MonkSVG::OpenVG_SVGHandler *vgSVGRenderer;
    
    BOOL animating;
    NSInteger animationFrameInterval;
    CADisplayLink *displayLink;
}

@property (readonly, nonatomic, getter=isAnimating) BOOL animating;
@property (nonatomic) NSInteger animationFrameInterval;

- (void)startAnimation;
- (void)stopAnimation;

@end
