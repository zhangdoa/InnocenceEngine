//
//  macMain.mm
//  macMain
//
//  Created by zhangdoa on 13/04/2019.
//  Copyright © 2019 zhangdoa. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import "InnoNSApplication.h"

int main(int argc, const char * argv[]) {
    NSDictionary *infoDictionary = [[NSBundle mainBundle] infoDictionary];
    Class principalClass =
    NSClassFromString([infoDictionary objectForKey:@"NSPrincipalClass"]);
    NSApplication *applicationObject = [principalClass sharedApplication];
    
    if ([applicationObject respondsToSelector:@selector(run)])
    {
        [applicationObject
         performSelectorOnMainThread:@selector(run)
         withObject:nil
         waitUntilDone:YES];
    }
    
    return 0;
}
