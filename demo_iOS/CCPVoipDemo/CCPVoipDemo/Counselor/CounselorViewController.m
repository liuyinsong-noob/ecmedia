/*
 *  Copyright (c) 2013 The CCP project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a Beijing Speedtong Information Technology Co.,Ltd license
 *  that can be found in the LICENSE file in the root of the web site.
 *
 *                    http://www.yuntongxun.com
 *
 *  An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#import "counselorViewController.h"
#import "MMGridViewDefaultCell.h"
#import "AppDelegate.h"

#import "counselorListViewController.h"

@interface counselorViewController()
- (void)reload;
- (void)setupPageControl;
@end

@implementation counselorViewController

// ----------------------------------------------------------------------------------

#pragma - Object lifecycle

- (void)dealloc
{
    self.imageScrollView = nil;
    [pageControl release];
    self.counselorArr = nil;
    [super dealloc];
}


-(void)goSubView:(int) index;
{
    NSDictionary * dict = [self.counselorArr objectAtIndex:index];
    NSString *strid = [dict objectForKey:@"id"];
    counselorListViewController* view = [[counselorListViewController alloc] initWithTypeID:strid];
    view.NameStr = [dict objectForKey:@"name"];
    [self.navigationController pushViewController:view animated:YES];
    [view release];
}

-(void)back
{
    [self.navigationController popViewControllerAnimated:YES];
}

- (void)gotoSettingView
{
    
}

- (void)viewWillAppear:(BOOL)animated{
    [super viewWillAppear:animated];
    self.modelEngineVoip.UIDelegate = self;
}
- (void)viewDidLoad
{
    [super viewDidLoad];
    self.modelEngineVoip.UIDelegate = self;
    [self displayProgressingView];
    [self.modelEngineVoip getServiceNum];
    [self.modelEngineVoip getCategoryList];
    
    self.title = @"专家咨询";
 
    UIBarButtonItem *btnBack = [[UIBarButtonItem alloc] initWithCustomView:[CommonTools navigationBackItemBtnInitWithTarget:self action:@selector(back)]];
    self.navigationItem.leftBarButtonItem = btnBack;
    [btnBack release];
    
    /*
    UIBarButtonItem *setBtn=[[UIBarButtonItem alloc] initWithCustomView:[CommonTools navigationItemBtnInitWithNormalImageNamed:@"title_bar_back_setting.png" andHighlightedImageNamed:@"title_bar_back_setting_on.png" target:self action:@selector(gotoSettingView)]];
    self.navigationItem.rightBarButtonItem = setBtn;
    [setBtn release];
    */
    
    UIScrollView *scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, 0, 320, 120)];
    self.imageScrollView = scrollView;
    [scrollView release];
    
    self.imageScrollView.contentSize = CGSizeMake(320, 120);
    self.imageScrollView.showsVerticalScrollIndicator = NO;
    self.imageScrollView.showsHorizontalScrollIndicator = NO;
    self.imageScrollView.pagingEnabled = YES;
    self.imageScrollView.bounces = NO;
    self.imageScrollView.scrollsToTop = NO;
    self.imageScrollView.delegate = self;
    self.imageScrollView.backgroundColor = [UIColor colorWithRed:244./255 green:244./255 blue:244./255 alpha:1.0];
    
    UIImageView* iv1 = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 320, 120)];
    iv1.image = [UIImage imageNamed:@"main_img.png"];
    [self.imageScrollView addSubview:iv1];
    [iv1 release];
    
    [self.view addSubview :self.imageScrollView];

    // setup MMGrid view
    gridView = [[MMGridView alloc] initWithFrame:CGRectMake(0, 120, 320, [UIScreen mainScreen].applicationFrame.size.height-([UIDevice currentDevice].systemVersion.floatValue>6?100.0f:12.0f))];
    gridView.showLine = YES;
    gridView.dataSource = self;
    gridView.delegate = self;
    gridView.cellMargin = 0;
    gridView.numberOfRows = gridView.frame.size.height/110.0f;
    gridView.numberOfColumns = 3;
    gridView.layoutStyle = VerticalLayout;
    gridView.backgroundColor = [UIColor colorWithRed:244./255 green:244./255 blue:244./255 alpha:1.0];
    [self.view addSubview:gridView];
    [gridView release];
    self.view.backgroundColor = [UIColor colorWithRed:244./255 green:244./255 blue:244./255 alpha:1.0];
    
    pageControl = [[UIPageControl alloc] initWithFrame:CGRectMake(0, 112, 320, 10)];
    pageControl.numberOfPages = 1;
    pageControl.hidden = YES;
    [self.view addSubview:pageControl];
    // setup the page control
    [self setupPageControl];
}


- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait ||
            interfaceOrientation == UIInterfaceOrientationLandscapeLeft ||
            interfaceOrientation == UIInterfaceOrientationLandscapeRight);
}


- (void)reload
{
    [gridView reloadData];
}

//
- (void)setupPageControl
{
    pageControl.numberOfPages = 3;
    pageControl.currentPage = 0;
}

// ----------------------------------------------------------------------------------

#pragma - MMGridViewDataSource

- (NSInteger)numberOfCellsInGridView:(MMGridView *)gridView
{
    return self.counselorArr.count;
}


- (MMGridViewCell *)gridView:(MMGridView *)gridView cellAtIndex:(NSUInteger)index
{
    MMGridViewDefaultCell *cell = [[[MMGridViewDefaultCell alloc] initWithFrame:CGRectNull] autorelease];
    NSMutableDictionary* dict = [self.counselorArr objectAtIndex:index];
    cell.textLabel.frame = CGRectMake(10, 80, 83, 20 );
    cell.textLabel.text = [dict objectForKey:@"name"];
    cell.backgroundView.frame = CGRectMake(25, 25, 53, 53);
    cell.backgroundView.image = [UIImage imageNamed:[dict objectForKey:@"imgPath"]];
    return cell;
}

// ----------------------------------------------------------------------------------

#pragma - MMGridViewDelegate

- (void)gridView:(MMGridView *)gridView didSelectCell:(MMGridViewCell *)cell atIndex:(NSUInteger)index
{
    //判断网络
//    if([self getNetworkFlag])
        [self goSubView:index];
}


- (void)gridView:(MMGridView *)gridView didDoubleTapCell:(MMGridViewCell *)cell atIndex:(NSUInteger)index
{
//    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:nil
//                                                    message:[NSString stringWithFormat:@"Cell at index %d was double tapped.", index]
//                                                   delegate:nil
//                                          cancelButtonTitle:@"Cool!"
//                                          otherButtonTitles:nil];
//    [alert show];
//    [alert release];
}


- (void)gridView:(MMGridView *)theGridView changedPageToIndex:(NSUInteger)index
{
    //[self setupPageControl];
}
- (void)updateCurrentPageIndex
{
    CGFloat pageWidth = self.imageScrollView.frame.size.width;
    NSUInteger cpi = floor((self.imageScrollView.contentOffset.x - pageWidth / 2) / pageWidth) + 1;
    pageControl.currentPage = cpi;
}

// ----------------------------------------------------------------------------------

#pragma - UIScrollViewDelegate

- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView
{
    [self updateCurrentPageIndex];
}
-(BOOL)canLoadMoreForGrid
{
    return NO;
}


- (void)scrollViewDidEndScrollingAnimation:(UIScrollView *)scrollView
{
    [self updateCurrentPageIndex];
}

- (void) onGetServiceNumWithReason:(CloopenReason*)reason
{
    NSLog(@"onGetServiceNumWithReason:%d",reason.reason);
}

- (void) onGetCategoryListWithReason:(CloopenReason*)reason andCategorys:(NSMutableArray *)categoryArray
{
    [self dismissProgressingView];
    if (reason.reason == 0)
    {
        self.counselorArr = categoryArray;
        [self reload];
    }
    else
    {
        [self popPromptViewWithMsg:[NSString stringWithFormat:@"获取列表失败。错误码：%d,错误详情：%@",reason.reason,reason.msg]];
    }
}
@end
