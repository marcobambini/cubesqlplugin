//
//  REALExampleLevelIndicator.h
//  HostedPluginExample
//
//  Copyright (c) 2013 Xojo Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "rb_plugin_cocoa.h"

@interface REALExampleLevelIndicator : NSLevelIndicator {
	REALcontrolInstance _controlInstance;
}
@property(assign) REALcontrolInstance controlInstance;
@end
