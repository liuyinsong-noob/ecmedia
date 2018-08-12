/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_VIDEO_CODING_MEDIA_OPT_UTIL_H_
#define WEBRTC_MODULES_VIDEO_CODING_MEDIA_OPT_UTIL_H_

#include <math.h>
#include <stdlib.h>

#include "../system_wrappers/include/exp_filter.h"
#include "internal_defines.h"
#include "qm_select.h"
#include "../system_wrappers/include/trace.h"
#include "typedefs.h"

namespace yuntongxunwebrtc {
namespace media_optimization {

// Number of time periods used for (max) window filter for packet loss
// TODO (marpan): set reasonable window size for filtered packet loss,
// adjustment should be based on logged/real data of loss stats/correlation.
enum { kLossPrHistorySize = 10 };

// 1000 ms, total filter length is (kLossPrHistorySize * 1000) ms
enum { kLossPrShortFilterWinMs = 1000 };

// The type of filter used on the received packet loss reports.
enum FilterPacketLossMode {
  kNoFilter,    // No filtering on received loss.
  kAvgFilter,   // Recursive average filter.
  kMaxFilter    // Max-window filter, over the time interval of:
                // (kLossPrHistorySize * kLossPrShortFilterWinMs) ms.
};

// Thresholds for hybrid NACK/FEC
// common to media optimization and the jitter buffer.
const int64_t kLowRttNackMs = 20;

struct VCMProtectionParameters
{
    VCMProtectionParameters() : rtt(0), lossPr(0.0f), bitRate(0.0f),
        packetsPerFrame(0.0f), packetsPerFrameKey(0.0f), frameRate(0.0f),
        keyFrameSize(0.0f), fecRateDelta(0), fecRateKey(0),
        residualPacketLossFec(0.0f), codecWidth(0), codecHeight(0),
        numLayers(1)
        {}

    int64_t             rtt;
    float               lossPr;
    float               bitRate;
    float               packetsPerFrame;
    float               packetsPerFrameKey;
    float               frameRate;
    float               keyFrameSize;
    uint8_t       fecRateDelta;
    uint8_t       fecRateKey;
    float               residualPacketLossFec;
    uint16_t      codecWidth;
    uint16_t      codecHeight;
    int                 numLayers;
};


/******************************/
/* VCMProtectionMethod class  */
/******************************/

enum VCMProtectionMethodEnum
{
    kNack,
    kFec,
    kNackFec,
    kNone
};

class VCMLossProbabilitySample
{
public:
    VCMLossProbabilitySample() : lossPr255(0), timeMs(-1) {};

    uint8_t     lossPr255;
    int64_t     timeMs;
};


class VCMProtectionMethod
{
public:
    VCMProtectionMethod();
    virtual ~VCMProtectionMethod();

    // Updates the efficiency of the method using the parameters provided
    //
    // Input:
    //         - parameters         : Parameters used to calculate efficiency
    //
    // Return value                 : True if this method is recommended in
    //                                the given conditions.
    virtual bool UpdateParameters(const VCMProtectionParameters* parameters) = 0;

    // Returns the protection type
    //
    // Return value                 : The protection type
    enum VCMProtectionMethodEnum Type() const { return _type; }

    // Returns the bit rate required by this protection method
    // during these conditions.
    //
    // Return value                 : Required bit rate
    virtual float RequiredBitRate() { return _efficiency; }

    // Returns the effective packet loss for ER, required by this protection method
    //
    // Return value                 : Required effective packet loss
    virtual uint8_t RequiredPacketLossER() { return _effectivePacketLoss; }

    // Extracts the FEC protection factor for Key frame, required by this protection method
    //
    // Return value                 : Required protectionFactor for Key frame
    virtual uint8_t RequiredProtectionFactorK() { return _protectionFactorK; }

    // Extracts the FEC protection factor for Delta frame, required by this protection method
    //
    // Return value                 : Required protectionFactor for delta frame
    virtual uint8_t RequiredProtectionFactorD() { return _protectionFactorD; }

    // Extracts whether the FEC Unequal protection (UEP) is used for Key frame.
    //
    // Return value                 : Required Unequal protection on/off state.
    virtual bool RequiredUepProtectionK() { return _useUepProtectionK; }

    // Extracts whether the the FEC Unequal protection (UEP) is used for Delta frame.
    //
    // Return value                 : Required Unequal protection on/off state.
    virtual bool RequiredUepProtectionD() { return _useUepProtectionD; }

    virtual int MaxFramesFec() const { return 1; }

    // Updates content metrics
    void UpdateContentMetrics(const VideoContentMetrics* contentMetrics);

protected:

    uint8_t                        _effectivePacketLoss;
    uint8_t                        _protectionFactorK;
    uint8_t                        _protectionFactorD;
    // Estimation of residual loss after the FEC
    float                                _residualPacketLossFec;
    float                                _scaleProtKey;
    int32_t                        _maxPayloadSize;

    VCMQmRobustness*                     _qmRobustness;
    bool                                 _useUepProtectionK;
    bool                                 _useUepProtectionD;
    float                                _corrFecCost;
    enum VCMProtectionMethodEnum         _type;
    float                                _efficiency;
};

class VCMNackMethod : public VCMProtectionMethod
{
public:
    VCMNackMethod();
    virtual ~VCMNackMethod();
    virtual bool UpdateParameters(const VCMProtectionParameters* parameters);
    // Get the effective packet loss
    bool EffectivePacketLoss(const VCMProtectionParameters* parameter);
};

class VCMFecMethod : public VCMProtectionMethod
{
public:
    VCMFecMethod();
    virtual ~VCMFecMethod();
    virtual bool UpdateParameters(const VCMProtectionParameters* parameters);
    // Get the effective packet loss for ER
    bool EffectivePacketLoss(const VCMProtectionParameters* parameters);
    // Get the FEC protection factors
    bool ProtectionFactor(const VCMProtectionParameters* parameters);
    // Get the boost for key frame protection
    uint8_t BoostCodeRateKey(uint8_t packetFrameDelta,
                                   uint8_t packetFrameKey) const;
    // Convert the rates: defined relative to total# packets or source# packets
    uint8_t ConvertFECRate(uint8_t codeRate) const;
    // Get the average effective recovery from FEC: for random loss model
    float AvgRecoveryFEC(const VCMProtectionParameters* parameters) const;
    // Update FEC with protectionFactorD
    void UpdateProtectionFactorD(uint8_t protectionFactorD);
    // Update FEC with protectionFactorK
    void UpdateProtectionFactorK(uint8_t protectionFactorK);
    // Compute the bits per frame. Account for temporal layers when applicable.
    int BitsPerFrame(const VCMProtectionParameters* parameters);

protected:
    enum { kUpperLimitFramesFec = 6 };
    // Thresholds values for the bytes/frame and round trip time, below which we
    // may turn off FEC, depending on |_numLayers| and |_maxFramesFec|.
    // Max bytes/frame for VGA, corresponds to ~140k at 25fps.
    enum { kMaxBytesPerFrameForFec = 700 };
    // Max bytes/frame for CIF and lower: corresponds to ~80k at 25fps.
    enum { kMaxBytesPerFrameForFecLow = 400 };
    // Max bytes/frame for frame size larger than VGA, ~200k at 25fps.
    enum { kMaxBytesPerFrameForFecHigh = 1000 };
};


class VCMNackFecMethod : public VCMFecMethod
{
public:
    VCMNackFecMethod(int64_t lowRttNackThresholdMs,
                     int64_t highRttNackThresholdMs);
    virtual ~VCMNackFecMethod();
    virtual bool UpdateParameters(const VCMProtectionParameters* parameters);
    // Get the effective packet loss for ER
    bool EffectivePacketLoss(const VCMProtectionParameters* parameters);
    // Get the protection factors
    bool ProtectionFactor(const VCMProtectionParameters* parameters);
    // Get the max number of frames the FEC is allowed to be based on.
    int MaxFramesFec() const;
    // Turn off the FEC based on low bitrate and other factors.
    bool BitRateTooLowForFec(const VCMProtectionParameters* parameters);
private:
    int ComputeMaxFramesFec(const VCMProtectionParameters* parameters);

    int64_t _lowRttNackMs;
    int64_t _highRttNackMs;
    int _maxFramesFec;
};

class VCMLossProtectionLogic
{
public:
    VCMLossProtectionLogic(int64_t nowMs);
    ~VCMLossProtectionLogic();

    // Set the protection method to be used
    //
    // Input:
    //        - newMethodType    : New requested protection method type. If one
    //                           is already set, it will be deleted and replaced
    // Return value:             Returns true on update
    bool SetMethod(VCMProtectionMethodEnum newMethodType);

    // Remove requested protection method
    // Input:
    //        - method          : method to be removed (if currently selected)
    //
    // Return value:             Returns true on update
    bool RemoveMethod(VCMProtectionMethodEnum method);

    // Return required bit rate per selected protectin method
    float RequiredBitRate() const;

    // Update the round-trip time
    //
    // Input:
    //          - rtt           : Round-trip time in seconds.
    void UpdateRtt(int64_t rtt);

    // Update residual packet loss
    //
    // Input:
    //          - residualPacketLoss  : residual packet loss:
    //                                  effective loss after FEC recovery
    void UpdateResidualPacketLoss(float _residualPacketLoss);

    // Update the filtered packet loss.
    //
    // Input:
    //          - packetLossEnc :  The reported packet loss filtered
    //                             (max window or average)
    void UpdateFilteredLossPr(uint8_t packetLossEnc);

    // Update the current target bit rate.
    //
    // Input:
    //          - bitRate          : The current target bit rate in kbits/s
    void UpdateBitRate(float bitRate);

    // Update the number of packets per frame estimate, for delta frames
    //
    // Input:
    //          - nPackets         : Number of packets in the latest sent frame.
    void UpdatePacketsPerFrame(float nPackets, int64_t nowMs);

   // Update the number of packets per frame estimate, for key frames
    //
    // Input:
    //          - nPackets         : umber of packets in the latest sent frame.
    void UpdatePacketsPerFrameKey(float nPackets, int64_t nowMs);

    // Update the keyFrameSize estimate
    //
    // Input:
    //          - keyFrameSize     : The size of the latest sent key frame.
    void UpdateKeyFrameSize(float keyFrameSize);

    // Update the frame rate
    //
    // Input:
    //          - frameRate        : The current target frame rate.
    void UpdateFrameRate(float frameRate) { _frameRate = frameRate; }

    // Update the frame size
    //
    // Input:
    //          - width        : The codec frame width.
    //          - height       : The codec frame height.
    void UpdateFrameSize(uint16_t width, uint16_t height);

    // Update the number of active layers
    //
    // Input:
    //          - numLayers    : Number of layers used.
    void UpdateNumLayers(int numLayers);

    // The amount of packet loss to cover for with FEC.
    //
    // Input:
    //          - fecRateKey      : Packet loss to cover for with FEC when
    //                              sending key frames.
    //          - fecRateDelta    : Packet loss to cover for with FEC when
    //                              sending delta frames.
    void UpdateFECRates(uint8_t fecRateKey, uint8_t fecRateDelta)
                       { _fecRateKey = fecRateKey;
                         _fecRateDelta = fecRateDelta; }

    // Update the protection methods with the current VCMProtectionParameters
    // and set the requested protection settings.
    // Return value     : Returns true on update
    bool UpdateMethod();

    // Returns the method currently selected.
    //
    // Return value                 : The protection method currently selected.
    VCMProtectionMethod* SelectedMethod() const;

    // Return the protection type of the currently selected method
    VCMProtectionMethodEnum SelectedType() const;

    // Updates the filtered loss for the average and max window packet loss,
    // and returns the filtered loss probability in the interval [0, 255].
    // The returned filtered loss value depends on the parameter |filter_mode|.
    // The input parameter |lossPr255| is the received packet loss.

    // Return value                 : The filtered loss probability
    uint8_t FilteredLoss(int64_t nowMs, FilterPacketLossMode filter_mode,
                               uint8_t lossPr255);

    void Reset(int64_t nowMs);

    void Release();

private:
    // Sets the available loss protection methods.
    void UpdateMaxLossHistory(uint8_t lossPr255, int64_t now);
    uint8_t MaxFilteredLossPr(int64_t nowMs) const;
    VCMProtectionMethod* _selectedMethod;
    VCMProtectionParameters _currentParameters;
    int64_t _rtt;
    float _lossPr;
    float _bitRate;
    float _frameRate;
    float _keyFrameSize;
    uint8_t _fecRateKey;
    uint8_t _fecRateDelta;
    int64_t _lastPrUpdateT;
    int64_t _lastPacketPerFrameUpdateT;
    int64_t _lastPacketPerFrameUpdateTKey;
    yuntongxunwebrtc::ExpFilter _lossPr255;
    VCMLossProbabilitySample _lossPrHistory[kLossPrHistorySize];
    uint8_t _shortMaxLossPr255;
    yuntongxunwebrtc::ExpFilter _packetsPerFrame;
    yuntongxunwebrtc::ExpFilter _packetsPerFrameKey;
    float _residualPacketLossFec;
    uint16_t _codecWidth;
    uint16_t _codecHeight;
    int _numLayers;
};

}  // namespace media_optimization
}  // namespace webrtc

#endif // WEBRTC_MODULES_VIDEO_CODING_MEDIA_OPT_UTIL_H_
