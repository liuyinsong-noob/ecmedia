//
//  ECImageInkwellFilter.h
//  FWMeituApp
//
//  Created by hzkmn on 16/1/11.
//  Copyright © 2016年 ForrestWoo co,.ltd. All rights reserved.
//

#import "ECImageFilterGroup.h"

@interface FWFilter10 : ECImageTwoInputFilter

@end

@interface ECImageInkwellFilter : ECImageFilterGroup
{
    ECImagePicture *imageSource;
}

@end
