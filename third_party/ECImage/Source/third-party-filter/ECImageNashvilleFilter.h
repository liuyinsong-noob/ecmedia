//
//  ECImageNashvilleFilter.h
//  FWMeituApp
//
//  Created by hzkmn on 16/1/8.
//  Copyright © 2016年 ForrestWoo co,.ltd. All rights reserved.
//

#import "ECImageFilterGroup.h"
#import "ECImageTwoInputFilter.h"
#import "ECImagePicture.h"

@interface FWFilter1 : ECImageTwoInputFilter



@end

@interface ECImageNashvilleFilter : ECImageFilterGroup
{
    ECImagePicture *imageSource ;
}

@end
