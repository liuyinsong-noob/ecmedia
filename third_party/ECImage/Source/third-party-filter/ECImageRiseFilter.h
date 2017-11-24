//
//  ECImageRiseFilter.h
//  FWMeituApp
//
//  Created by hzkmn on 16/1/11.
//  Copyright © 2016年 ForrestWoo co,.ltd. All rights reserved.
//

#import "ECImageFilterGroup.h"

@interface FWFilter4 : ECImageFourInputFilter

@end

@interface ECImageRiseFilter : ECImageFilterGroup
{
    ECImagePicture *imageSource1;
    ECImagePicture *imageSource2;
    ECImagePicture *imageSource3;
}

@end
