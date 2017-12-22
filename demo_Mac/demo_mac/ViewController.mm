//
//  ViewController.m
//  demo_mac
//
//  Created by 葛昭友 on 2017/12/22.
//  Copyright © 2017年 ronglianyun. All rights reserved.
//

#import "ViewController.h"
#include "ECMedia.h"

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // Do any additional setup after loading the view.
}


- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
    ECMedia_get_Version();
}


@end
