//
//  ECImageHistogramEqualizationFilter.h
//  FilterShowcase
//
//  Created by Adam Marcus on 19/08/2014.
//  Copyright (c) 2014 Sunset Lake Software LLC. All rights reserved.
//

#import "ECImageFilterGroup.h"
#import "ECImageHistogramFilter.h"
#import "ECImageRawDataOutput.h"
#import "ECImageRawDataInput.h"
#import "ECImageTwoInputFilter.h"

@interface ECImageHistogramEqualizationFilter : ECImageFilterGroup
{
    ECImageHistogramFilter *histogramFilter;
    ECImageRawDataOutput *rawDataOutputFilter;
    ECImageRawDataInput *rawDataInputFilter;
}

@property(readwrite, nonatomic) NSUInteger downsamplingFactor;

- (id)initWithHistogramType:(ECImageHistogramType)newHistogramType;

@end
