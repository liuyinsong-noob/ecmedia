/*
 encoder.h

 Copyright (C) 2011 Belledonne Communications, Grenoble, France
 Author : Johan Pascal
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef ENCODER_H
#define ENCODER_H

typedef struct bcg729EncoderChannelContextStruct_struct bcg729EncoderChannelContextStruct;

#if !(defined(_MSC_VER) && (_MSC_VER < 1600))
#include <stdint.h>
#endif

#if defined(_MSC_VER)
	#define LIBSSH_API
#else  // __GNUC__
	#define LIBSSH_API __attribute__ ((visibility ("default"))) 
#endif

/*****************************************************************************/
/* initBcg729EncoderChannel : create context structure and initialise it     */
/*    return value :                                                         */
/*      - the encoder channel context data                                   */
/*                                                                           */
/*****************************************************************************/
LIBSSH_API int16_t initBcg729EncoderChannel(bcg729EncoderChannelContextStruct *encInst);

/*****************************************************************************/
/* closeBcg729EncoderChannel : free memory of context structure              */
/*    parameters:                                                            */
/*      -(i) encoderChannelContext : the channel context data                */
/*                                                                           */
/*****************************************************************************/
LIBSSH_API void closeBcg729EncoderChannel(bcg729EncoderChannelContextStruct *encoderChannelContext);

/*****************************************************************************/
/* bcg729Encoder :                                                           */
/*    parameters:                                                            */
/*      -(i) encoderChannelContext : context for this encoder channel        */
/*      -(i) inputFrame : 80 samples (16 bits PCM)                           */
/*      -(o) bitStream : The 15 parameters for a frame on 80 bits            */
/*           on 80 bits (5 16bits words)                                     */
/*                                                                           */
/*****************************************************************************/
LIBSSH_API void bcg729Encoder(bcg729EncoderChannelContextStruct *encoderChannelContext, int16_t inputFrame[], uint8_t bitStream[]);
//void test();
#endif /* ifndef ENCODER_H */
