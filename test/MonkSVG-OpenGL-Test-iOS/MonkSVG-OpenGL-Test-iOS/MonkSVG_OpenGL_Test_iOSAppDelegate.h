//
//  MonkSVG_OpenGL_Test_iOSAppDelegate.h
//  MonkSVG-OpenGL-Test-iOS
//
//  Created by Micah Pearlman on 6/22/11.
//  Copyright 2011 Zero Vision. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MonkSVG_OpenGL_Test_iOSViewController;

@interface MonkSVG_OpenGL_Test_iOSAppDelegate : NSObject <UIApplicationDelegate> {

}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@property (nonatomic, retain) IBOutlet MonkSVG_OpenGL_Test_iOSViewController *viewController;

@end
