/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "examples/peerconnection/client/main_wnd.h"

#include <math.h>

#include "api/video/i420_buffer.h"
#include "examples/peerconnection/client/defaults.h"
#include "rtc_base/arraysize.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "third_party/libyuv/include/libyuv/convert_argb.h"

#include "media_client_old.h"

// MediaClient g_MediaClient;

ATOM MainWnd::wnd_class_ = 0;
const wchar_t MainWnd::kClassName[] = L"WebRTC_MainWnd";

namespace {

const char kConnecting[] = "Connecting... ";
const char kNoVideoStreams[] = "(no video streams either way)";
const char kNoIncomingStream[] = "(no incoming video)";

void CalculateWindowSizeForText(HWND wnd,
                                const wchar_t* text,
                                size_t* width,
                                size_t* height) {
  HDC dc = ::GetDC(wnd);
  RECT text_rc = {0};
  ::DrawTextW(dc, text, -1, &text_rc, DT_CALCRECT | DT_SINGLELINE);
  ::ReleaseDC(wnd, dc);
  RECT client, window;
  ::GetClientRect(wnd, &client);
  ::GetWindowRect(wnd, &window);

  *width = text_rc.right - text_rc.left;
  *width += (window.right - window.left) - (client.right - client.left);
  *height = text_rc.bottom - text_rc.top;
  *height += (window.bottom - window.top) - (client.bottom - client.top);
}

HFONT GetDefaultFont() {
  static HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
  return font;
}

std::string GetWindowText(HWND wnd) {
  char text[MAX_PATH] = {0};
  ::GetWindowTextA(wnd, &text[0], ARRAYSIZE(text));
  return text;
}

void AddListBoxItem(HWND listbox, const std::string& str, LPARAM item_data) {
  LRESULT index = ::SendMessageA(listbox, LB_ADDSTRING, 0,
                                 reinterpret_cast<LPARAM>(str.c_str()));
  ::SendMessageA(listbox, LB_SETITEMDATA, index, item_data);
}

}  // namespace

MainWnd::MainWnd(const char* server,
                 int port,
                 bool auto_connect,
                 bool auto_call)
    : ui_(CONNECT_TO_SERVER),
      wnd_(NULL),
      edit1_(NULL),
      edit2_(NULL),
      label1_(NULL),
      label2_(NULL),
      button_(NULL),
      listbox_(NULL),
      destroyed_(false),
      nested_msg_(NULL),
      callback_(NULL),
      server_(server),
      auto_connect_(auto_connect),
      auto_call_(auto_call),
      m_pMediaClient(new rtc::RefCountedObject<MediaClient>(this)) {
  char buffer[10];
  snprintf(buffer, sizeof(buffer), "%i", port);
  port_ = buffer;
}

MainWnd::~MainWnd() {
  RTC_DCHECK(!IsWindow());
}

bool MainWnd::Create() {
  RTC_DCHECK(wnd_ == NULL);
  if (!RegisterWindowClass())
    return false;

  ui_thread_id_ = ::GetCurrentThreadId();
  wnd_ =
      ::CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, kClassName, L"WebRTC",
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), this);

  ::SendMessage(wnd_, WM_SETFONT, reinterpret_cast<WPARAM>(GetDefaultFont()),
                TRUE);

  CreateChildWindows();
  SwitchToConnectUI();

  return wnd_ != NULL;
}

bool MainWnd::Destroy() {
  BOOL ret = FALSE;
  if (IsWindow()) {
    ret = ::DestroyWindow(wnd_);
  }

  return ret != FALSE;
}

void MainWnd::RegisterObserver(MainWndCallback* callback) {
  callback_ = callback;
}

bool MainWnd::IsWindow() {
  return wnd_ && ::IsWindow(wnd_) != FALSE;
}

bool MainWnd::PreTranslateMessage(MSG* msg) {
  bool ret = false;
  if (msg->message == WM_CHAR) {
    if (msg->wParam == VK_TAB) {
      HandleTabbing();
      ret = true;
    } else if (msg->wParam == VK_RETURN) {
      OnDefaultAction();
      ret = true;
    } else if (msg->wParam == VK_ESCAPE) {
      if (callback_) {
        if (ui_ == STREAMING) {
          callback_->DisconnectFromCurrentPeer();
        } else {
          callback_->DisconnectFromServer();
        }
      }
    }
  } else if (msg->hwnd == NULL && msg->message == UI_THREAD_CALLBACK) {
    callback_->UIThreadCallback(static_cast<int>(msg->wParam),
                                reinterpret_cast<void*>(msg->lParam));
    ret = true;
  }
  return ret;
}

void MainWnd::SwitchToConnectUI() {
  RTC_DCHECK(IsWindow());
  LayoutPeerListUI(false);
  ui_ = CONNECT_TO_SERVER;
  LayoutConnectUI(true);
  ::SetFocus(edit1_);

  if (auto_connect_)
    ::PostMessage(button_, BM_CLICK, 0, 0);
}

void MainWnd::SwitchToPeerList(const Peers& peers) {
  LayoutConnectUI(false);

  ::SendMessage(listbox_, LB_RESETCONTENT, 0, 0);

  AddListBoxItem(listbox_, "List of currently connected peers:", -1);
  Peers::const_iterator i = peers.begin();
  for (; i != peers.end(); ++i)
    AddListBoxItem(listbox_, i->second.c_str(), i->first);

  ui_ = LIST_PEERS;
  LayoutPeerListUI(true);
  ::SetFocus(listbox_);

  if (auto_call_ && peers.begin() != peers.end()) {
    // Get the number of items in the list
    LRESULT count = ::SendMessage(listbox_, LB_GETCOUNT, 0, 0);
    if (count != LB_ERR) {
      // Select the last item in the list
      LRESULT selection = ::SendMessage(listbox_, LB_SETCURSEL, count - 1, 0);
      if (selection != LB_ERR)
        ::PostMessage(wnd_, WM_COMMAND,
                      MAKEWPARAM(GetDlgCtrlID(listbox_), LBN_DBLCLK),
                      reinterpret_cast<LPARAM>(listbox_));
    }
  }
}

void MainWnd::SwitchToStreamingUI() {
  LayoutConnectUI(false);
  LayoutPeerListUI(false);
  ui_ = STREAMING;
}

void MainWnd::MessageBox(const char* caption, const char* text, bool is_error) {
  DWORD flags = MB_OK;
  if (is_error)
    flags |= MB_ICONERROR;

  ::MessageBoxA(handle(), text, caption, flags);
}

void MainWnd::StartLocalRenderer(webrtc::VideoTrackInterface* local_video) {
  local_renderer_.reset(new VideoRenderer(handle(), 1, 1, local_video));
}

void MainWnd::StopLocalRenderer() {
  local_renderer_.reset();
}

void MainWnd::StartRemoteRenderer(webrtc::VideoTrackInterface* remote_video) {
  remote_renderer_.reset(new VideoRenderer(handle(), 1, 1, remote_video));
}

void MainWnd::StopRemoteRenderer() {
  remote_renderer_.reset();
}

void MainWnd::QueueUIThreadCallback(int msg_id, void* data) {
  ::PostThreadMessage(ui_thread_id_, UI_THREAD_CALLBACK,
                      static_cast<WPARAM>(msg_id),
                      reinterpret_cast<LPARAM>(data));
}

void MainWnd::OnPaint() {
  PAINTSTRUCT ps;
  ::BeginPaint(handle(), &ps);

  RECT rc;
  ::GetClientRect(handle(), &rc);

  VideoRenderer* local_renderer = local_renderer_.get();
  VideoRenderer* remote_renderer = remote_renderer_.get();
  if (ui_ == STREAMING && remote_renderer && local_renderer) {
    AutoLock<VideoRenderer> local_lock(local_renderer);
    AutoLock<VideoRenderer> remote_lock(remote_renderer);

    const BITMAPINFO& bmi = remote_renderer->bmi();
    int height = abs(bmi.bmiHeader.biHeight);
    int width = bmi.bmiHeader.biWidth;

    const uint8_t* image = remote_renderer->image();
    if (image != NULL) {
      HDC dc_mem = ::CreateCompatibleDC(ps.hdc);
      ::SetStretchBltMode(dc_mem, HALFTONE);

      // Set the map mode so that the ratio will be maintained for us.
      HDC all_dc[] = {ps.hdc, dc_mem};
      for (size_t i = 0; i < arraysize(all_dc); ++i) {
        SetMapMode(all_dc[i], MM_ISOTROPIC);
        SetWindowExtEx(all_dc[i], width, height, NULL);
        SetViewportExtEx(all_dc[i], rc.right, rc.bottom, NULL);
      }

      HBITMAP bmp_mem = ::CreateCompatibleBitmap(ps.hdc, rc.right, rc.bottom);
      HGDIOBJ bmp_old = ::SelectObject(dc_mem, bmp_mem);

      POINT logical_area = {rc.right, rc.bottom};
      DPtoLP(ps.hdc, &logical_area, 1);

      HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
      RECT logical_rect = {0, 0, logical_area.x, logical_area.y};
      ::FillRect(dc_mem, &logical_rect, brush);
      ::DeleteObject(brush);

      int x = (logical_area.x / 2) - (width / 2);
      int y = (logical_area.y / 2) - (height / 2);

      StretchDIBits(dc_mem, x, y, width, height, 0, 0, width, height, image,
                    &bmi, DIB_RGB_COLORS, SRCCOPY);

      if ((rc.right - rc.left) > 200 && (rc.bottom - rc.top) > 200) {
        const BITMAPINFO& bmi = local_renderer->bmi();
        image = local_renderer->image();
        int thumb_width = bmi.bmiHeader.biWidth / 4;
        int thumb_height = abs(bmi.bmiHeader.biHeight) / 4;
        StretchDIBits(dc_mem, logical_area.x - thumb_width - 10,
                      logical_area.y - thumb_height - 10, thumb_width,
                      thumb_height, 0, 0, bmi.bmiHeader.biWidth,
                      -bmi.bmiHeader.biHeight, image, &bmi, DIB_RGB_COLORS,
                      SRCCOPY);
      }

      BitBlt(ps.hdc, 0, 0, logical_area.x, logical_area.y, dc_mem, 0, 0,
             SRCCOPY);

      // Cleanup.
      ::SelectObject(dc_mem, bmp_old);
      ::DeleteObject(bmp_mem);
      ::DeleteDC(dc_mem);
    } else {
      // We're still waiting for the video stream to be initialized.
      HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
      ::FillRect(ps.hdc, &rc, brush);
      ::DeleteObject(brush);

      HGDIOBJ old_font = ::SelectObject(ps.hdc, GetDefaultFont());
      ::SetTextColor(ps.hdc, RGB(0xff, 0xff, 0xff));
      ::SetBkMode(ps.hdc, TRANSPARENT);

      std::string text(kConnecting);
      if (!local_renderer->image()) {
        text += kNoVideoStreams;
      } else {
        text += kNoIncomingStream;
      }
      ::DrawTextA(ps.hdc, text.c_str(), -1, &rc,
                  DT_SINGLELINE | DT_CENTER | DT_VCENTER);
      ::SelectObject(ps.hdc, old_font);
    }
  } else {
    HBRUSH brush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
    ::FillRect(ps.hdc, &rc, brush);
    ::DeleteObject(brush);
  }

  ::EndPaint(handle(), &ps);
}

void MainWnd::OnDestroyed() {
  PostQuitMessage(0);
}

//static const char sdp_answer[] = "{\
//\"sdp\" : \"v=0\r\no=- 101907403895895076 2 IN IP4 127.0.0.1\r\ns=-\r\nt=0 0\r\na=group:BUNDLE 0 1\r\na=msid-semantic: WMS stream_id\r\nm=audio 9 RTP/AVPF 111 103 104 9 102 0 8 106 105 13 110 112 113 126\r\nc=IN IP4 0.0.0.0\r\na=rtcp:9 IN IP4 0.0.0.0\r\na=ice-ufrag:Gy5Z\r\na=ice-pwd:pF149pzUfNk/86A8r0HsSIvJ\r\na=ice-options:trickle\r\na=mid:0\r\na=extmap:1 urn:ietf:params:rtp-hdrext:ssrc-audio-level\r\na=extmap:2 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\na=extmap:3 urn:ietf:params:rtp-hdrext:sdes:mid\r\na=extmap:4 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id\r\na=extmap:5 urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id\r\na=sendrecv\r\na=msid:stream_id audio_label\r\na=rtcp-mux\r\na=rtpmap:111 opus/48000/2\r\na=rtcp-fb:111 transport-cc\r\na=fmtp:111 minptime=10;useinbandfec=1\r\na=rtpmap:103 ISAC/16000\r\na=rtpmap:104 ISAC/32000\r\na=rtpmap:9 G722/8000\r\na=rtpmap:102 ILBC/8000\r\na=rtpmap:0 PCMU/8000\r\na=rtpmap:8 PCMA/8000\r\na=rtpmap:106 CN/32000\r\na=rtpmap:105 CN/16000\r\na=rtpmap:13 CN/8000\r\na=rtpmap:110 telephone-event/48000\r\na=rtpmap:112 telephone-event/32000\r\na=rtpmap:113 telephone-event/16000\r\na=rtpmap:126 telephone-event/8000\r\na=ssrc:1194352311 cname:PsVDQJ3VfQu5PE7D\r\nm=video 9 RTP/AVPF 96 97 98 99 100 101 127 121 125 120 124 107 108 109 123 119 122\r\nc=IN IP4 0.0.0.0\r\na=rtcp:9 IN IP4 0.0.0.0\r\na=ice-ufrag:Gy5Z\r\na=ice-pwd:pF149pzUfNk/86A8r0HsSIvJ\r\na=ice-options:trickle\r\na=mid:1\r\na=extmap:14 urn:ietf:params:rtp-hdrext:toffset\r\na=extmap:13 http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time\r\na=extmap:12 urn:3gpp:video-orientation\r\na=extmap:2 http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01\r\na=extmap:11 http://www.webrtc.org/experiments/rtp-hdrext/playout-delay\r\na=extmap:6 http://www.webrtc.org/experiments/rtp-hdrext/video-content-type\r\na=extmap:7 http://www.webrtc.org/experiments/rtp-hdrext/video-timing\r\na=extmap:8 http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07\r\na=extmap:9 http://www.webrtc.org/experiments/rtp-hdrext/color-space\r\na=extmap:3 urn:ietf:params:rtp-hdrext:sdes:mid\r\na=extmap:4 urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id\r\na=extmap:5 urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id\r\na=sendrecv\r\na=msid:stream_id video_label\r\na=rtcp-mux\r\na=rtcp-rsize\r\na=rtpmap:96 VP8/90000\r\na=rtcp-fb:96 goog-remb\r\na=rtcp-fb:96 transport-cc\r\na=rtcp-fb:96 ccm fir\r\na=rtcp-fb:96 nack\r\na=rtcp-fb:96 nack pli\r\na=rtpmap:97 rtx/90000\r\na=fmtp:97 apt=96\r\na=rtpmap:98 VP9/90000\r\na=rtcp-fb:98 goog-remb\r\na=rtcp-fb:98 transport-cc\r\na=rtcp-fb:98 ccm fir\r\na=rtcp-fb:98 nack\r\na=rtcp-fb:98 nack pli\r\na=fmtp:98 profile-id=0\r\na=rtpmap:99 rtx/90000\r\na=fmtp:99 apt=98\r\na=rtpmap:100 VP9/90000\r\na=rtcp-fb:100 goog-remb\r\na=rtcp-fb:100 transport-cc\r\na=rtcp-fb:100 ccm fir\r\na=rtcp-fb:100 nack\r\na=rtcp-fb:100 nack pli\r\na=fmtp:100 profile-id=2\r\na=rtpmap:101 rtx/90000\r\na=fmtp:101 apt=100\r\na=rtpmap:127 H264/90000\r\na=rtcp-fb:127 goog-remb\r\na=rtcp-fb:127 transport-cc\r\na=rtcp-fb:127 ccm fir\r\na=rtcp-fb:127 nack\r\na=rtcp-fb:127 nack pli\r\na=fmtp:127 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42001f\r\na=rtpmap:121 rtx/90000\r\na=fmtp:121 apt=127\r\na=rtpmap:125 H264/90000\r\na=rtcp-fb:125 goog-remb\r\na=rtcp-fb:125 transport-cc\r\na=rtcp-fb:125 ccm fir\r\na=rtcp-fb:125 nack\r\na=rtcp-fb:125 nack pli\r\na=fmtp:125 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42001f\r\na=rtpmap:120 rtx/90000\r\na=fmtp:120 apt=125\r\na=rtpmap:124 H264/90000\r\na=rtcp-fb:124 goog-remb\r\na=rtcp-fb:124 transport-cc\r\na=rtcp-fb:124 ccm fir\r\na=rtcp-fb:124 nack\r\na=rtcp-fb:124 nack pli\r\na=fmtp:124 level-asymmetry-allowed=1;packetization-mode=1;profile-level-id=42e01f\r\na=rtpmap:107 rtx/90000\r\na=fmtp:107 apt=124\r\na=rtpmap:108 H264/90000\r\na=rtcp-fb:108 goog-remb\r\na=rtcp-fb:108 transport-cc\r\na=rtcp-fb:108 ccm fir\r\na=rtcp-fb:108 nack\r\na=rtcp-fb:108 nack pli\r\na=fmtp:108 level-asymmetry-allowed=1;packetization-mode=0;profile-level-id=42e01f\r\na=rtpmap:109 rtx/90000\r\na=fmtp:109 apt=108\r\na=rtpmap:123 red/90000\r\na=rtpmap:119 rtx/90000\r\na=fmtp:119 apt=123\r\na=rtpmap:122 ulpfec/90000\r\na=ssrc-group:FID 1934118080 1680690671\r\na=ssrc:1934118080 cname:PsVDQJ3VfQu5PE7D\r\na=ssrc:1680690671 cname:PsVDQJ3VfQu5PE7D\r\n\",\
//\"type\" : \"answer\"}";
//
//static const char sdp_candidate[] = "{\
//\"candidate\" : \"candidate:3350409123 1 udp 2122260223 192.168.0.101 55555 typ host generation 0 ufrag Gy5Z network-id 3\",\
//\"sdpMLineIndex\" : 0,\"sdpMid\" : \"0\"}";

void MainWnd::OnDefaultAction() {
  //static bool s_bUse = false;
  std::string type_str(GetWindowText(edit4_));
  std::string server(GetWindowText(edit1_));
  std::string port_str(GetWindowText(edit2_));
  std::string local_str(GetWindowText(edit3_));

  if (m_pMediaClient) {
    rtc::SocketAddress local, remote;
    std::ostringstream strL, strS;
    strL << local_str << ":" << port_str;
    local.FromString(strL.str().c_str());
    strS << server << ":" << port_str;
    remote.FromString(strS.str().c_str());

    m_pMediaClient->SetSocketAddress(local, remote, 7078, 9078);
  }

  bool useType = type_str.compare("p2p") ? false : true;
  if (useType) {
    m_pMediaClient->InitializePeerConnection();

    //std::string strSdp;
    //m_pMediaClient->SetLocalDescription("");
    //m_pMediaClient->SetRemoteDescription(sdp_answer, sdp_candidate);
    return;
  }
  if (!callback_)
    return;
  if (ui_ == CONNECT_TO_SERVER) {

    int port = port_str.length() ? atoi(port_str.c_str()) : 0;
    callback_->StartLogin(server, port);
    // g_MediaClient.MediaBuild();
  } else if (ui_ == LIST_PEERS) {
    LRESULT sel = ::SendMessage(listbox_, LB_GETCURSEL, 0, 0);
    if (sel != LB_ERR) {
      LRESULT peer_id = ::SendMessage(listbox_, LB_GETITEMDATA, sel, 0);
      if (peer_id != -1 && callback_) {
        callback_->ConnectToPeer(peer_id);
      }
    }
  } else {
    ::MessageBoxA(wnd_, "OK!", "Yeah", MB_OK);
  }
}

bool MainWnd::OnMessage(UINT msg, WPARAM wp, LPARAM lp, LRESULT* result) {
  switch (msg) {
    case WM_ERASEBKGND:
      *result = TRUE;
      return true;

    case WM_PAINT:
      OnPaint();
      return true;

    case WM_SETFOCUS:
      if (ui_ == CONNECT_TO_SERVER) {
        SetFocus(edit1_);
      } else if (ui_ == LIST_PEERS) {
        SetFocus(listbox_);
      }
      return true;

    case WM_SIZE:
      if (ui_ == CONNECT_TO_SERVER) {
        LayoutConnectUI(true);
      } else if (ui_ == LIST_PEERS) {
        LayoutPeerListUI(true);
      }
      break;

    case WM_CTLCOLORSTATIC:
      *result = reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
      return true;

    case WM_COMMAND:
      if (button_ == reinterpret_cast<HWND>(lp)) {
        if (BN_CLICKED == HIWORD(wp))
          OnDefaultAction();
      } else if (listbox_ == reinterpret_cast<HWND>(lp)) {
        if (LBN_DBLCLK == HIWORD(wp)) {
          OnDefaultAction();
        }
      }
      return true;

    case WM_CLOSE:
      if (callback_)
        callback_->Close();
      break;
  }
  return false;
}

// static
LRESULT CALLBACK MainWnd::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  MainWnd* me =
      reinterpret_cast<MainWnd*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (!me && WM_CREATE == msg) {
    CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lp);
    me = reinterpret_cast<MainWnd*>(cs->lpCreateParams);
    me->wnd_ = hwnd;
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(me));
  }

  LRESULT result = 0;
  if (me) {
    void* prev_nested_msg = me->nested_msg_;
    me->nested_msg_ = &msg;

    bool handled = me->OnMessage(msg, wp, lp, &result);
    if (WM_NCDESTROY == msg) {
      me->destroyed_ = true;
    } else if (!handled) {
      result = ::DefWindowProc(hwnd, msg, wp, lp);
    }

    if (me->destroyed_ && prev_nested_msg == NULL) {
      me->OnDestroyed();
      me->wnd_ = NULL;
      me->destroyed_ = false;
    }

    me->nested_msg_ = prev_nested_msg;
  } else {
    result = ::DefWindowProc(hwnd, msg, wp, lp);
  }

  return result;
}

// static
bool MainWnd::RegisterWindowClass() {
  if (wnd_class_)
    return true;

  WNDCLASSEXW wcex = {sizeof(WNDCLASSEX)};
  wcex.style = CS_DBLCLKS;
  wcex.hInstance = GetModuleHandle(NULL);
  wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
  wcex.lpfnWndProc = &WndProc;
  wcex.lpszClassName = kClassName;
  wnd_class_ = ::RegisterClassExW(&wcex);
  RTC_DCHECK(wnd_class_ != 0);
  return wnd_class_ != 0;
}

void MainWnd::CreateChildWindow(HWND* wnd,
                                MainWnd::ChildWindowID id,
                                const wchar_t* class_name,
                                DWORD control_style,
                                DWORD ex_style) {
  if (::IsWindow(*wnd))
    return;

  // Child windows are invisible at first, and shown after being resized.
  DWORD style = WS_CHILD | control_style;
  *wnd = ::CreateWindowExW(ex_style, class_name, L"", style, 100, 100, 100, 100,
                           wnd_, reinterpret_cast<HMENU>(id),
                           GetModuleHandle(NULL), NULL);
  RTC_DCHECK(::IsWindow(*wnd) != FALSE);
  ::SendMessage(*wnd, WM_SETFONT, reinterpret_cast<WPARAM>(GetDefaultFont()),
                TRUE);
}

void MainWnd::CreateChildWindows() {
  // Create the child windows in tab order.
  CreateChildWindow(&label1_, LABEL1_ID, L"Static", ES_CENTER | ES_READONLY, 0);
  CreateChildWindow(&edit1_, EDIT_ID, L"Edit",
                    ES_LEFT | ES_NOHIDESEL | WS_TABSTOP, WS_EX_CLIENTEDGE);
  CreateChildWindow(&label2_, LABEL2_ID, L"Static", ES_CENTER | ES_READONLY, 0);
  CreateChildWindow(&edit2_, EDIT_ID, L"Edit",
                    ES_LEFT | ES_NOHIDESEL | WS_TABSTOP, WS_EX_CLIENTEDGE);
  CreateChildWindow(&button_, BUTTON_ID, L"Button", BS_CENTER | WS_TABSTOP, 0);

  CreateChildWindow(&label3_, LABEL3_ID, L"Static", ES_CENTER | ES_READONLY, 0);
  CreateChildWindow(&edit3_, EDIT_ID, L"Edit",
                    ES_LEFT | ES_NOHIDESEL | WS_TABSTOP, WS_EX_CLIENTEDGE);

  CreateChildWindow(&label4_, LABEL4_ID, L"Static", ES_CENTER | ES_READONLY, 0);
  CreateChildWindow(&edit4_, EDIT_ID, L"Edit",
                    ES_LEFT | ES_NOHIDESEL | WS_TABSTOP, WS_EX_CLIENTEDGE);

  CreateChildWindow(&listbox_, LISTBOX_ID, L"ListBox",
                    LBS_HASSTRINGS | LBS_NOTIFY, WS_EX_CLIENTEDGE);

  ::SetWindowTextA(edit1_, "192.168.1.147" /*server_.c_str()*/);
  ::SetWindowTextA(edit2_, port_.c_str());
  ::SetWindowTextA(edit3_, "192.168.1.32" /*server_.c_str()*/);
  ::SetWindowTextA(edit4_, "p2p");
}

void MainWnd::LayoutConnectUI(bool show) {
  struct Windows {
    HWND wnd;
    const wchar_t* text;
    size_t width;
    size_t height;
  } windows[] = {
      {label1_, L"Server"},  {edit1_, L"XXXyyyYYYgggXXXyyyYYYggg"},
      {label2_, L":"},       {edit2_, L"XyXyX"},
      {label3_, L"  Local:"}, {edit3_, L"XyzXyzXXyzXyzXXyzXyzX"},
      {label4_, L"  Type:"},  {edit4_, L"XyzXyzX"},
      {button_, L"Connect"},
  };

  if (show) {
    const size_t kSeparator = 5;
    size_t total_width = (ARRAYSIZE(windows) - 1) * kSeparator;

    for (size_t i = 0; i < ARRAYSIZE(windows); ++i) {
      CalculateWindowSizeForText(windows[i].wnd, windows[i].text,
                                 &windows[i].width, &windows[i].height);
      total_width += windows[i].width;
    }

    RECT rc;
    ::GetClientRect(wnd_, &rc);
    size_t x = (rc.right / 2) - (total_width / 2);
    size_t y = rc.bottom / 2;
    for (size_t i = 0; i < ARRAYSIZE(windows); ++i) {
      size_t top = y - (windows[i].height / 2);
      ::MoveWindow(windows[i].wnd, static_cast<int>(x), static_cast<int>(top),
                   static_cast<int>(windows[i].width),
                   static_cast<int>(windows[i].height), TRUE);
      x += kSeparator + windows[i].width;
      if (windows[i].text[0] != 'X')
        ::SetWindowTextW(windows[i].wnd, windows[i].text);
      ::ShowWindow(windows[i].wnd, SW_SHOWNA);
    }
  } else {
    for (size_t i = 0; i < ARRAYSIZE(windows); ++i) {
      ::ShowWindow(windows[i].wnd, SW_HIDE);
    }
  }
}

void MainWnd::LayoutPeerListUI(bool show) {
  if (show) {
    RECT rc;
    ::GetClientRect(wnd_, &rc);
    ::MoveWindow(listbox_, 0, 0, rc.right, rc.bottom, TRUE);
    ::ShowWindow(listbox_, SW_SHOWNA);
  } else {
    ::ShowWindow(listbox_, SW_HIDE);
    InvalidateRect(wnd_, NULL, TRUE);
  }
}

void MainWnd::HandleTabbing() {
  bool shift = ((::GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
  UINT next_cmd = shift ? GW_HWNDPREV : GW_HWNDNEXT;
  UINT loop_around_cmd = shift ? GW_HWNDLAST : GW_HWNDFIRST;
  HWND focus = GetFocus(), next;
  do {
    next = ::GetWindow(focus, next_cmd);
    if (IsWindowVisible(next) &&
        (GetWindowLong(next, GWL_STYLE) & WS_TABSTOP)) {
      break;
    }

    if (!next) {
      next = ::GetWindow(focus, loop_around_cmd);
      if (IsWindowVisible(next) &&
          (GetWindowLong(next, GWL_STYLE) & WS_TABSTOP)) {
        break;
      }
    }
    focus = next;
  } while (true);
  ::SetFocus(next);
}

//
// MainWnd::VideoRenderer
//

MainWnd::VideoRenderer::VideoRenderer(
    HWND wnd,
    int width,
    int height,
    webrtc::VideoTrackInterface* track_to_render)
    : wnd_(wnd), rendered_track_(track_to_render) {
  ::InitializeCriticalSection(&buffer_lock_);
  ZeroMemory(&bmi_, sizeof(bmi_));
  bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi_.bmiHeader.biPlanes = 1;
  bmi_.bmiHeader.biBitCount = 32;
  bmi_.bmiHeader.biCompression = BI_RGB;
  bmi_.bmiHeader.biWidth = width;
  bmi_.bmiHeader.biHeight = -height;
  bmi_.bmiHeader.biSizeImage =
      width * height * (bmi_.bmiHeader.biBitCount >> 3);
  rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
}

MainWnd::VideoRenderer::~VideoRenderer() {
  rendered_track_->RemoveSink(this);
  ::DeleteCriticalSection(&buffer_lock_);
}

void MainWnd::VideoRenderer::SetSize(int width, int height) {
  AutoLock<VideoRenderer> lock(this);

  if (width == bmi_.bmiHeader.biWidth && height == bmi_.bmiHeader.biHeight) {
    return;
  }

  bmi_.bmiHeader.biWidth = width;
  bmi_.bmiHeader.biHeight = -height;
  bmi_.bmiHeader.biSizeImage =
      width * height * (bmi_.bmiHeader.biBitCount >> 3);
  image_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);
}

void MainWnd::VideoRenderer::OnFrame(const webrtc::VideoFrame& video_frame) {
  {
    AutoLock<VideoRenderer> lock(this);

    rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(
        video_frame.video_frame_buffer()->ToI420());
    if (video_frame.rotation() != webrtc::kVideoRotation_0) {
      buffer = webrtc::I420Buffer::Rotate(*buffer, video_frame.rotation());
    }

    SetSize(buffer->width(), buffer->height());

    RTC_DCHECK(image_.get() != NULL);
    libyuv::I420ToARGB(buffer->DataY(), buffer->StrideY(), buffer->DataU(),
                       buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
                       image_.get(),
                       bmi_.bmiHeader.biWidth * bmi_.bmiHeader.biBitCount / 8,
                       buffer->width(), buffer->height());
  }
  InvalidateRect(wnd_, NULL, TRUE);
}
