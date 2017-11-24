//
//  ECImageLomofiFilter.h
//  FWMeituApp
//
//  Created by hzkmn on 16/1/11.
//  Copyright © 2016年 ForrestWoo co,.ltd. All rights reserved.
//

#import "ECImageFilterGroup.h"

@interface FWFilter6 : ECImageThreeInputFilter

@end

@interface ECImageLomofiFilter : ECImageFilterGroup
{
    ECImagePicture *imageSource1;
    ECImagePicture *imageSource2;
}

@end
