//
//  MonkSVG_Test_iosAppDelegate.h
//  MonkSVG-Test-ios
//
//  Created by Micah Pearlman on 8/2/10.
//  Copyright Zero Vision 2010. All rights reserved.
//

#import <UIKit/UIKit.h>

@class MainViewController;

@interface MonkSVG_Test_iosAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    MainViewController *mainViewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet MainViewController *mainViewController;

@end

