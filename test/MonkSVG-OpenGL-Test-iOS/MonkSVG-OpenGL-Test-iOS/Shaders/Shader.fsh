//
//  Shader.fsh
//  MonkSVG-OpenGL-Test-iOS
//
//  Created by Micah Pearlman on 6/22/11.
//  Copyright 2011 Zero Vision. All rights reserved.
//

varying lowp vec4 colorVarying;

void main()
{
    gl_FragColor = colorVarying;
}
