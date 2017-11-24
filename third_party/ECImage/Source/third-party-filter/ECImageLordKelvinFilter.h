//
//  ECImageLordKelvinFilter.h
//  FWMeituApp
//
//  Created by hzkmn on 16/1/8.
//  Copyright © 2016年 ForrestWoo co,.ltd. All rights reserved.
//

#import "ECImageFilterGroup.h"

@interface FWFilter2 : ECImageTwoInputFilter

@end

@interface ECImageLordKelvinFilter : ECImageFilterGroup
{
    ECImagePicture *imageSource;
}

@end
