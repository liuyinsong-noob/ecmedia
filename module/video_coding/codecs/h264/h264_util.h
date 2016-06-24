#ifndef WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_UTIL_H_
#define WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_UTIL_H_

typedef struct _SequenceParameterSet
{
public:
    const unsigned char * m_pStart;
    unsigned short m_nLength;
    int m_nCurrentBit;
    
	//pps data
	int profile_idc ;
	int constraint_set0_flag ;
	int constraint_set1_flag ;
	int constraint_set2_flag ;
	int constraint_set3_flag ;
	int constraint_set4_flag ;
	int constraint_set5_flag ;
	int reserved_zero_2bits  ;
	int level_idc ;
	int seq_parameter_set_id ;
	int chroma_format_idc ;
	int residual_colour_transform_flag ;
	int bit_depth_luma_minus8 ;
	int bit_depth_chroma_minus8 ;
	int qpprime_y_zero_transform_bypass_flag ;
	int seq_scaling_matrix_present_flag;
	int seq_scaling_list_present_flag ;
	int log2_max_frame_num_minus4 ;
	int pic_order_cnt_type ;
	int log2_max_pic_order_cnt_lsb_minus4 ;
	int delta_pic_order_always_zero_flag ;
	int offset_for_non_ref_pic ;
	int offset_for_top_to_bottom_field ;
	int num_ref_frames_in_pic_order_cnt_cycle ;
	int num_ref_frames ;
	int gaps_in_frame_num_value_allowed_flag;
	int pic_width_in_mbs_minus1 ;
	int pic_height_in_map_units_minus1 ;
	int frame_mbs_only_flag ;
	int mb_adaptive_frame_field_flag ;
	int direct_8x8_inference_flag ;
	int frame_cropping_flag ;
	int frame_crop_left_offset ;
	int frame_crop_right_offset ;
	int frame_crop_top_offset;
	int frame_crop_bottom_offset ;
	int vui_parameters_present_flag ;
    
    unsigned int ReadBit()
    {
        //ATLASSERT(m_nCurrentBit <= m_nLength * 8);
        int nIndex = m_nCurrentBit / 8;
        int nOffset = m_nCurrentBit % 8 + 1;
        
        m_nCurrentBit ++;
        return (m_pStart[nIndex] >> (8-nOffset)) & 0x01;
    }
    
    unsigned int ReadBits(int n)
    {
        int r = 0;
        int i;
        for (i = 0; i < n; i++)
        {
            r |= ( ReadBit() << ( n - i - 1 ) );
        }
        return r;
    }
    
    unsigned int ReadExponentialGolombCode()
    {
        int r = 0;
        int i = 0;
        
        while( (ReadBit() == 0) && (i < 32) )
        {
            i++;
        }
        r = ReadBits(i);
        r += (1 << i) - 1;
        return r;
    }
    
    
    unsigned int ReadSE()
    {
        int r = ReadExponentialGolombCode();
        if (r & 0x01)
        {
            r = (r+1)/2;
        }
        else
        {
            r = -(r/2);
        }
        return r;
    }
    
public:
    void Parse(const unsigned char * pStart, unsigned short nLen)
    {
        m_pStart = pStart;
        m_nLength = nLen;
        m_nCurrentBit = 0;
        
        profile_idc = ReadBits(8);
        constraint_set0_flag = ReadBit();
        constraint_set1_flag = ReadBit();
        constraint_set2_flag = ReadBit();
        constraint_set3_flag = ReadBit();
        constraint_set4_flag = ReadBit();
        constraint_set5_flag = ReadBit();
        reserved_zero_2bits  = ReadBits(2);
        level_idc = ReadBits(8);
        seq_parameter_set_id = ReadExponentialGolombCode();
        
        if( profile_idc == 100 || profile_idc == 110 ||
           profile_idc == 122 || profile_idc == 144 )
        {
            chroma_format_idc = ReadExponentialGolombCode();
            if( chroma_format_idc == 3 )
            {
                residual_colour_transform_flag = ReadBit();
            }
            bit_depth_luma_minus8 = ReadExponentialGolombCode();
            bit_depth_chroma_minus8 = ReadExponentialGolombCode();
            qpprime_y_zero_transform_bypass_flag = ReadBit();
            seq_scaling_matrix_present_flag = ReadBit();
            if( seq_scaling_matrix_present_flag )
            {
                for( int i = 0; i < 8; i++ )
                {
                    seq_scaling_list_present_flag = ReadBit();
                    if( seq_scaling_list_present_flag )
                    {
                        /*
                         if( i < 6 )
                         {
                         read_scaling_list( b, sps->ScalingList4x4[ i ], 16,
                         sps->UseDefaultScalingMatrix4x4Flag[ i ]);
                         }
                         else
                         {
                         read_scaling_list( b, sps->ScalingList8x8[ i - 6 ], 64,
                         sps->UseDefaultScalingMatrix8x8Flag[ i - 6 ] );
                         }
                         */
                    }
                }
            }
        }
        log2_max_frame_num_minus4 = ReadExponentialGolombCode();
        pic_order_cnt_type = ReadExponentialGolombCode();
        if( pic_order_cnt_type == 0 )
        {
            log2_max_pic_order_cnt_lsb_minus4 = ReadExponentialGolombCode();
        }
        else if( pic_order_cnt_type == 1 )
        {
            delta_pic_order_always_zero_flag = ReadBit();
            offset_for_non_ref_pic = ReadSE();
            offset_for_top_to_bottom_field = ReadSE();
            num_ref_frames_in_pic_order_cnt_cycle = ReadExponentialGolombCode();
            for( int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++ )
            {
                ReadSE();
                //sps->offset_for_ref_frame[ i ] = ReadSE();
            }
        }
        num_ref_frames = ReadExponentialGolombCode();
        gaps_in_frame_num_value_allowed_flag = ReadBit();
        pic_width_in_mbs_minus1 = ReadExponentialGolombCode();
        pic_height_in_map_units_minus1 = ReadExponentialGolombCode();
        frame_mbs_only_flag = ReadBit();
        if( !frame_mbs_only_flag )
        {
            mb_adaptive_frame_field_flag = ReadBit();
        }
        direct_8x8_inference_flag = ReadBit();
        frame_cropping_flag = ReadBit();
        if( frame_cropping_flag )
        {
            frame_crop_left_offset = ReadExponentialGolombCode();
            frame_crop_right_offset = ReadExponentialGolombCode();
            frame_crop_top_offset = ReadExponentialGolombCode();
            frame_crop_bottom_offset = ReadExponentialGolombCode();
        }
        vui_parameters_present_flag = ReadBit();
        
        pStart++;
    }
}SequenceParameterSet, *LPSequenceParameterSet;

#endif  //WEBRTC_MODULES_VIDEO_CODING_CODECS_H264_H_
