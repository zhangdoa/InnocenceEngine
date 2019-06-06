//
//  macMain.mm
//  macMain
//
//  Created by zhangdoa on 13/04/2019.
//  Copyright © 2019 zhangdoa. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import "AppDelegate.h"

int main(int argc, const char * argv[]) {
    [NSApplication sharedApplication];
    AppDelegate* appDelegate = [[AppDelegate alloc] init];
    [NSApp setDelegate:appDelegate];
    [NSApp run];
    return 0;
}
