//
//  ECImageAmaroFilter.h
//  FWMeituApp
//
//  Created by hzkmn on 16/1/11.
//  Copyright © 2016年 ForrestWoo co,.ltd. All rights reserved.
//

#import "ECImageFourInputFilter.h"
#import "ECImageFilterGroup.h"
#import "ECImagePicture.h"

@interface FWFilter3 : ECImageFourInputFilter

@end

@interface ECImageAmaroFilter : ECImageFilterGroup
{
    ECImagePicture *imageSource1;
    ECImagePicture *imageSource2;
    ECImagePicture *imageSource3;
}

@end
