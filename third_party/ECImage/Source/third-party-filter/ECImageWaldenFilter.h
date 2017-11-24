//
//  ECImageWaldenFilter.h
//  FWMeituApp
//
//  Created by hzkmn on 16/1/11.
//  Copyright © 2016年 ForrestWoo co,.ltd. All rights reserved.
//

#import "ECImageFilterGroup.h"
#import "ECImageThreeInputFilter.h"
#import "ECImagePicture.h"

@interface FWFilter7 : ECImageThreeInputFilter

@end

@interface ECImageWaldenFilter : ECImageFilterGroup
{
    ECImagePicture *imageSource1;
    ECImagePicture *imageSource2;
}

@end
