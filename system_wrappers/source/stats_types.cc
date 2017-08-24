#include "stats_types.h"

#include "../base/checks.h"
#include "../system_wrappers/source/stringencode.h"

StatsReport::Value::Value(StatsValueName name, int64_t value, Type int_type)
	:name_(name), type_(int_type)
{
	DCHECK(int_type == kUInt8 || int_type == kUInt16 || int_type == kUInt32 
		|| int_type == kInt32 || int_type == kUInt64 || int_type == kInt64);
	switch (int_type)
	{
	case kUInt8:
		value_.uint8_ = static_cast<uint8_t>(value);
		break;
	case kUInt16:
		value_.uint16_ = static_cast<uint16_t>(value);
		break;
	case kUInt32:
		value_.uint32_ = static_cast<uint32_t>(value);
		break;
	case kInt32:
		value_.int32_ = static_cast<int32_t>(value);
		break;
	case kUInt64:
		value_.uint64_ = static_cast<uint64_t>(value);
		break;
	case kInt64:
		value_.int64_ = static_cast<int64_t>(value);
		break;
	}
}
StatsReport::Value::Value(StatsValueName name, float f)
	: name_(name), type_(kFloat) {
	value_.float_ = f;
}

StatsReport::Value::Value(StatsValueName name, const std::string& value)
	: name_(name), type_(kString) {
	value_.string_ = new std::string(value);
}

StatsReport::Value::Value(StatsValueName name, const char* value)
	: name_(name), type_(kStaticString) {
	value_.static_string_ = value;
}

StatsReport::Value::Value(StatsValueName name, bool b)
	: name_(name), type_(kBool) {
	value_.bool_ = b;
}

StatsReport::Value::~Value() {
	switch (type_) {
	case kUInt8:
	case kUInt16:
	case kUInt32:
	case kUInt64:
	case kFloat:
	case kBool:
	case kStaticString:
		break;
	case kString:
		delete value_.string_;
		break;
	}
}

bool StatsReport::Value::operator== (const std::string &value) const {
	return (type_ == kString && value_.string_->compare(value) == 0) ||
		   (type_ == kStaticString && value.compare(value_.static_string_) == 0);
}
bool StatsReport::Value::operator== (const char* value) const{
	if (type_ == kString)
		return value_.string_->compare(value) == 0;
	if (type_ != kStaticString)
		return false;
	return value_.static_string_ == value;
}
bool StatsReport::Value::operator==(int64_t value) const {
	switch (type_)
	{
	case kUInt8:
		return value_.uint8_ == static_cast<uint8_t>(value);
	case kUInt16:
		return value_.uint16_ == static_cast<uint16_t>(value);
	case kUInt32:
		return value_.uint32_ == static_cast<uint32_t>(value);
	case  kInt32:
		return value_.int32_ == static_cast<int32_t>(value);
	case kUInt64:
		return value_.uint64_ == static_cast<uint64_t>(value);
	case kInt64:
		return value_.int64_ == static_cast<int64_t>(value);
	default:
		return false;
	}
}

bool StatsReport::Value::operator==(bool value) const {
	return type_ == kBool && value_.bool_ == value;
}

bool StatsReport::Value::operator==(float value) const {
	return type_ == kFloat && value_.float_ == value;
}


uint8_t StatsReport::Value::uint8_val() const {
	DCHECK(type_ == kUInt8);
	return value_.uint8_;
}

uint16_t StatsReport::Value::uint16_val() const {
	DCHECK(type_ == kUInt16);
	return value_.uint16_;
}

uint32_t StatsReport::Value::uint32_val() const {
	DCHECK(type_ == kUInt32);
	return value_.uint32_;
}

int32_t StatsReport::Value::int32_val() const {
	DCHECK(type_ == kInt32);
	return value_.int32_;
}

uint64_t StatsReport::Value::uint64_val() const {
	DCHECK(type_ == kUInt64);
	return value_.uint64_;
}

int64_t StatsReport::Value::int64_val() const {
	DCHECK(type_ == kInt64);
	return value_.int64_;
}

float StatsReport::Value::float_val() const {
	DCHECK(type_ == kFloat);
	return value_.float_;
}

const char* StatsReport::Value::static_string_val() const {
	DCHECK(type_ == kStaticString);
	return value_.static_string_;
}

const std::string& StatsReport::Value::string_val() const {
	DCHECK(type_ == kString);
	return *value_.string_;
}

bool StatsReport::Value::bool_val() const {
	DCHECK(type_ == kBool);
	return value_.bool_;
}

const char* StatsReport::Value::display_name() const
{
	switch (name_)
	{
	case kStatsValueNameReportType:
		return "reportType";
		//basic common
 	case kStatsValueNameTimestamp:
 		return "timestamp";
	case kStatsValueNameChannelId:
		return "channelId";
	case kStatsValueNameSsrc:
		return "ssrc";
	case kStatsValueNameCodecImplementationName:
		return "codecImplementatinName";
			// rtp/rtcp common 
	case kStatsValueNameBytesReceived:
		return "bytesReceived";
	case kStatsValueNameBytesSent:
		return "bytesSent";
	case kStatsValueNameRetransmittedBytes:
		return "retransmittedBytes";
	case kStatsValueNameRetransmittedPackets:
		return "retransmittedPackets";
	case kStatsValueNameFecPackets:
		return "fecPacketsSent";
	case kStatsValueNamePacketsReceived:
		return "packetsReceived";
	case kStatsValueNamePacketsSent:
		return "packetsSent";
	case kStatsValueNameTransmitBitrate:
		return "transmitBitrateKbps";
	case kStatsValueNameRetransmitBitrate:
		return "retransmittedBitrateKbps";
	case kStatsValueNameFecBitrate:
		return "fecBitrateKbps";
	case kStatsValueNameTransmitPacketsRate:
		return "transmitPacketsRate";
	case kStatsValueNameRetransmitPacketsRate:
		return "retransmittedPacketsRate";
	case kStatsValueNamePacketsLost:
		return "packetsLost";
	case kStatsValueNameLossFractionInPercent:
		return "lossFractionInPercent";
	case kStatsValueNameJitterReceived:
		return "jitterReceived";
	case kStatsValueNameFirsReceived:
		return "firsReceived";
	case kStatsValueNameFirsSent:
		return "firsSent";
	case kStatsValueNamePlisReceived:
		return "plisReceived";
	case kStatsValueNamePlisSent:
		return "plisSent";
	case kStatsValueNameNacksReceived:
		return "nacksReceived";
	case kStatsValueNameNacksSent:
		return "nacksSent";
	case kStatsValueNameNacksRequestsReceived:
		return "nacksRequestsReceived";
	case kStatsValueNameNacksRequestsSent:
		return "nacksRequestsSent";
	case kStatsValueNameNacksUniqueRequestsReceived:
		return "nacksUniqueRequestsReceived";
	case kStatsValueNameNacksUniqueRequestsSent:
		return "nacksUniqueRequestsSent";

			//video Send
	case kStatsValueNameEncoderSetting:
		return "encoderSetting";
	case kStatsValueNameCodecSettingFrameWidth:
		return "codecSettingFrameWidth";
	case kStatsValueNameCodecSettingFrameHeight:
		return "codecSettingFrameHeight";
	case kStatsValueNameCodecSettingFrameRate:
		return "codecSettingFrameRate";
	case kStatsValueNameCodecSettingStartBitrate:
		return "codecSettingStartBitrateKbps";
	case kStatsValueNameCodecSettingMinBitrate:
		return "codecSettingMinBitrateKbps";
	case kStatsValueNameCodecSettingMaxBitrate:
		return "codecSettingMaxBitrateKbps";
	case kStatsValueNameCodecSettingTargetBitrate:
		return "codecSettingTargetBitrateKbps";
	case kStatsValueNameCodecSettingSimulcastNum:
		return "codecSettingSimulcastNum";

	case kStatsValueNameCapturedFrameRate:
		return "frameRateCaptured";
	case kStatsValueNameTargetEncFrameRate:
		return "targetEncframeRate";
	case kStatsValueNameActualEncFrameRate:
		return "actualEncframeRate";
	case kStatsValueNameTargetEncBitrate:
		return "targetEncBitrateKbps";
	case kStatsValueNameActualEncBitrate:
		return "actualEncBitrateKbps";
	case kStatsValueNameAvgEncodeMs:
		return "avgEncodeMs";
	case kStatsValueNameEncodeUsagePercent:
		return "encodeUsagePercet";
	case kStatsValueNameCapturedFrameWidth:
		return "frameWidthCaptured";
	case kStatsValueNameQMFrameWidth:
		return "frameWidthQMSetting";
	case kStatsValueNameQMFrameHeight:
		return "frameHeightQMSetting";
	case  kStatsValueNameQMFrameRate:
		return "frameRateQMSetting";
	case kStatsValueNameCapturedFrameHeight:
		return "frameHeightCaptured";
	case kStatsValueNameAvailableReceiveBandwidth:
		return "availableReceiveBandwidthKbps";
	case kStatsValueNameAvailableSendBandwidth:
		return "availableSendBandwidthKbps";
	case kStatsValueNameBucketDelayInMs:
		return "bucketDelayInMs";
	case kStatsValueNameRttInMs:
		return "rttInMs";

			//video recv
	case kStatsValueNameReceivedFrameRate:
		return "frameRateReceived";
	case kStatsValueNameReceivedTotalBitrate:
		return "bitrateReceived";
	case kStatsValueNameDecoderFrameRate:
		return "frameRateDecoded";
	case kStatsValueNameFrameRateRender:
		return "frameRateRender";
	case kStatsValueNameFrameWidthReceived:
		return "frameWidthReceived";
	case kStatsValueNameFrameHeightReceived:
		return "frameHeightReceived";
	case kStatsValueNameDecodeMs:
		return "decodeMs";
	case kStatsValueNameMaxDecodeMs:
		return "maxDecodeMs";
	case kStatsValueNameCurrentDelayMs:
		return "currentDelayMs";
	case kStatsValueNameTargetDelayMs:
		return "targetDelayMs";
	case kStatsValueNameJitterBufferMs:
		return "jitterBufferMs";
	case kStatsValueNameMinPlayoutDelayMs:
		return "minPlayoutDelayMs";
	case kStatsValueNameRenderDelayMs:
		return "renderDelayMs";
	case  kStatsValueNameDiscardedPackets:
		return "discardedPackets";
	case kStatsValueNameLossModePart1:
		return "lossModePart1";
	case kStatsValueNameLossModePart2:
		return "lossModePart2";
	case kStatsValueNameLossModePart3:
		return "lossModePart3";
	case kStatsValueNameLossModePart4:
		return "lossModePart4";
		//audio send
	case kStatsValueNameAudioInputLevel:
		return "audioInputLevel";
	case kStatsValueNameEchoDelayMedian:
		return "echoCancellationEchoDelayMedian";
	case kStatsValueNameEchoDelayStdDev:
		return "echoCancellationEchoDelayStdDev";
	case kStatsValueNameEchoReturnLoss:
		return "echoCancellationReturnLoss";
	case kStatsValueNameEchoReturnLossEnhancement:
		return "echoCancellationReturnLossEnhancement";
		//audio receive
	case kStatsValueNameAudioOutputLevel:
		return "audioOutputLevel";
	case kStatsValueNamePreferredJitterBufferMs:
		return "preferredJitterBufferMs";
	case kStatsValueNameAccelerateRate:
		return "accelerateRate";
	case kStatsValueNameExpandRate:
		return "expandRate";
	case  kStatsValueNamePreemptiveExpandRate:
		return "preemptiveExpandRate";
	case kStatsValueNameDecodingNormal:
		return "decodingNormal";
	case kStatsValueNameDecodingPLC:
		return "decodingPLC";
	case  kStatsValueNameDecodingCNG:
		return "decodingCNG";
	case kStatsValueNameDecodingPLCCNG:
		return "decodingPLCCNG";

	case kStatsValueNameSimulcastSsrc:
		return "simulcastSsrcs";
	case kStatsValueNameSimulcastBytesSent:
		return "simulcastBytesSent";
	case kStatsValueNameSimulcastPacketsSent:
		return "simulcastPacketsSent";
	case kStatsValueNameSimulcastPacketsLost:
		return "simulcastPacketsLost";
	case kStatsValueNameSimulcastJitterReceived:
		return "simulcastJitterReceived";
	case kStatsValueNameSimulcastFirsReceived:
		return "simulcastFirsReceived";
	case kStatsValueNameSimulcastPlisReceived:
		return "simulcastPlisReceived";
	case kStatsValueNameSimulcastNacksReceived:
		return "simulcastNacksReceived";
	case kStatsValueNameSimulcastNacksRequestsReceived:
		return "simulcastNacksRequestsReceived";
	case kStatsValueNameSimulcastNacksUniqueRequestsReceived:
		return "simulcastNacksUniqueRequestsReceived";
	case kStatsValueNameSimulcastTransmitBitrate:
		return "simulcastTransmitBitrate";
	case kStatsValueNameSimulcastRetransmitBitrate:
		return "simulcastRetransmitBitrate";
	}
	return nullptr;
}

std::string StatsReport::Value::ToString() const
{
	switch (type_)
	{
	case kUInt8:
		return cloopenwebrtc::ToString(value_.uint8_);
	case kUInt16:
		return cloopenwebrtc::ToString(value_.uint16_);
	case kUInt32:
		return cloopenwebrtc::ToString(value_.uint32_);
	case kUInt64:
		return cloopenwebrtc::ToString(value_.uint64_);
	case kString:
		return *value_.string_;
	case kStaticString:
		return std::string(value_.static_string_);
	case kFloat:
	case kBool:
		return value_.bool_ ? "true" : "false";
	}
	return std::string();
}

const StatsReport::Value* StatsReport::FindValue(StatsValueName name) const
{	
	Values::const_iterator it = values_.find(name);
	return it == values_.end() ? nullptr : it->second.get();
}

StatsReport::StatsReport(const int64_t &id): id_(id), timestamp_(0){
}

void StatsReport::AddString(StatsValueName name, const std::string &value){
	const Value *found = FindValue(name);
	if (!found || !(*found == value))
		values_[name] = ValuePtr(new Value(name, value));
}
void StatsReport::AddString(StatsValueName name, const char* value){
	const Value *found = FindValue(name);
	if (!found || !(*found == value))
		values_[name] = ValuePtr(new Value(name, value));
}

void StatsReport::AddUInt8(StatsReport::StatsValueName name, uint8_t value) {
	const Value* found = FindValue(name);
	if (!found || !(*found == static_cast<int64_t>(value)))
		values_[name] = ValuePtr(new Value(name, value, Value::kUInt8));
}

void StatsReport::AddUInt16(StatsReport::StatsValueName name, uint16_t value) {
	const Value* found = FindValue(name);
	if (!found || !(*found == static_cast<int64_t>(value)))
		values_[name] = ValuePtr(new Value(name, value, Value::kUInt16));
}

void StatsReport::AddUInt32(StatsReport::StatsValueName name, uint32_t value) {
	const Value* found = FindValue(name);
	if (!found || !(*found == static_cast<int64_t>(value)))
		values_[name] = ValuePtr(new Value(name, value, Value::kUInt32));
}

void StatsReport::AddInt32(StatsValueName name, int32_t value){
	const Value* found = FindValue(name);
	if (!found || !(*found == static_cast<int64_t>(value)))
		values_[name] = ValuePtr(new Value(name, value, Value::kInt32));
}

void StatsReport::AddUInt64(StatsReport::StatsValueName name, uint64_t value) {
	const Value* found = FindValue(name);
	if (!found || !(*found == static_cast<int64_t>(value)))
		values_[name] = ValuePtr(new Value(name, value, Value::kUInt64));
}

void StatsReport::AddInt64(StatsReport::StatsValueName name, int64_t value) {
	const Value* found = FindValue(name);
	if (!found || !(*found == static_cast<int64_t>(value)))
		values_[name] = ValuePtr(new Value(name, value, Value::kInt64));
}

void StatsReport::AddFloat(StatsReport::StatsValueName name, float value) {
	const Value* found = FindValue(name);
	if (!found || !(*found == value))
		values_[name] = ValuePtr(new Value(name, value));
}

void StatsReport::AddBoolean(StatsReport::StatsValueName name, bool value) {
	const Value* found = FindValue(name);
	if (!found || !(*found == value))
		values_[name] = ValuePtr(new Value(name, value));
}


StatsCollection::StatsCollection(){
}

StatsCollection::~StatsCollection() {
	for (auto* r:list_)
	{
		delete r;
	}
}
StatsCollection::const_iterator StatsCollection::begin() const{
	return list_.begin();
}
StatsCollection::const_iterator StatsCollection::end() const {
	return list_.end();
}
int StatsCollection::size() const {
	return list_.size();
}

StatsReport* StatsCollection::InsertNew(const int64_t &id)
{
	StatsReport *report = new StatsReport(id);
	list_.push_back(report);
	return report;
}


StatsReport* StatsCollection::Find(const int64_t &id)
{
	Container::iterator it = list_.begin();
	for (;it!=list_.end(); ++it)
	{
		if ((*it)->Id() == id)
			return *it;
	}
	return nullptr;
}

StatsReport* StatsCollection::FindOrAddNew(const int64_t &id)
{
	StatsReport *ret = Find(id);
	return ret == nullptr ? InsertNew(id) : ret;
}

void StatsCollection::Delete(const int64_t& id)
{
	StatsReport *report = nullptr;
	std::list<StatsReport*>::iterator it = list_.begin();
	for (; it!=list_.end(); ++it)
	{
		report = *it;
		if (report->Id() == id)
		{
			break;
		}
	}
	if (it != list_.end())
	{
		list_.erase(it);
		delete report;
	}
}

