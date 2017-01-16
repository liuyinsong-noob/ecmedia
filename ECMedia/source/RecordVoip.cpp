//
//  RecordVoip.cpp
//  servicecoreVideo
//
//  Created by hubin on 13-12-4.
//
//
#ifdef _WIN32
#ifdef VIDEO_ENABLED
#include <winsock2.h>
#include <windows.h>
#include "webrtc_libyuv.h"
#endif
#endif //_WIN32

#include "RecordVoip.h"
#include "utility.h"
//#include "sometools.h"
#include "tick_util.h"


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "ECMedia.h"

using namespace cloopenwebrtc;

//#define  TEST_RECORD_H264_

#ifdef TEST_RECORD_H264_
FILE *record_h264_data_ = NULL;
#endif // TEST_RECORD_H264_

RecordVoip::RecordVoip()
	:_videoCrit(CriticalSectionWrapper::CreateCriticalSection())
	,_audioCrit(CriticalSectionWrapper::CreateCriticalSection())
	,_critsectH264Encoder(CriticalSectionWrapper::CreateCriticalSection())
	,_videoThread(NULL)
	,_audioThread(NULL)
	,_videoEvent(EventWrapper::Create())
	,_audioEvent(EventWrapper::Create())
	,_playbackList()
	,_recordingList()
	, _frameRecvList()
	, _frameSendList()
	,_frameScreenList()
	,_startRecordWav(false)
	, _startRecordRVideo(false)
	, _startRecordLVideo(false)
	,_startRecordScreen(false)
	,_recordScreenBitRates(640)
	,_captureScreenInterval(100)
	, _recordScreenFps(10)
	,_captureScreenIndex(-1)
	,_wavRecordFile(NULL)
	,_wavRemoteRecordFile(NULL)
	,_wavLocalRecordFile(NULL)
	, _audioSamplingFreq(-1)
	//,_recordVideoType(RecordVideoTypeNone)
	//,_recordAudioType(RecordAudioTypeNone)
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	,_captureScreethread(NULL)
	, _h264RecordRemote(NULL)
	, _h264RecordLocal(NULL)
	,_h264RecordScreen(NULL)
	,_h264Encoder(NULL)
	,_captureScreen(NULL)
#endif
#endif //_WIN32
{
	_videoThread = ThreadWrapper::CreateThread(RecordVoip::RecordVideoThreadRun, this, kHighestPriority, "RecordVideo_Thread");
	_audioThread = ThreadWrapper::CreateThread(RecordVoip::RecordAudioThreadRun, this, kHighestPriority, "RecordAudio_Thread");

    unsigned int tid = 0;
	_videoThread->Start(tid);
    _audioThread->Start(tid);
}

RecordVoip::~RecordVoip()
{
	PrintConsole("%s in.\n", __FUNCTION__);
    _videoThread->SetNotAlive();
    _videoEvent->Set();

	_audioThread->SetNotAlive();
	_audioEvent->Set();

	if(_startRecordWav)
		StopRecordAudio(0);

#ifdef _WIN32
#ifdef VIDEO_ENABLED
    if(_startRecordRVideo)
        StopRecordRemoteVideo(0);

	if (_startRecordRVideo)
		StopRecordLocalVideo(0);

	if(_startRecordScreen)
		StopRecordScreen(0);

	delete _critsectH264Encoder;
	_critsectH264Encoder = NULL;
#endif
#endif //_WIN32

	if(_videoThread->Stop()) {
		delete _videoEvent;
		_videoEvent = NULL;
		delete _videoThread;
		_videoThread = NULL;
	}
	else {
		PrintConsole("RecordVoip failed to stop thread, leaking");
	}

	if(_audioThread->Stop()) {
		delete _audioEvent;
		_audioEvent = NULL;
		delete _audioThread;
		_audioThread = NULL;
	}
	else {
		PrintConsole("RecordVoip failed to stop thread, leaking");
	}

	delete _videoCrit;
	_videoCrit = NULL;

	delete _audioCrit;
	_audioCrit = NULL;

	PrintConsole("%s out.", __FUNCTION__);
}

bool RecordVoip::RecordVideoThreadRun(void* obj)
{
    return static_cast<RecordVoip*>(obj)->ProcessVideoData();
}

bool RecordVoip::RecordAudioThreadRun(void* obj)
{
	return static_cast<RecordVoip*>(obj)->ProcessAudioData();
}


bool RecordVoip::ProcessVideoData()
{
	bool saveWavFlag = true;
	int writeTimes = 0;
	
    if( _videoEvent->Wait(100) == kEventSignaled ) {
		
		//PrintConsole("record video RL=%d SL=%d.\n", _frameRecvList.GetSize(), _frameScreenList.GetSize());

		CriticalSectionScoped lock(_videoCrit);

#ifdef _WIN32
#ifdef VIDEO_ENABLED
		writeTimes = 10;
		while (_frameRecvList.GetSize() && writeTimes-- >= 0) {
			ListItem *storageItem =  _frameRecvList.First();
			if(_startRecordRVideo && storageItem) {
				EncodedVideoData *frame = (EncodedVideoData*)storageItem->GetItem();
				_h264RecordRemote->wirte_video_data(frame->payloadData, frame->payloadSize, (double)frame->timeStamp);
#ifdef TEST_RECORD_H264_
				if(record_h264_data_)
					fwrite(frame->payloadData, 1, frame->payloadSize, record_h264_data_);
#endif // DEBUG
				delete frame;
			}
			_frameRecvList.PopFront();
		}
		//while (_frameRecvList.GetSize() > 100)
		//{
		//	PrintConsole("drop remote pakcet..\n");
		//	_frameRecvList.PopFront();
		//}

		writeTimes = 10;
		while (_frameSendList.GetSize() && writeTimes-- >= 0) {
			ListItem *storageItem = _frameSendList.First();
			if (_startRecordLVideo && storageItem) {
				EncodedVideoData *frame = (EncodedVideoData*)storageItem->GetItem();
				_h264RecordLocal->wirte_video_data(frame->payloadData, frame->payloadSize, (double)frame->timeStamp);
				delete frame;
			}
			_frameSendList.PopFront();
		}
		//while (_frameSendList.GetSize() > 100)
		//{
		//		PrintConsole("drop local pakcet..\n");
		//		_frameSendList.PopFront();
		//}

		writeTimes = 10;
		while (_frameScreenList.GetSize() && writeTimes-- >= 0) {
			ListItem *screenFrameItem =  _frameScreenList.First();
			if(_startRecordScreen && screenFrameItem) {
				EncodedVideoData *frame = (EncodedVideoData*)screenFrameItem->GetItem();
				_h264RecordScreen->wirte_video_data(frame->payloadData, frame->payloadSize, (double)frame->timeStamp);
				delete frame;
			}
			_frameScreenList.PopFront();
		}
		//while (_frameScreenList.GetSize() > 100)
		//{
		//		PrintConsole("drop screen pakcet..\n");
		//		_frameScreenList.PopFront();
		//}
#endif
#endif //_WIN32
	}

	//PrintConsole("%s out.\n", __FUNCTION__);   
    return true;
}

bool RecordVoip::ProcessAudioData()
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED

	bool saveWavFlag = true;
	int writeTimes = 0;
	
	if( _audioEvent->Wait(200) == kEventSignaled ) {

		//PrintConsole("record audio RL=%d LL=%d.\n", _playbackList.GetSize(),_recordingList.GetSize());

		CriticalSectionScoped lock(_audioCrit);

		writeTimes = 20;
		while (_recordingList.GetSize() && writeTimes-- >= 0 ) {

			WebRtc_Word16 *playbackData = NULL;
			WebRtc_Word16 *recordingData = NULL;
			ListItem *playbackItem = _playbackList.First();
			ListItem *recordItem = _recordingList.First();
			if(playbackItem) {
				playbackData = (WebRtc_Word16 *)playbackItem->GetItem();
				_playbackList.PopFront();
			}
			if(recordItem) {
				recordingData = (WebRtc_Word16 *)recordItem->GetItem();
				_recordingList.PopFront();
			}

			if(_startRecordWav && playbackData && _wavRemoteRecordFile) {
				if( fwrite(playbackData, 2, _audioDataLen, _wavRemoteRecordFile) != _audioDataLen) {
					saveWavFlag = false;
				}
			}
			if(_startRecordWav && recordingData && _wavLocalRecordFile) {
				if( fwrite(recordingData, 2, _audioDataLen, _wavLocalRecordFile) != _audioDataLen) {
					saveWavFlag = false;
				}
			}

			if(playbackData && recordingData) {
				voe::MixWithSat((WebRtc_Word16*)playbackData, 1, (WebRtc_Word16*)recordingData, 1, _audioDataLen);
#ifdef _WIN32
#ifdef VIDEO_ENABLED

				if(_startRecordRVideo && _h264RecordRemote) {
					_h264RecordRemote->write_audio_data(playbackData, _audioDataLen, _audioSamplingFreq) ;
				}

				if (_startRecordLVideo && _h264RecordLocal) {
					_h264RecordLocal->write_audio_data(playbackData, _audioDataLen, _audioSamplingFreq);
				}

				if(_startRecordScreen && _h264RecordScreen) {
					_h264RecordScreen->write_audio_data(playbackData, _audioDataLen, _audioSamplingFreq);
				}
#endif
#endif //_WIN32

				if(_startRecordWav && _wavRecordFile) {
					if(fwrite(playbackData, 2, _audioDataLen, _wavRecordFile) != _audioDataLen) {
						saveWavFlag = false;
					}
				}

			} else if(recordingData) {
#ifdef _WIN32
#ifdef VIDEO_ENABLED
				if(_startRecordRVideo && _h264RecordRemote) {
					_h264RecordRemote->write_audio_data(recordingData, _audioDataLen, _audioSamplingFreq) ;
				}

				if (_startRecordLVideo && _h264RecordLocal) {
					_h264RecordLocal->write_audio_data(recordingData, _audioDataLen, _audioSamplingFreq);
				}

				if(_startRecordScreen && _h264RecordScreen) {
					_h264RecordScreen->write_audio_data(recordingData, _audioDataLen, _audioSamplingFreq);
				}
#endif
#endif //_WIN32
				if(_startRecordWav && _wavRecordFile) {
					if(fwrite(recordingData, 2, _audioDataLen, _wavRecordFile) != _audioDataLen) {
						saveWavFlag = false;
					}
				}
			}

			free(playbackData);
			free(recordingData);

			if(!saveWavFlag) {
				PrintConsole("AudioRecord::ProcessAudioData Write data failed");
				StopRecordAudio(-2);
			}
		}
		while(_playbackList.GetSize() > 10) {
			PrintConsole("%s _playbackList delete.\n", __FUNCTION__);
			_playbackList.PopFront();

		}
		while(_recordingList.GetSize() > 10) {
			PrintConsole("%s _recordingList delete.\n", __FUNCTION__);
			_recordingList.PopFront();
		}
	}
	//PrintConsole("%s out.\n", __FUNCTION__);
#endif
#endif //_WIN32
	return true;
}


int RecordVoip::StartRecordRemoteVideo(const char *filename)
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED
    if (!filename) {
        return -1;
    }

    if(_startRecordRVideo) {
        StopRecordRemoteVideo(0);
    }

    if(_remoteVideoFileName) {
		sprintf(_remoteVideoFileName, "%s", filename);
	}

	_h264RecordRemote = new h264_record();
	if(!_h264RecordRemote) {
		return -1;
	}
	_h264RecordRemote->init(_remoteVideoFileName);

#ifdef TEST_RECORD_H264_
	if (!record_h264_data_) {
		char buf[128];
		static int i = 0;
		i++;
		sprintf(buf, "./record_recv_dat_%d.h264", i);
		record_h264_data_ = fopen(buf, "wb");

	}
#endif // TEST_RECORD_H264_

    _startRecordRVideo = true;
	return 0;
#endif
#endif //_WIN32
    return -1;
}

int RecordVoip::StopRecordRemoteVideo(int status)
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	CriticalSectionScoped lock_video(_videoCrit);
	CriticalSectionScoped lock_audio(_audioCrit);

	if (!_startRecordRVideo)
		return 0;

#ifdef TEST_RECORD_H264_
	if (record_h264_data_)
	{
		fclose(record_h264_data_);
		record_h264_data_ = NULL;
	}
#endif // TEST_RECORD_H264_

    _startRecordRVideo = false;
    _videoEvent->Set();

	bool isWrted = _h264RecordRemote->get_write_status();

	_h264RecordRemote->uninit();
	delete _h264RecordRemote;
	_h264RecordRemote = NULL;

	if(isWrted)
		return 0;
	else
		return -2;
#endif
#endif //_WIN32
	return -1;
}

int RecordVoip::StartRecordLocalVideo(const char *filename)
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	if (!filename) {
		return -1;
	}

	if (_startRecordLVideo) {
		StopRecordLocalVideo(0);
	}

	if (_localVideoFileName) {
		sprintf(_localVideoFileName, "%s", filename);
	}

	_h264RecordLocal = new h264_record();
	if (!_h264RecordLocal) {
		return -1;
	}
	_h264RecordLocal->init(_localVideoFileName);

	_startRecordLVideo = true;
	return 0;
#endif
#endif //_WIN32
	return -1;
}
int RecordVoip::StopRecordLocalVideo(int status)
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	CriticalSectionScoped lock_video(_videoCrit);
	CriticalSectionScoped lock_audio(_audioCrit);

	if (!_startRecordLVideo)
		return 0;

	_startRecordLVideo = false;
	_videoEvent->Set();

	bool isWrted = _h264RecordLocal->get_write_status();

	_h264RecordLocal->uninit();
	delete _h264RecordLocal;
	_h264RecordLocal = NULL;

	if (isWrted)
		return 0;
	else
		return -2;
#endif
#endif //_WIN32
	return -1;
}

int RecordVoip::StartRecordAudio(const char *filename)
{
	if(_startRecordWav) {
		StopRecordAudio(0);
	}
	if(filename) {
		sprintf(_wavFileName, "%s", filename);
		_wavRecordFile = fopen(filename, "wb");
		if( !_wavRecordFile ) {
			PrintConsole("AudioRecord can't open file");
			return -1;
		}

		if( WriteWavFileHeader(_wavRecordFile) != 0 ) {
			PrintConsole("AudioRecord Write WAV header failed");
			return -1;
		}
		_startRecordWav = true;
		return 0;
	}

	return -1;
}

int RecordVoip::StartRecordAudioEx(const char *rFileName, const char *lFileName)
{
	if(!rFileName && !lFileName)
	{
		PrintConsole("StartRecordAudioEx File name is NULL.\n");
		return -1;
	}

	if(_startRecordWav) {
		StopRecordAudio(0);
	}

	if(rFileName){
		sprintf(_wavRemoteFileName, "%s", rFileName);
		_wavRemoteRecordFile = fopen(rFileName, "wb");
		if( !_wavRemoteRecordFile ) {
			PrintConsole("AudioRecord can't open file.\n");
			return -1;
		}

		if( WriteWavFileHeader(_wavRemoteRecordFile) != 0 ) {
			PrintConsole("AudioRecord Write WAV header failed.\n");
			return -1;
		}
	}
	if(lFileName) {
		sprintf(_wavLocalFileName, "%s", lFileName);
		_wavLocalRecordFile = fopen(lFileName, "wb");
		if( !_wavLocalRecordFile ) {
			PrintConsole("AudioRecord can't open file.\n");
			return -1;
		}

		if( WriteWavFileHeader(_wavLocalRecordFile) != 0 ) {
			PrintConsole("AudioRecord Write WAV header failed.\n");
			return -1;
		}
	}

	_startRecordWav = true;
	return 0;
}


int RecordVoip::StopRecordAudio(int status)
{

	if(!_startRecordWav)
		return 0;

	CriticalSectionScoped lock_audio(_audioCrit);

	_startRecordWav = false;
	_audioEvent->Set();

	if(_wavRecordFile) {
		StopAudioFile(_wavRecordFile, _wavFileName, status);
		_wavRecordFile = NULL;
	}


	if(_wavRemoteRecordFile) {
		StopAudioFile(_wavRemoteRecordFile, _wavRemoteFileName, status);
		_wavRemoteRecordFile = NULL;
	}


	if(_wavLocalRecordFile) {
		StopAudioFile(_wavLocalRecordFile, _wavLocalFileName, status);
		_wavLocalRecordFile = NULL;
	}

	return 0;
}

int RecordVoip::StopAudioFile(FILE *audioFile, const char *filename,   int status)
{
	fflush(audioFile);

	if( CompleteWavFile(audioFile) < 0 ) {
		PrintConsole("AudioRecord CompleteWavFile Failed\n");
		fclose(audioFile);
		//_call->core->record_audio_status(_call, filename, -1);
	}  else {
		fclose(audioFile);
		//_call->core->record_audio_status(_call, filename, status);
	}

	return 0;
}

bool RecordVoip::CaptureScreenThreadRun(void* obj)
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	RecordVoip* recordvoip = static_cast<RecordVoip*>(obj);

	if (!recordvoip->isAlreadWriteScreenAudio()) {
		Sleep(recordvoip->_captureScreenInterval);
		return true;
	}

	unsigned char *snapShotBuf = NULL;
	int size=0, width=0, height=0;

	int ret = recordvoip->_captureScreen->getScreenFrameEx(&snapShotBuf, &size, &width, &height,
		recordvoip->_recordScreenLeft, recordvoip->_recordScreenTop, recordvoip->_recordScreenWidth, recordvoip->_recordScreenHeight);
	if( !ret ) {
		recordvoip->CapturedScreeImage(snapShotBuf, size, width, height);
	} else {
		Sleep(recordvoip->_captureScreenInterval);
		return true;
	}
	Sleep(recordvoip->_captureScreenInterval);
#endif
#endif //_WIN32
	return true;
}

int RecordVoip::StartRecordScreen(const char *filename, int bitrates, int fps, int screenIndex)
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED

	PrintConsole("%s in.\n", __FUNCTION__);

	if (!filename) {
		PrintConsole("%s failed, filename is NULL.\n", __FUNCTION__);
		return -1;
	}

	if(_startRecordScreen) {
		StopRecordScreen(0);
	}

	sprintf(_recordScreenFileName, "%s", filename);
	_h264RecordScreen = new h264_record();
	if(!_h264RecordScreen) {
		PrintConsole("%s create _h264RecordScreen failed.\n", __FUNCTION__);
		return -1;
	}
	_h264RecordScreen->init(_recordScreenFileName);

	_captureScreen = new CaptureScreen();
	if(!_captureScreen) {
		PrintConsole("%s create _captureScreen failed.\n", __FUNCTION__);
		delete _h264RecordScreen;
		_h264RecordScreen = NULL;
		return -1;
	}

	int screenCount = 0;
	screen * screenInfo;
	screenCount = _captureScreen->getScreenInfo(&screenInfo);
	if(screenIndex < 0 || screenIndex >= screenCount) {
		PrintConsole("%s screenIndex:%d is out of range:%d.\n", __FUNCTION__, screenIndex, screenCount);
		delete _captureScreen;
		_captureScreen = NULL;
		delete _h264RecordScreen;
		_h264RecordScreen = NULL;
		return -1;
	}
	_recordScreenLeft = screenInfo[screenIndex].left;
	_recordScreenTop = screenInfo[screenIndex].top;
	_recordScreenWidth = screenInfo[screenIndex].width;
	if (_recordScreenWidth % 4) {  //width must align 4
		_recordScreenWidth = _recordScreenWidth - _recordScreenWidth % 4;
	}
	_recordScreenHeight = screenInfo[screenIndex].height;
	_captureScreenIndex = screenIndex;

	_recordScreenBitRates = bitrates;
	_recordScreenFps = fps;
	_captureScreenInterval = 1000/fps;

	_captureScreethread = ThreadWrapper::CreateThread(RecordVoip::CaptureScreenThreadRun, this, kNormalPriority, "CaptureScreen");
	_startRecordScreen = true;

	unsigned int tid = 0;
	_captureScreethread->Start(tid);

	PrintConsole("%s out.\n", __FUNCTION__);
	return 0;
#endif
#endif //_WIN32
	return -1;
}

int RecordVoip::StartRecordScreenEx(const char *filename, int bitrates, int fps, int screenIndex, int left, int top, int width, int height)
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	PrintConsole("%s in.\n", __FUNCTION__);
	if (!filename) {
		PrintConsole("%s failed, filename is NULL.\n", __FUNCTION__);
		return -1;
	}

	if(_startRecordScreen) {
		StopRecordScreen(0);
	}

	sprintf(_recordScreenFileName, "%s", filename);
	_h264RecordScreen = new h264_record();
	if(!_h264RecordScreen) {
		PrintConsole("%s create _h264RecordScreen failed.\n", __FUNCTION__);
		return -1;
	}
	_h264RecordScreen->init(_recordScreenFileName);

	_captureScreen = new CaptureScreen();
	if(!_captureScreen) {
		PrintConsole("%s create _captureScreen failed.\n", __FUNCTION__);
		delete _h264RecordScreen;
		_h264RecordScreen = NULL;
		return -1;
	}

	int screenCount = 0;
	screen * screenInfo;
	screenCount = _captureScreen->getScreenInfo(&screenInfo);
	if(screenIndex < 0 || screenIndex >= screenCount) {
		PrintConsole("%s screenIndex:%d is out of range:%d.\n", __FUNCTION__, screenIndex, screenCount);
		delete _captureScreen;
		_captureScreen = NULL;
		delete _h264RecordScreen;
		_h264RecordScreen = NULL;
		return -1;
	}

	int reqLeft = left;
	int reqTop = top;
	int reqRight = left + width;
	int reqButtom = top + height;

	int screenLeft = screenInfo[screenIndex].left;
	int screenTop = screenInfo[screenIndex].top;
	int screenRight = screenInfo[screenIndex].left + screenInfo[screenIndex].width;
	int screenButtom = screenInfo[screenIndex].top + screenInfo[screenIndex].height;

	int dstLeft = (reqLeft > screenLeft) ? reqLeft : screenLeft;
	int dstTop = (reqTop > screenTop) ?  reqTop : screenTop;
	int dstRight = (reqRight > screenRight) ? screenRight : reqRight;
	int dstButtom = (reqButtom > screenButtom) ? screenButtom : reqButtom;

	if(dstLeft >= dstRight  || dstTop >= dstButtom) {
		PrintConsole("%s failed, invalid range. dstLeft:%d dstRight:%d dstTop:%d dstButtom:%d .\n", __FUNCTION__, dstLeft, dstRight, dstTop, dstButtom);
		return -1;
	}

	_recordScreenLeft = dstLeft;
	_recordScreenTop = dstTop;
	_recordScreenWidth = dstRight - dstLeft;
	if (_recordScreenWidth % 4) {  //width must align 4
		_recordScreenWidth = _recordScreenWidth - _recordScreenWidth%4;
	}
	_recordScreenHeight = dstButtom - dstTop;
	_recordScreenBitRates = bitrates;
	_recordScreenFps = fps;
	_captureScreenInterval = 1000/fps;
	_captureScreenIndex = screenIndex;

	_captureScreethread = ThreadWrapper::CreateThread(RecordVoip::CaptureScreenThreadRun, this, kNormalPriority, "CaptureScreen");

	_startRecordScreen = true;
	unsigned int tid = 0;
	_captureScreethread->Start(tid);

	PrintConsole("%s out.\n", __FUNCTION__);
	return 0;
#endif
#endif //_WIN32
	return -1;
}

int RecordVoip::StopRecordScreen(int status)
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	PrintConsole("%s in.\n", __FUNCTION__);

	if(!_startRecordScreen)
		return 0;

	_captureScreethread->SetNotAlive();
	if(_captureScreethread->Stop()){
		delete _captureScreethread;
		_captureScreethread = NULL;
	}

	_startRecordScreen = false;
	_videoEvent->Set();

	CriticalSectionScoped lock_video(_videoCrit);
	CriticalSectionScoped lock_audio(_audioCrit);

	PrintConsole("%s 000.\n", __FUNCTION__);
	_h264RecordScreen->uninit();
	delete _h264RecordScreen;
	_h264RecordScreen = NULL;
	PrintConsole("%s 111.\n", __FUNCTION__);

	delete _h264Encoder;
	_h264Encoder = NULL;

	_frameScreenList.Empty();

	if(_captureScreen) {
		delete _captureScreen;
		_captureScreen = NULL;
	}

	PrintConsole("%s out.\n", __FUNCTION__);
	return 0;
#endif
#endif //_WIN32
	return -1;
}

#if !defined(NO_VOIP_FUNCTION)

//implement VoEMediaProcess
void RecordVoip::Process(int channel, ProcessingTypes type,
                     WebRtc_Word16 audio10ms[], int length,
                         int samplingFreq, bool isStereo)
{
	CriticalSectionScoped lock(_audioCrit);
	if(_startRecordWav  || _startRecordRVideo || _startRecordLVideo || _startRecordScreen)  {
		_audioDataLen = length;
		_audioSamplingFreq = samplingFreq;
		int dataLen = length * sizeof(WebRtc_Word16);
		WebRtc_Word16 *data = (WebRtc_Word16 *)malloc(dataLen);
		memcpy(data, audio10ms, dataLen);
		if(type == kPlaybackPerChannel) {
			_playbackList.PushBack(data);
		}
		else if (type == kRecordingPerChannel) {
			_recordingList.PushBack(data);
		}
		if(_recordingList.GetSize() > 5) {
			_audioEvent->Set();
		}
		PrintConsole("audio in %s RL=%d LL=%d\n", (type==kPlaybackPerChannel)?"R":"L", _playbackList.GetSize(), _recordingList.GetSize());
	}
}

#ifdef _WIN32
#ifdef VIDEO_ENABLED

//implement VCMFrameStorageCallback
WebRtc_Word32 RecordVoip::StoreReceivedFrame(const EncodedVideoData& frameToStore)
{
	CriticalSectionScoped lock(_videoCrit);
	if(_startRecordRVideo && _h264RecordRemote)  {
		if(frameToStore.payloadData && frameToStore.payloadSize > 0) {
			EncodedVideoData *cloneData = new EncodedVideoData(frameToStore);
			_frameRecvList.PushBack(cloneData);
			_videoEvent->Set();
		}
	}
    return 0;
}

WebRtc_Word32 RecordVoip::CapturedScreeImage(unsigned char *imageData, int size, int width, int height)
{
	CriticalSectionScoped lock(_critsectH264Encoder);	

	int stride_y = width;
	int stride_uv = (width + 1) / 2;

	I420VideoFrame capture_frame;
	int ret = capture_frame.CreateEmptyFrame(width,
		height,
		stride_y,
		stride_uv, stride_uv);
	if(ret < 0) {
		PrintConsole("%s Failed to create empty frame, this should only happen due to bad parameters.", __FUNCTION__);
		return -1;
	}

	I420VideoFrame mirror_frame;
	ret = mirror_frame.CreateEmptyFrame(width,
		height,
		stride_y,
		stride_uv, stride_uv);
	if(ret < 0) {
		PrintConsole("%s Failed to create empty frame, this should only happen due to bad parameters.", __FUNCTION__);
		return -1;
	}

	const int conversionResult = ConvertToI420(kRGB24,
		imageData,
		0, 0,  // No cropping
		width, height,
		size,
		kRotateNone,
		&capture_frame);
	if (conversionResult < 0)
	{
		PrintConsole("%s Failed to convert capture frame from type KRGB24 to I420.", __FUNCTION__);
		return -1;
	}

	const int mirrorResult = MirrorI420UpDown(&capture_frame, &mirror_frame);
	if(mirrorResult < 0) 
	{
		PrintConsole("%s Failed to mirror capture frame.", __FUNCTION__);
		return -1;
	}
	
	if(!_h264Encoder) {
		_h264Encoder = cloopenwebrtc::H264Encoder::Create();
		if(!_h264Encoder) {
			return -1;
		}
		//_h264Encoder->setH264EncoderCallback(h264Callback, this);
		_h264Encoder->RegisterEncodeCompleteCallback(this);

		VideoCodec codecInst;
		strncpy(codecInst.plName, "H264", 5);
		codecInst.codecType = kVideoCodecH264;
		// 96 to 127 dynamic payload types for video codecs
		codecInst.plType = VCM_H264_PAYLOAD_TYPE;
		codecInst.startBitrate = _recordScreenBitRates;
		codecInst.minBitrate = _recordScreenBitRates*3/5;
		codecInst.maxBitrate = _recordScreenBitRates*7/5;
		codecInst.maxFramerate = _recordScreenFps;
		codecInst.width = width;
		codecInst.height = height;
		codecInst.numberOfSimulcastStreams = 0;
		codecInst.mode = kSaveToFile;
		_h264Encoder->InitEncode(&codecInst, 1, 0);
	}

	mirror_frame.set_timestamp(90 * (WebRtc_UWord32)TickTime::MillisecondTimestamp());
	CodecSpecificInfo codecSpecificInfo;
	std::vector<VideoFrameType> video_frame_types(kDeltaFrame);
	_h264Encoder->Encode(mirror_frame, &codecSpecificInfo, &video_frame_types);

	return 0;
}


int32_t RecordVoip::Encoded(
	const EncodedImage& encoded_image,
	const CodecSpecificInfo* codec_specific_info,
	const RTPFragmentationHeader* fragmentation)
{
	CriticalSectionScoped lock(_videoCrit);
	if(_startRecordScreen && _h264RecordScreen)  {
		if(encoded_image._buffer && encoded_image._size > 0 && encoded_image._completeFrame) {

			EncodedVideoData *cloneData = new EncodedVideoData();
			cloneData->timeStamp           = encoded_image._timeStamp;
			cloneData->encodedWidth        = encoded_image._encodedWidth;
			cloneData->encodedHeight       = encoded_image._encodedHeight;
			cloneData->completeFrame       = encoded_image._completeFrame;
			cloneData->payloadSize         = encoded_image._length;
			cloneData->fragmentationHeader = *fragmentation;
			if (encoded_image._size > 0)
			{
				cloneData->payloadData = new WebRtc_UWord8[encoded_image._size];
				memcpy(cloneData->payloadData, encoded_image._buffer, encoded_image._size);
			}
			else
			{
				cloneData->payloadData = NULL;
			}
			_frameScreenList.PushBack(cloneData);
			_videoEvent->Set();
		}
	}
	return 0;
}

int32_t RecordVoip::SendData(uint8_t payloadType,
	const EncodedImage& encoded_image,
	const RTPFragmentationHeader& fragmentationHeader,
	const RTPVideoHeader* rtpVideoHdr)
{
	CriticalSectionScoped lock(_videoCrit);
	if (_startRecordLVideo && _h264RecordLocal) {
		if (encoded_image._buffer && encoded_image._size > 0 && encoded_image._completeFrame) {
			EncodedVideoData *cloneData = new EncodedVideoData();
			cloneData->timeStamp = encoded_image._timeStamp;
			cloneData->encodedWidth = encoded_image._encodedWidth;
			cloneData->encodedHeight = encoded_image._encodedHeight;
			cloneData->completeFrame = encoded_image._completeFrame;
			cloneData->payloadSize = encoded_image._length + fragmentationHeader.fragmentationVectorSize * 4;
			cloneData->fragmentationHeader = fragmentationHeader;
			cloneData->payloadData = new WebRtc_UWord8[encoded_image._size + fragmentationHeader.fragmentationVectorSize*4];
			uint8_t *data = encoded_image._buffer;

			uint8_t startcode[4] = { 0x0, 0x0, 0x0, 0x1 };
			uint32_t offset = 0;
			for (int i = 0; i < fragmentationHeader.fragmentationVectorSize; i++) {
				uint32_t nalu_size = fragmentationHeader.fragmentationLength[i];
				uint8_t * nalu = data + fragmentationHeader.fragmentationOffset[i];
				memcpy(cloneData->payloadData + offset, &startcode, 4);
				offset += 4;
				memcpy(cloneData->payloadData + offset, nalu, nalu_size);
				offset += nalu_size;
			}

			//memcpy(cloneData->payloadData, encoded_image._buffer, encoded_image._size);
			_frameSendList.PushBack(cloneData);
			_videoEvent->Set();
		}
	}
	return 0;
}

#endif 
#endif //_WIN32

#endif 

bool RecordVoip::isAlreadWriteScreenAudio()
{
#ifdef _WIN32
#ifdef VIDEO_ENABLED
	if (_h264RecordScreen && _h264RecordScreen->get_audio_freq() > 0) {
		return true;
	}
	return false;
#endif
#endif
	return false;
}
typedef struct {
	char          fccID[4];
	unsigned long dwSize;
	char          fccType[4];
}HEADER;

typedef struct {
	char            fccID[4];
	unsigned long   dwSize;
	unsigned short  wFormatTag;
	unsigned short  wChannels;
	unsigned long   dwSamplesPerSec;
	unsigned long   dwAvgBytesPerSec;
	unsigned short  wBlockAlign;
	unsigned short  uiBitsPerSample;
}FMT;

typedef struct {
	char            fccID[4];
	unsigned long   dwSize;
}DATA;

int RecordVoip::WriteWavFileHeader(FILE *wavFile)
{
	if(!wavFile) {
		return -1;
	}

	fseek(wavFile, 0L, SEEK_SET);

	//以下是为了建立.wav头而准备的变量
	HEADER pcmHEADER;
	FMT pcmFMT;
	DATA pcmDATA;

	if( fwrite(&pcmHEADER, 1, sizeof(pcmHEADER), wavFile) != sizeof(pcmHEADER) )
		return -1;
	if( fwrite(&pcmFMT, 1, sizeof(pcmFMT), wavFile) != sizeof(pcmFMT) )
		return -1;
	if( fwrite(&pcmDATA, 1, sizeof(pcmDATA), wavFile) != sizeof(pcmDATA) )
		return -1;
	return 0;
}

int RecordVoip::CompleteWavFile(FILE *wavFile)
{
	if(!wavFile) {
		return -1;
	}

	//以下是为了建立.wav头而准备的变量
	HEADER pcmHEADER;
	FMT pcmFMT;
	DATA pcmDATA;

	//以下是创建wav头的HEADER;但.dwsize未定，因为不知道Data的长度。
	memcpy(pcmHEADER.fccID,"RIFF", 4);
	pcmHEADER.dwSize = 0; //
	memcpy(pcmHEADER.fccType,"WAVE", 4);

	//以下是创建wav头的FMT;
	memcpy(pcmFMT.fccID,"fmt ", 4);
	pcmFMT.dwSize=16;   //
	pcmFMT.wFormatTag=1;
	pcmFMT.wChannels=1;
	pcmFMT.dwSamplesPerSec=_audioSamplingFreq;
	pcmFMT.dwAvgBytesPerSec=_audioSamplingFreq*2;
	pcmFMT.wBlockAlign=2;
	pcmFMT.uiBitsPerSample=16;
	//以上是创建wav头的FMT;

	//以下是创建wav头的DATA;   但由于DATA.dwsize未知所以不能写入.wav文件
	strcpy(pcmDATA.fccID,"data");
	pcmDATA.dwSize=0; //给pcmDATA.dwsize   0以便于下面给它赋值

	fseek(wavFile, 0L, SEEK_END);
	unsigned int fileSize = ftell(wavFile);
	if( fileSize <= (sizeof(pcmHEADER) + sizeof(pcmFMT) + sizeof(pcmDATA)) ) {
		return -1;
	}

	pcmHEADER.dwSize = fileSize - 8;
	pcmDATA.dwSize = fileSize - sizeof(pcmHEADER) - sizeof(pcmFMT) - sizeof(pcmDATA);

	fseek(wavFile, 0L, SEEK_SET);

	fwrite(&pcmHEADER, 1, sizeof(pcmHEADER), wavFile);
	fwrite(&pcmFMT, 1, sizeof(pcmFMT), wavFile);
	fwrite(&pcmDATA, 1, sizeof(pcmDATA), wavFile);

	return 0;
}
