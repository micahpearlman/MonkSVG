//
//  MonkSVG_OpenGL_Test_iOSViewController.m
//  MonkSVG-OpenGL-Test-iOS
//
//  Created by Micah Pearlman on 6/22/11.
//  Copyright 2011 Zero Vision. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>

#import "MonkSVG_OpenGL_Test_iOSViewController.h"
#import "EAGLView.h"
#import "ARCMacro.h"

/// std
#include <string>
#include <iostream>
#include <fstream>

/// openvg
#include <MonkVG/openvg.h>
#include <MonkVG/vgu.h>

/// svg
#include <mkSVG.h>
#include <openvg/mkOpenVG_SVG.h>
static MonkSVG::OpenVG_SVGHandler::SmartPtr vgSVGRenderer;

//#define USE_OPENGLES_11
#define USE_OPENGLES_20

// Uniform index.
enum {
    UNIFORM_TRANSLATE,
    NUM_UNIFORMS
};
GLint uniforms[NUM_UNIFORMS];

// Attribute index.
enum {
    ATTRIB_VERTEX,
    ATTRIB_COLOR,
    NUM_ATTRIBUTES
};

@interface MonkSVG_OpenGL_Test_iOSViewController ()
@property (nonatomic, retain) EAGLContext *context;
@property (nonatomic, assign) CADisplayLink *displayLink;
- (BOOL)loadShaders;
- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file;
- (BOOL)linkProgram:(GLuint)prog;
- (BOOL)validateProgram:(GLuint)prog;
@end

@implementation MonkSVG_OpenGL_Test_iOSViewController

@synthesize animating, context, displayLink;

- (void)awakeFromNib
{
#if defined(USE_OPENGLES_11)
    EAGLContext *aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
#elif defined(USE_OPENGLES_20)
    EAGLContext *aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
#endif
    if (!aContext) {
        aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
    }
    
    if (!aContext)
        NSLog(@"Failed to create ES context");
    else if (![EAGLContext setCurrentContext:aContext])
        NSLog(@"Failed to set ES context current");
    
	self.context = aContext;
	[aContext RELEASE];
	
    [(EAGLView *)self.view setContext:context];
    [(EAGLView *)self.view setFramebuffer];
    
    if ([context API] == kEAGLRenderingAPIOpenGLES2)
        [self loadShaders];
    
    animating = FALSE;
    animationFrameInterval = 1;
    self.displayLink = nil;
	
}

- (void)dealloc
{
    if (program) {
        glDeleteProgram(program);
        program = 0;
    }
    
    // Tear down context.
    if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
    
    [context RELEASE];
    
    [super DEALLOC];
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc. that aren't in use.
}

- (void)viewWillAppear:(BOOL)animated
{
    [self startAnimation];
    
    [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self stopAnimation];
    
    [super viewWillDisappear:animated];
}

- (void)viewDidLoad {
	// initialize MonkVG
    float width = self.view.bounds.size.width * self.view.contentScaleFactor;
    float height = self.view.bounds.size.height * self.view.contentScaleFactor;
#if defined(USE_OPENGLES_11)
	vgCreateContextMNK( width, height, VG_RENDERING_BACKEND_TYPE_OPENGLES11 );
#elif defined(USE_OPENGLES_20)
    vgCreateContextMNK( width, height, VG_RENDERING_BACKEND_TYPE_OPENGLES20 );
#endif
    
    [self addGestureRecognizers];
    
	// load an example
	
	MonkSVG::ISVGHandler::SmartPtr handler =  MonkSVG::OpenVG_SVGHandler::create();
	vgSVGRenderer = std::static_pointer_cast<MonkSVG::OpenVG_SVGHandler>( handler );
	std::string resourcePath( [[[NSBundle mainBundle] resourcePath] UTF8String] );
	std::string svgFilePath = resourcePath + "/tiger.svg";
	std::fstream is( svgFilePath.c_str(), std::fstream::in );
	
	//std::string buffer( (std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>() );
	
	// get length of file:
	is.seekg (0, ios::end);
	int length = is.tellg();
	is.seekg (0, ios::beg);
	
	// allocate memory:
	char* buffer = new char [length];
	
	// read data as a block:
	is.read (buffer,length);
	
	// run it through the svg parser
	MonkSVG::SVG svg_parser;
	svg_parser.initialize( vgSVGRenderer );
	svg_parser.read( buffer );
	
	// optimize for MonkVG
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();
	//vgSVGRenderer->optimize();

	delete [] buffer;
}

- (void)viewDidUnload
{
	[super viewDidUnload];
	
    if (program) {
        glDeleteProgram(program);
        program = 0;
    }

    // Tear down context.
    if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
	self.context = nil;	
}

- (NSInteger)animationFrameInterval
{
    return animationFrameInterval;
}

- (void)setAnimationFrameInterval:(NSInteger)frameInterval
{
    /*
	 Frame interval defines how many display frames must pass between each time the display link fires.
	 The display link will only fire 30 times a second when the frame internal is two on a display that refreshes 60 times a second. The default frame interval setting of one will fire 60 times a second when the display refreshes at 60 times a second. A frame interval setting of less than one results in undefined behavior.
	 */
    if (frameInterval >= 1) {
        animationFrameInterval = frameInterval;
        
        if (animating) {
            [self stopAnimation];
            [self startAnimation];
        }
    }
}

- (void)startAnimation
{
    if (!animating) {
        CADisplayLink *aDisplayLink = [[UIScreen mainScreen] displayLinkWithTarget:self selector:@selector(drawFrame)];
        [aDisplayLink setFrameInterval:animationFrameInterval];
        [aDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        self.displayLink = aDisplayLink;
        
        animating = TRUE;
    }
}

- (void)stopAnimation
{
    if (animating) {
        [self.displayLink invalidate];
        self.displayLink = nil;
        animating = FALSE;
    }
}

- (void)drawFrame
{
    EAGLView *eaglView = (EAGLView *)self.view;
    BOOL recreated = [eaglView setFramebuffer];
    
    if (recreated) {
        vgResizeSurfaceMNK(eaglView.framebufferWidth, eaglView.framebufferHeight);
    }
	
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
	VGfloat clearColor[] = {1,1,1,1};
	vgSetfv(VG_CLEAR_COLOR, 4, clearColor);
	
	vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
	vgLoadIdentity();
    vgScale(self.view.contentScaleFactor, -self.view.contentScaleFactor);
    vgTranslate(0, [(EAGLView *)self.view framebufferHeight]);
    
    VGfloat xf[9];
    vgGetMatrix(xf);
    vgScale(5.0f, 5.0f);
	vgTranslate(translation.x * self.view.contentScaleFactor,
                -translation.y * self.view.contentScaleFactor);
    vgSVGRenderer->draw();
    vgLoadMatrix(xf);
    
    float scale = 0.1f + 2 * fabs(sinf(CACurrentMediaTime()));
    vgScale(scale, scale);
    vgTranslate(300 + 200 * sinf(5 * CACurrentMediaTime()),
                200 * cosf(5 * CACurrentMediaTime()));
    
    vgSVGRenderer->draw();
	
    [(EAGLView *)self.view presentFramebuffer];
}

- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file
{
    GLint status;
    const GLchar *source;
    
    source = (GLchar *)[[NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil] UTF8String];
    if (!source)
    {
        NSLog(@"Failed to load vertex shader");
        return FALSE;
    }
    
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0)
    {
        glDeleteShader(*shader);
        return FALSE;
    }
    
    return TRUE;
}

- (BOOL)linkProgram:(GLuint)prog
{
    GLint status;
    
    glLinkProgram(prog);
    
#if defined(DEBUG)
    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program link log:\n%s", log);
        free(log);
    }
#endif
    
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0)
        return FALSE;
    
    return TRUE;
}

- (BOOL)validateProgram:(GLuint)prog
{
    GLint logLength, status;
    
    glValidateProgram(prog);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program validate log:\n%s", log);
        free(log);
    }
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == 0)
        return FALSE;
    
    return TRUE;
}

- (BOOL)loadShaders
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;
    
    // Create shader program.
    program = glCreateProgram();
    
    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"vsh"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname])
    {
        NSLog(@"Failed to compile vertex shader");
        return FALSE;
    }
    
    // Create and compile fragment shader.
    fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"fsh"];
    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER file:fragShaderPathname])
    {
        NSLog(@"Failed to compile fragment shader");
        return FALSE;
    }
    
    // Attach vertex shader to program.
    glAttachShader(program, vertShader);
    
    // Attach fragment shader to program.
    glAttachShader(program, fragShader);
    
    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation(program, ATTRIB_VERTEX, "position");
    glBindAttribLocation(program, ATTRIB_COLOR, "color");
    
    // Link program.
    if (![self linkProgram:program])
    {
        NSLog(@"Failed to link program: %d", program);
        
        if (vertShader)
        {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader)
        {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (program)
        {
            glDeleteProgram(program);
            program = 0;
        }
        
        return FALSE;
    }
    
    // Get uniform locations.
    uniforms[UNIFORM_TRANSLATE] = glGetUniformLocation(program, "translate");
    
    // Release vertex and fragment shaders.
    if (vertShader)
        glDeleteShader(vertShader);
    if (fragShader)
        glDeleteShader(fragShader);
    
    return TRUE;
}

#pragma mark - Gesture handlers

- (void) addGestureRecognizers {
    UIPanGestureRecognizer *oneFingerPan =
    [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(oneFingerPan:)];
    [self.view addGestureRecognizer:oneFingerPan];
    [oneFingerPan RELEASE];
}

- (void) oneFingerPan:(UIPanGestureRecognizer *) recognizer {
    CGPoint location = [recognizer translationInView:self.view];
    translation.x += location.x;
    translation.y += location.y;
    
    //Reset recognizer so change doesn't accumulate
    [recognizer setTranslation:CGPointZero inView:self.view];
}

@end
