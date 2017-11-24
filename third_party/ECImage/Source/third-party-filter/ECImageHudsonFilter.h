//
//  ECImageHudsonFilter.h
//  FWMeituApp
//
//  Created by hzkmn on 16/1/11.
//  Copyright © 2016年 ForrestWoo co,.ltd. All rights reserved.
//

#import "ECImageFilterGroup.h"

@interface FWFilter5 : ECImageFourInputFilter

@end

@interface ECImageHudsonFilter : ECImageFilterGroup
{
    ECImagePicture *imageSource1;
    ECImagePicture *imageSource2;
    ECImagePicture *imageSource3;
}

@end
