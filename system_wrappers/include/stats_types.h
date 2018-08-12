#pragma once
#include <string>
#include <map>
#include <vector>
#include <list>
#include "../system_wrappers/include/linked_ptr.h"

enum StatsCollectorMode {
	StatsCollectorModeAll,
	StatsCollectorModeOnlyFull,
	StatsCollectorModeOnlySimplified,
	StatsCollectorModeOther,
};

enum StatsContentType {
	kStatsContentFull,
	kStatsContentSimplified,
};

class StatsReport {
public:
	enum StatsReportType {
		kStatsReportTypeAudioSend,
		kStatsReportTypeAudioRecv,
		kStatsReportTypeVideoSend,
		kStatsReportTypeVideoRecv,
		kStatsReportTypeAudioSend_Simplified,
		kStatsReportTypeAudioRecv_Simplified,
		kStatsReportTypeVideoSend_Simplified,
		kStatsReportTypeVideoRecv_Simplified,
		//kStatsReportTypeVideoCodecSetting,
	};
	enum StatsValueName {
		//type
		kStatsValueNameReportType,
		//basic common
		kStatsValueNameTimestamp,
		kStatsValueNameChannelId,
		kStatsValueNameSsrc,
		kStatsValueNameCodecImplementationName,
		// rtp/rtcp send common 
		kStatsValueNameBytesSent,  //TODO: delete
		kStatsValueNamePacketsSent, //TODO: delete
		kStatsValueNameRetransmittedBytes, //TODO: delete
		kStatsValueNameRetransmittedPackets, //TODO: delete
		kStatsValueNameFecPackets,  //TODO: delete
		kStatsValueNamePacketsLost,
		kStatsValueNameLossFractionInPercent,
		kStatsValueNameJitterReceived,
		kStatsValueNameFirsReceived,
		kStatsValueNamePlisReceived, //TODO: delete
		kStatsValueNameNacksReceived,
		kStatsValueNameNacksRequestsReceived,
		kStatsValueNameNacksUniqueRequestsReceived,
		kStatsValueNameTransmitBitrate,
		kStatsValueNameRetransmitBitrate,
		kStatsValueNameFecBitrate,
		kStatsValueNameTransmitPacketsRate,
		kStatsValueNameRetransmitPacketsRate,
		// rtp/rtcp receive common 
		kStatsValueNameBytesReceived,
		kStatsValueNamePacketsReceived,
		kStatsValueNameFirsSent,
		kStatsValueNamePlisSent,
		kStatsValueNameNacksSent,
		kStatsValueNameNacksRequestsSent,
		kStatsValueNameNacksUniqueRequestsSent,

		//video Send
		kStatsValueNameEncoderSetting,
		kStatsValueNameCodecSettingFrameWidth,
		kStatsValueNameCodecSettingFrameHeight,
		kStatsValueNameCodecSettingFrameRate,
		kStatsValueNameCodecSettingStartBitrate,
		kStatsValueNameCodecSettingMinBitrate,
		kStatsValueNameCodecSettingMaxBitrate,
		kStatsValueNameCodecSettingTargetBitrate,
		kStatsValueNameCodecSettingSimulcastNum,

		kStatsValueNameCapturedFrameWidth,
		kStatsValueNameCapturedFrameHeight,
		kStatsValueNameCapturedFrameRate,
		kStatsValueNameQMFrameWidth,
		kStatsValueNameQMFrameHeight,
		kStatsValueNameQMFrameRate,
		kStatsValueNameTargetEncFrameRate,
		kStatsValueNameActualEncFrameRate,
		kStatsValueNameTargetEncBitrate,
		kStatsValueNameActualEncBitrate,
		kStatsValueNameAvgEncodeMs,
		kStatsValueNameEncodeUsagePercent,
		
		kStatsValueNameAvailableReceiveBandwidth,
		kStatsValueNameAvailableSendBandwidth,
		kStatsValueNameBucketDelayInMs,
		kStatsValueNameRttInMs,

		//video recv
		kStatsValueNameReceivedTotalBitrate,
		kStatsValueNameReceivedFrameRate,
		kStatsValueNameDecoderBitrate,
		kStatsValueNameDecoderFrameRate,
		kStatsValueNameFrameRateRender,
		kStatsValueNameFrameWidthReceived,
		kStatsValueNameFrameHeightReceived,
		kStatsValueNameDecodeMs,
		kStatsValueNameMaxDecodeMs,
		kStatsValueNameTargetDelayMs,
		kStatsValueNameMinPlayoutDelayMs,
		kStatsValueNameRenderDelayMs,
		kStatsValueNameCurrentDelayMs, //media receive common
		kStatsValueNameJitterBufferMs, //media receive common
		kStatsValueNameDiscardedPackets,
		kStatsValueNameLossModePart1,
		kStatsValueNameLossModePart2,
		kStatsValueNameLossModePart3,
		kStatsValueNameLossModePart4,
		//audio send
		kStatsValueNameAudioInputLevel,
		kStatsValueNameEchoDelayMedian,
		kStatsValueNameEchoDelayStdDev,
		kStatsValueNameEchoReturnLoss,
		kStatsValueNameEchoReturnLossEnhancement,
		//audio receive
		kStatsValueNameAudioOutputLevel,
		kStatsValueNamePreferredJitterBufferMs,
		kStatsValueNameAccelerateRate,
		kStatsValueNameExpandRate,
		kStatsValueNamePreemptiveExpandRate,
		kStatsValueNameDecodingNormal,
		kStatsValueNameDecodingCNG,
		kStatsValueNameDecodingPLC,
		kStatsValueNameDecodingPLCCNG,

		//simulcast
		kStatsValueNameSimulcastSsrc,
		kStatsValueNameSimulcastBytesSent,
		kStatsValueNameSimulcastPacketsSent,
		kStatsValueNameSimulcastPacketsLost,
		kStatsValueNameSimulcastJitterReceived,
		kStatsValueNameSimulcastFirsReceived,
		kStatsValueNameSimulcastPlisReceived,
		kStatsValueNameSimulcastNacksReceived,
		kStatsValueNameSimulcastNacksRequestsReceived,
		kStatsValueNameSimulcastNacksUniqueRequestsReceived,
		kStatsValueNameSimulcastTransmitBitrate,
		kStatsValueNameSimulcastRetransmitBitrate,
	};

	struct Value {
		enum Type {
			kUInt8,           // uint8_t.
			kUInt16,         // uint16_t.
			kUInt32,			//uint32_t
			kInt32,			//int32_t
			kUInt64,		//uint64_t
			kInt64,			//int64_t
			kFloat,         // float.
			kString,        // std::string
			kStaticString,  // const char*.
			kBool,          // bool.
		};
		const StatsValueName name_;

		Value(StatsValueName name, int64_t value, Type int_type);
		Value(StatsValueName name, float f);
		Value(StatsValueName name, const std::string& value);
		Value(StatsValueName name, const char* value);
		Value(StatsValueName name, bool b);

		~Value();

		bool operator== (const std::string &value) const;
		bool operator== (const char* value) const;
		bool operator== (const int64_t value) const;
		bool operator== (const float value) const;
		bool operator== (const bool value) const;


		uint8_t uint8_val() const;
		uint16_t uint16_val() const;
		uint32_t uint32_val() const;
		int32_t int32_val() const;
		uint64_t uint64_val() const;
		int64_t int64_val() const;
		float float_val() const;
		const char* static_string_val() const;
		const std::string& string_val() const;
		bool bool_val() const;

		Type type() const { return type_; }
		std::string ToString() const;

		// Returns the string representation of |name|.
		const char* display_name() const;
	private:
		const Type type_;
		union InternalType {
			uint8_t uint8_;
			uint16_t uint16_;
			uint32_t uint32_;
			int32_t	 int32_;
			uint64_t uint64_;
			int64_t int64_;
			float float_;
			bool bool_;
			std::string* string_;
			const char* static_string_;
		} value_;
	};

	explicit StatsReport(const int64_t& id);
	typedef yuntongxunwebrtc::linked_ptr<Value> ValuePtr;
	typedef std::map<StatsValueName, ValuePtr> Values;
	const Value* FindValue(StatsValueName name) const;
	void AddString(StatsValueName name, const std::string& value);
	void AddString(StatsValueName name, const char* value);
	
	void AddUInt8(StatsValueName name, uint8_t value);
	void AddUInt16(StatsValueName name, uint16_t value);
	void AddUInt32(StatsValueName name, uint32_t value);
	void AddInt32(StatsValueName name, int32_t value);
	void AddUInt64(StatsValueName name, uint64_t value);
	void AddInt64(StatsValueName name, int64_t value);

	void AddFloat(StatsValueName name, float value);
	void AddBoolean(StatsValueName name, bool value);
	void SetReportType(StatsReportType type) { type_ = type; }
	void SetChannelId(int32_t channal_id) { channel_id_ = channal_id; }
	const int64_t& Id() const { return id_; }
	const Values& values() const { return values_; }
	const int32_t channel_id() const { return channel_id_; }
	const int type() { return type_; }

private:
	int64_t		id_;
	int32_t		channel_id_;
	int64_t	timestamp_;
	Values	values_;
	StatsReportType	type_; 
};

/*typedef std::vector<const StatsReport*> StatsReports;*/

class StatsCollection
{
public:
	StatsCollection();
	~StatsCollection();
	typedef std::list<StatsReport *> Container;
/*	typedef Container::iterator iterator;*/
	typedef Container::const_iterator const_iterator;

	const_iterator begin() const;
	const_iterator end() const;
	int size() const;

	StatsReport* Find(const int64_t &id);
	StatsReport* InsertNew(const int64_t &id);
	StatsReport* FindOrAddNew(const int64_t &id);
	void Delete(const int64_t& id);

private:
	/*Container list_;*/
	std::list<StatsReport*> list_;
};

