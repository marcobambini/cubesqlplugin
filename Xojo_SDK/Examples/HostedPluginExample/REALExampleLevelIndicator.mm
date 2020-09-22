//
//  REALExampleLevelIndicator.mm
//  HostedPluginExample
//
//  Copyright (c) 2013 Xojo Inc. All rights reserved.
//

#import "REALExampleLevelIndicator.h"

@implementation REALExampleLevelIndicator
@synthesize controlInstance = _controlInstance;

REAL_VIEW_MIXINS(_controlInstance);
REAL_DND_MIXINS(_controlInstance, REALExampleLevelIndicator);

@end
