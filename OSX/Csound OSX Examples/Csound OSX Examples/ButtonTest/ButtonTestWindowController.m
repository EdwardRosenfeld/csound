/*
 
 ButtonTestWindowController.m:
 
 Copyright (C) 2014 Aurelius Prochazka
 
 This file is part of Csound OSX Examples.
 
 The Csound for iOS Library is free software; you can redistribute it
 and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 Csound is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with Csound; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 
 */

#import "ButtonTestWindowController.h"
#import "CsoundObj.h"
#import "CsoundUI.h"

@interface ButtonTestWindowController () <CsoundObjCompletionListener> {
    CsoundObj* csound;
}
@property (strong) IBOutlet NSButton *startStopButton;
@property (strong) IBOutlet NSButton *valueButton;
@property (strong) IBOutlet NSSlider *durationSlider;
@property (strong) IBOutlet NSSlider *attackSlider;
@property (strong) IBOutlet NSSlider *decaySlider;
@property (strong) IBOutlet NSSlider *sustainSlider;
@property (strong) IBOutlet NSSlider *releaseSlider;

@end

@implementation ButtonTestWindowController

-(IBAction) eventButtonHit:(id)sender {
    NSString *score = [NSString stringWithFormat:@"i2 0 %f", [_durationSlider floatValue]];
    [csound sendScore:score];
}

-(IBAction) toggleOnOff:(id)component {
    
	if(([self.startStopButton.title isEqualToString:@"Start"]) ) {
        
        NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"buttonTest" ofType:@"csd"];
        NSLog(@"FILE PATH: %@", tempFile);
        
		[csound stopCsound];
        
        csound = [[CsoundObj alloc] init];
        [csound addCompletionListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:csound];
        
        [csoundUI addSlider:_durationSlider forChannelName:@"duration"];
        [csoundUI addSlider:_attackSlider forChannelName:@"attack"];
        [csoundUI addSlider:_decaySlider forChannelName:@"decay"];
        [csoundUI addSlider:_sustainSlider forChannelName:@"sustain"];
        [csoundUI addSlider:_releaseSlider forChannelName:@"release"];
        
        [csound startCsound:tempFile];
        
	} else {
        [csound stopCsound];
    }
}



#pragma mark CsoundObjCompletionListener

-(void)csoundObjDidStart:(CsoundObj *)csoundObj {
    self.startStopButton.title = @"Stop";
}

-(void)csoundObjComplete:(CsoundObj *)csoundObj {
	self.startStopButton.title = @"Start";
}


@end
