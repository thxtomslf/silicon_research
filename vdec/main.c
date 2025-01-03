#include "main.h"
#include "recorder.h"
#include <stdint.h>

#define earthRadiusKm 6371.0
typedef struct hiHDMI_ARGS_S {
  HI_HDMI_ID_E enHdmi;
} HDMI_ARGS_S;

HDMI_ARGS_S g_stHdmiArgs;
HI_HDMI_CALLBACK_FUNC_S g_stCallbackFunc;

static HI_VOID HDMI_HotPlug_Proc(HI_VOID* pPrivateData) {
  HDMI_ARGS_S stArgs;
  HI_S32 s32Ret = HI_FAILURE;
  HI_HDMI_ATTR_S stAttr;
  HI_HDMI_SINK_CAPABILITY_S stCaps;

  memset(&stAttr, 0, sizeof(HI_HDMI_ATTR_S));
  memset(&stArgs, 0, sizeof(HDMI_ARGS_S));
  memcpy(&stArgs, pPrivateData, sizeof(HDMI_ARGS_S));
  memset(&stCaps, 0, sizeof(HI_HDMI_SINK_CAPABILITY_S));

  s32Ret = HI_MPI_HDMI_GetSinkCapability(stArgs.enHdmi, &stCaps);
  if (s32Ret != HI_SUCCESS) {
    printf("get sink caps failed!\n");
  } else {
    printf("get sink caps success!\n");
    printf("Video latency: %d\n", stCaps.u8Video_Latency);
    printf("Native format = %d\n", stCaps.enNativeVideoFormat);

    for (int i = 0; i < 49; i++) {
      printf("FMT #%d = %s\n", i, stCaps.bVideoFmtSupported[i] ? "YES" : "-");
    }
  }

  HI_MPI_HDMI_GetAttr(stArgs.enHdmi, &stAttr);
  HI_MPI_HDMI_SetAttr(stArgs.enHdmi, &stAttr);
  HI_MPI_HDMI_Start(stArgs.enHdmi);

  return;
}

double deg2rad(double deg) {
  return (deg * M_PI / 180);
}

double distanceEarth(double lat1d, double lon1d, double lat2d, double lon2d) {
  double lat1r, lon1r, lat2r, lon2r, u, v;
  lat1r = deg2rad(lat1d);
  lon1r = deg2rad(lon1d);
  lat2r = deg2rad(lat2d);
  lon2r = deg2rad(lon2d);
  u = sin((lat2r - lat1r) / 2);
  v = sin((lon2r - lon1r) / 2);

  return 2.0 * earthRadiusKm * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
}

size_t numOfChars(const char s[]) {
  size_t n = 0;
  while (s[n] != '\0') {
    ++n;
  }

  return n;
}

char* insertString(char s1[], const char s2[], size_t pos) {
  size_t n1 = numOfChars(s1);
  size_t n2 = numOfChars(s2);
  if (n1 < pos) {
    pos = n1;
  }

  for (size_t i = 0; i < n1 - pos; i++) {
    s1[n1 + n2 - i - 1] = s1[n1 - i - 1];
  }

  for (size_t i = 0; i < n2; i++) {
    s1[pos + i] = s2[i];
  }

  s1[n1 + n2] = '\0';

  return s1;
}

static HI_VOID HDMI_UnPlug_Proc(HI_VOID* pPrivateData) {
  HDMI_ARGS_S stArgs;
  memset(&stArgs, 0, sizeof(HDMI_ARGS_S));
  memcpy(&stArgs, pPrivateData, sizeof(HDMI_ARGS_S));
  HI_MPI_HDMI_Stop(stArgs.enHdmi);

  return;
}

static HI_VOID HDMI_EventCallBack(
  HI_HDMI_EVENT_TYPE_E event, HI_VOID* pPrivateData) {

  switch (event) {
    case HI_HDMI_EVENT_HOTPLUG:
      HDMI_HotPlug_Proc(pPrivateData);
      break;

    case HI_HDMI_EVENT_NO_PLUG:
      HDMI_UnPlug_Proc(pPrivateData);
      break;
  }

  return;
}

void printHelp() {
  printf(
    "\n\t\tOpenIPC FPV Decoder for HI3536 (%s)\n"
    "\n"
    "  Usage:\n"
    "    vdec [Arguments]\n"
    "\n"
    "  Arguments:\n"
    "    -p [Port]      - Listen port                       (Default: 5600)\n"
    "    -c [Codec]     - Video codec                       (Default: h264)\n"
    "      h264           - H264\n"
    "      h265           - H265\n"
    "\n"
    "    -d [Format]    - Data format                       (Default: stream)\n"
    "      stream         - Incoming data is stream\n"
    "      frame          - Incoming data is frame\n"
    "\n"
    "    -t [Format]    - OSD Type                          (Default: normal)\n"
    "      normal         - Regular OSD\n"
    "      minimal        - Minimal OSD\n"
    "\n"
    "    -m [Mode]        - Screen output mode              (Default: 720p60)\n"
    "      720p60         - 1280 x 720    @ 60 fps\n"
    "      1080p60        - 1920 x 1080   @ 60 fps\n"
    "      1024x768x60    - 1024 x 768    @ 60 fps\n"
    "      1366x768x60    - 1366 x 768    @ 60 fps\n"
    "      1280x1024x60   - 1280 x 1024   @ 60 fps\n"
    "      1600x1200x60   - 1600 x 1200   @ 60 fps\n"
    "      2560x1440x30   - 2560 x 1440   @ 30 fps\n"
    "\n"
    "    -w [Path]        - DVR feature: saving video to file extention h265 (tested with SDcard reader)\n"
    "      Example        -w /mnt/sda1/recorder/video1.h265\n"
    "\n"
    "    --ar [mode]      - Aspect ratio mode               (Default: keep)\n"
    "      keep             - Keep stream aspect ratio\n"
    "      stretch          - Stretch to output resolution\n"
    "      manual           - Manual image size definition\n"
    "\n"
    "    --ar-bg-r [Value]  - Fill color red component      (Default: 0)\n"
    "    --ar-bg-g [Value]  - Fill color green component    (Default: 0)\n"
    "    --ar-bg-b [Value]  - Fill color blue component     (Default: 0)\n"
    "\n"
    "    --ar-x [Value]     - Image position X\n"
    "    --ar-y [Value]     - Image position Y\n"
    "    --ar-w [Value]     - Image width\n"
    "    --ar-h [Value]     - Image height\n"
    "\n"
    "    --osd                  - Enable OSD\n"
    "    --mavlink-port [port]  - MavLink Rx port           (Default: 14550)\n"
    "    --bg-r [Value]         - Background color red      (Default: 0)\n"
    "    --bg-g [Value]         - Background color green    (Default: 96)\n"
    "    --bg-b [Value]         - Background color blue     (Default: 0)\n"
    "\n", __DATE__
  );
}

extern uint32_t frames_received;
uint32_t stats_rx_bytes = 0;
struct timespec last_timestamp = {0, 0};

double getTimeInterval(struct timespec* timestamp, struct timespec* last_meansure_timestamp) {
  return (timestamp->tv_sec - last_meansure_timestamp->tv_sec) +
       (timestamp->tv_nsec - last_meansure_timestamp->tv_nsec) / 1000000000.;
}

int crsf_port = -1;  // Порт для CRSF, -1 если не используется
const uint8_t CRSF_BATTERY_SENSOR = 0x08;
const uint8_t CRSF_ATTITUDE_TYPE = 0x1E;
const uint8_t CRSF_FRAMETYPE_FLIGHT_MODE = 0x21;


float crsf_battery_voltage = 0.0;
float crsf_battery_current = 0.0;
float crsf_pitch = 0.0;
float crsf_roll = 0.0;
float crsf_yaw = 0.0;
char crsf_flight_mode[256] = "";

uint8_t crc8_dvb_s2(uint8_t crc, uint8_t a) {
    crc ^= a;
    for (int i = 0; i < 8; i++) {
        if (crc & 0x80) {
            crc = (crc << 1) ^ 0xD5;
        } else {
            crc <<= 1;
        }
    }
    return crc & 0xFF;
}

uint8_t crc8_data(const uint8_t* data, size_t length) {
    uint8_t crc = 0;
    for (size_t i = 0; i < length; i++) {
        crc = crc8_dvb_s2(crc, data[i]);
    }
    return crc;
}

int crsf_validate_frame(const uint8_t* frame, size_t length) {
    if (length < 4) {
        return 0; // Недостаточная длина
    }
    uint8_t calculated_crc = crc8_data(frame + 2, length - 3);
    uint8_t received_crc = frame[length - 1];
    return calculated_crc == received_crc;
}

void handleCrsfPacket(uint8_t ptype, const uint8_t* data, size_t length) {
    if (ptype == CRSF_BATTERY_SENSOR) {
        crsf_battery_voltage = (data[3] << 8 | data[4]) / 10.0;
        crsf_battery_current = (data[5] << 8 | data[6]) / 10.0;
    } else if (ptype == CRSF_ATTITUDE_TYPE) {
        crsf_pitch = (int16_t)(data[3] << 8 | data[4]) / 10000.0;
        crsf_roll = (int16_t)(data[5] << 8 | data[6]) / 10000.0;
        crsf_yaw = (int16_t)(data[7] << 8 | data[8]) / 10000.0;
    } else if (ptype == CRSF_FRAMETYPE_FLIGHT_MODE) {
        int mode_length = data[1] - 3;
        if (mode_length > 0 && mode_length < 256) {
            memcpy(crsf_flight_mode, &data[3], mode_length);
            crsf_flight_mode[mode_length] = '\0'; // Завершение строки
        } else {
            strcpy(crsf_flight_mode, "Unknown");
        }
    }
}

uint16_t osd_element1x = 0;
uint16_t osd_element1y = 0;
uint16_t osd_element2x = 0;
uint16_t osd_element2y = 0;
uint16_t osd_element3x = 0;
uint16_t osd_element3y = 0;
uint16_t osd_element4x = 0;
uint16_t osd_element4y = 0;
uint16_t osd_element5x = 0;
uint16_t osd_element5y = 0;
uint16_t osd_element6x = 0;
uint16_t osd_element6y = 0;
uint16_t osd_element7x = 0;
uint16_t osd_element7y = 0;
uint16_t osd_element8x = 0;
uint16_t osd_element8y = 0;
uint16_t osd_element9x = 0;
uint16_t osd_element9y = 0;
uint16_t osd_element10x = 0;
uint16_t osd_element10y = 0;
uint16_t osd_element11x = 0;
uint16_t osd_element11y = 0;
uint16_t osd_element12x = 0;
uint16_t osd_element12y = 0;
uint16_t osd_element13x = 0;
uint16_t osd_element13y = 0;
uint16_t osd_element14x = 0;
uint16_t osd_element14y = 0;
uint16_t osd_element15x = 0;
uint16_t osd_element15y = 0;
uint16_t osd_element16x = 0;
uint16_t osd_element16y = 0;
uint16_t osd_element17x = 0;
uint16_t osd_element17y = 0;
uint16_t osd_element18x = 0;
uint16_t osd_element18y = 0;
uint16_t osd_element19x = 0;
uint16_t osd_element19y = 0;
uint16_t mavlink_port = 14550;
uint32_t vo_width = 1280;
uint32_t vo_height = 720;

int main(int argc, const char* argv[]) {
  VO_INTF_SYNC_E vo_mode = VO_OUTPUT_720P60;
  uint32_t vo_framerate = 60;

  VDEC_CHN vdec_channel_id = 0;
  int ret = 0;

  VPSS_GRP vpss_group_id = 0;
  VPSS_CHN vpss_channel_id = 0;
  VO_DEV vo_device_id = 0;
  VO_LAYER vo_layer_id = 0;
  VO_CHN vo_channel_id = 0;

  uint16_t listen_port = 5600;
  uint32_t background_color = 0x006000;

  const char* write_stream_path = 0;
  int enable_osd = 0;
  int codec_mode_stream = 1;
  PAYLOAD_TYPE_E codec_id = PT_H264;

  ASPECT_RATIO_E vo_layer_aspect_ratio = ASPECT_RATIO_AUTO;
  uint32_t vo_layer_fill_color = 0;
  RECT_S vo_layer_aspect_ratio_rect;
  memset(&vo_layer_aspect_ratio_rect, 0x00, sizeof(vo_layer_aspect_ratio_rect));

  // Load console arguments
  __BeginParseConsoleArguments__(printHelp) __OnArgument("-p") {
    listen_port = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-c") {
    const char* codec = __ArgValue;
    if (!strcmp(codec, "h264")) {
      codec_id = PT_H264;
    } else if (!strcmp(codec, "h265")) {
      codec_id = PT_H265;
    } else {
      printf("> ERROR: Unsupported video codec [%s]\n", codec);
    }
    continue;
  }

  __OnArgument("-d") {
    const char* format = __ArgValue;
    if (!strcmp(format, "stream")) {
      codec_mode_stream = 1;
    } else if (!strcmp(format, "frame")) {
      codec_mode_stream = 0;
    } else {
      printf("> ERROR: Unsupported data format [%s]\n", format);
    }
    continue;
  }

  __OnArgument("-m") {
    const char* mode = __ArgValue;
    if (!strcmp(mode, "720p60")) {
      vo_mode = VO_OUTPUT_720P60;
      vo_width = 1280;
      vo_height = 720;
      vo_framerate = 60;
    } else if (!strcmp(mode, "1080p60")) {
      vo_mode = VO_OUTPUT_1080P60;
      vo_width = 1920;
      vo_height = 1080;
      vo_framerate = 60;
    } else if (!strcmp(mode, "1024x768x60")) {
      vo_mode = VO_OUTPUT_1024x768_60;
      vo_width = 1024;
      vo_height = 768;
      vo_framerate = 60;
    } else if (!strcmp(mode, "1366x768x60")) {
      vo_mode = VO_OUTPUT_1366x768_60;
      vo_width = 1024;
      vo_height = 768;
      vo_framerate = 60;
    } else if (!strcmp(mode, "1280x1024x60")) {
      vo_mode = VO_OUTPUT_1280x1024_60;
      vo_width = 1280;
      vo_height = 1024;
      vo_framerate = 60;
    } else if (!strcmp(mode, "1600x1200x60")) {
      vo_mode = VO_OUTPUT_1600x1200_60;
      vo_width = 1600;
      vo_height = 1200;
      vo_framerate = 60;
    } else if (!strcmp(mode, "2560x1440x30")) {
      vo_mode = VO_OUTPUT_2560x1440_30;
      vo_width = 2560;
      vo_height = 1440;
      vo_framerate = 30;
    } else {
      printf("> ERROR: Unsupported video mode [%s]\n", mode);
    }
    continue;
  }

  __OnArgument("--bg-r") {
    uint8_t v = atoi(__ArgValue);
    background_color &= ~(0xFF << 16);
    background_color |= (v << 16);
    continue;
  }

  __OnArgument("--bg-g") {
    uint8_t v = atoi(__ArgValue);
    background_color &= ~(0xFF << 8);
    background_color |= (v << 8);
    continue;
  }

  __OnArgument("--bg-b") {
    uint8_t v = atoi(__ArgValue);
    background_color &= ~(0xFF);
    background_color |= (v);
    continue;
  }

  __OnArgument("-w") {
    write_stream_path = __ArgValue;
    continue;
  }

  __OnArgument("--osd") {
    enable_osd = 1;
    continue;
  }

  __OnArgument("--mavlink-port") {
    mavlink_port = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele1x") {
    osd_element1x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele1y") {
    osd_element1y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele2x") {
    osd_element2x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele2y") {
    osd_element2y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele3x") {
    osd_element3x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele3y") {
    osd_element3y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele4x") {
    osd_element4x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele4y") {
    osd_element4y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele5x") {
    osd_element5x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele5y") {
    osd_element5y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele6x") {
    osd_element6x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele6y") {
    osd_element6y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele7x") {
    osd_element7x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele7y") {
    osd_element7y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele8x") {
    osd_element8x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele8y") {
    osd_element8y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele9x") {
    osd_element9x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele9y") {
    osd_element9y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele10x") {
    osd_element10x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele10y") {
    osd_element10y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele11x") {
    osd_element11x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele11y") {
    osd_element11y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele12x") {
    osd_element12x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele12y") {
    osd_element12y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele13x") {
    osd_element13x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele13y") {
    osd_element13y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele14x") {
    osd_element14x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele14y") {
    osd_element14y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele15x") {
    osd_element15x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele15y") {
    osd_element15y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele16x") {
    osd_element16x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele16y") {
    osd_element16y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele17x") {
    osd_element17x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele17y") {
    osd_element17y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele18x") {
    osd_element18x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele18y") {
    osd_element18y = atoi(__ArgValue);
    continue;
  }

  __OnArgument("-osd_ele19x") {
    osd_element19x = atoi(__ArgValue);
    continue;
  }
  __OnArgument("-osd_ele19y") {
    osd_element19y = atoi(__ArgValue);
    continue;
  }


__OnArgument("--crsf") {
    crsf_port = atoi(__ArgValue);
    continue;
}

  __OnArgument("--ar") {
    const char* mode = __ArgValue;
    if (!strcmp(mode, "keep")) {
      vo_layer_aspect_ratio = ASPECT_RATIO_AUTO;
    } else if (!strcmp(mode, "stretch")) {
      vo_layer_aspect_ratio = ASPECT_RATIO_NONE;
    } else if (!strcmp(mode, "manual")) {
      vo_layer_aspect_ratio = ASPECT_RATIO_MANUAL;
    } else {
      printf("> ERROR: Unsupported aspect ratio mode [%s]\n", mode);
    }
    continue;
  }

  __OnArgument("--ar-bg-r") {
    uint8_t v = atoi(__ArgValue);
    vo_layer_fill_color &= ~(0xFF << 16);
    vo_layer_fill_color |= (v << 16);
    continue;
  }

  __OnArgument("--ar-bg-g") {
    uint8_t v = atoi(__ArgValue);
    vo_layer_fill_color &= ~(0xFF << 8);
    vo_layer_fill_color |= (v << 8);
    continue;
  }

  __OnArgument("--ar-bg-b") {
    uint8_t v = atoi(__ArgValue);
    vo_layer_fill_color &= ~(0xFF);
    vo_layer_fill_color |= (v);
    continue;
  }

  __OnArgument("--ar-x") {
    vo_layer_aspect_ratio_rect.s32X = ALIGN_UP(atoi(__ArgValue), 2);
    continue;
  }

  __OnArgument("--ar-y") {
    vo_layer_aspect_ratio_rect.s32Y = ALIGN_UP(atoi(__ArgValue), 2);
    continue;
  }

  __OnArgument("--ar-w") {
    vo_layer_aspect_ratio_rect.u32Width = ALIGN_UP(atoi(__ArgValue), 2);
    continue;
  }

  __OnArgument("--ar-h") {
    vo_layer_aspect_ratio_rect.u32Height = ALIGN_UP(atoi(__ArgValue), 2);
    continue;
  }

  __EndParseConsoleArguments__

  // Calculate maximum video settings
  uint32_t vdec_max_width = ALIGN_UP(2592, DEFAULT_ALIGN);
  uint32_t vdec_max_height = ALIGN_UP(1944, DEFAULT_ALIGN);

  uint32_t vo_layer_max_width = MIN2(1920, vo_width);
  uint32_t vo_layer_max_height = MIN2(1200, vo_height);

  // Reset previous configuration
  ret = HI_MPI_SYS_Exit();

  for (uint32_t i = 0; i < VB_MAX_USER; i++) {
    ret = HI_MPI_VB_ExitModCommPool(i);
  }

  for (uint32_t i = 0; i < VB_MAX_POOLS; i++) {
    ret = HI_MPI_VB_DestroyPool(i);
  }

  ret = HI_MPI_VB_Exit();

  // Setup memory pools and initialize system
  VB_CONF_S vb_conf;
  memset(&vb_conf, 0x00, sizeof(vb_conf));

  // Set maximum memory pools count
  vb_conf.u32MaxPoolCnt = 16;

  // Configure video buffer
  ret = HI_MPI_VB_SetConf(&vb_conf);
  if (ret) {
    printf("ERROR: Configure VB failed : 0x%x\n", ret);
  }

  // Initilize video buffer
  ret = HI_MPI_VB_Init();
  if (ret) {
    printf("ERROR: Init VB failed : 0x%x\n", ret);
  }

  // Configure system
  MPP_SYS_CONF_S sys_config;
  sys_config.u32AlignWidth = 16;
  ret = HI_MPI_SYS_SetConf(&sys_config);
  if (ret) {
    printf("ERROR: Init VB failed : 0x%x\n", ret);
  }

  // Initialize system
  ret = HI_MPI_SYS_Init();
  if (ret) {
    printf("ERROR: Init SYS failed : 0x%x\n", ret);
    HI_MPI_VB_Exit();
  }

  MPP_VERSION_S version;
  HI_MPI_SYS_GetVersion(&version);
  printf("[%s]\n", version.aVersion);

  // Release previous pool configuration
  ret = HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to release VDEC memory pool\n");
    return 1;
  }

  // Configure video buffer for decoder
  memset(&vb_conf, 0x00, sizeof(vb_conf));
  vb_conf.u32MaxPoolCnt = 2;
  vb_conf.astCommPool[0].u32BlkCnt = 4;
  vb_conf.astCommPool[0].u32BlkSize = 0;
  vb_conf.astCommPool[1].u32BlkCnt = 2;
  vb_conf.astCommPool[1].u32BlkSize = 0;

  // Calculate required size for video buffer
  VB_PIC_BLK_SIZE(vdec_max_width, vdec_max_height, codec_id, vb_conf.astCommPool[0].u32BlkSize);
  VB_PMV_BLK_SIZE(vdec_max_width, vdec_max_height, codec_id, vb_conf.astCommPool[1].u32BlkSize);
  printf("> VDEC picture block size = %d, %d\n",
    vb_conf.astCommPool[0].u32BlkSize, vb_conf.astCommPool[1].u32BlkSize);

  ret = HI_MPI_VB_SetModPoolConf(VB_UID_VDEC, &vb_conf);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to configure ModComPool\n");
    return 1;
  }

  ret = HI_MPI_VB_InitModCommPool(VB_UID_VDEC);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to init ModComPool = 0x%x\n", ret);
    return 1;
  }

  ret = HI_MPI_VO_SetDispBufLen(vo_layer_id, 2);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to set display buffer length\n");
    return 1;
  }

  // Set partitioning mode
  HI_MPI_VO_SetVideoLayerPartitionMode(vo_layer_id, VO_PART_MODE_SINGLE);

  // Initialize VO
  VO_init(vo_device_id, VO_INTF_HDMI | VO_INTF_VGA, vo_mode, vo_framerate, background_color);

  // Initialize HDMI
  VO_HDMI_init(0, vo_mode);

  ret = HI_MPI_VO_GetDevFrameRate(vo_device_id, &vo_framerate);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to set VO framerate = 0x%x\n");
    return ret;
  }

  printf("> VO: %dx%d @ %d\n", vo_width, vo_height, vo_framerate);

  // Configure video layer
  VO_VIDEO_LAYER_ATTR_S layer_config;
  memset(&layer_config, 0x00, sizeof(layer_config));
  ret = HI_MPI_VO_GetVideoLayerAttr(vo_layer_id, &layer_config);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to get VO layer information\n");
    return 1;
  }

  layer_config.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
  layer_config.bDoubleFrame = HI_FALSE;
  layer_config.bClusterMode = HI_FALSE;
  layer_config.u32DispFrmRt = vo_framerate;

  layer_config.stDispRect.u32Width = vo_layer_max_width;
  layer_config.stDispRect.u32Height = vo_layer_max_height;
  layer_config.stDispRect.s32X = 0;
  layer_config.stDispRect.s32Y = 0;

  layer_config.stImageSize.u32Width = vo_layer_max_width;
  layer_config.stImageSize.u32Height = vo_layer_max_height;

  ret = HI_MPI_VO_SetVideoLayerAttr(vo_layer_id, &layer_config);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to configure video layer = 0x%x\n", ret);
    return 1;
  }

  // Enable video layer
  ret = HI_MPI_VO_EnableVideoLayer(vo_layer_id);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to enable video layer = 0x%x\n", ret);
    return 1;
  }

  // Configure video channel
  VO_CHN_ATTR_S channel_config;
  ret = HI_MPI_VO_GetChnAttr(vo_layer_id, vo_channel_id, &channel_config);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to get channel configuration\n");
    return 1;
  }

  channel_config.bDeflicker = HI_FALSE;
  channel_config.u32Priority = VO_MAX_PRIORITY;
  channel_config.stRect.s32X = 0;
  channel_config.stRect.s32Y = 0;
  channel_config.stRect.u32Width = vo_layer_max_width;
  channel_config.stRect.u32Height = vo_layer_max_height;

  ret = HI_MPI_VO_SetChnAttr(vo_layer_id, vo_channel_id, &channel_config);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to configure channel\n");
    return 1;
  }

  ret = HI_MPI_VO_EnableChn(vo_layer_id, vo_channel_id);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to enable video channel\n");
    return 1;
  }

  // Configure aspect ratio
  VO_CHN_PARAM_S param;
  HI_MPI_VO_GetChnParam(vo_layer_id, vo_channel_id, &param);
  param.stAspectRatio.enMode = vo_layer_aspect_ratio;
  param.stAspectRatio.u32BgColor = vo_layer_fill_color;
  param.stAspectRatio.stVideoRect = vo_layer_aspect_ratio_rect;

  ret = HI_MPI_VO_SetChnParam(vo_layer_id, vo_channel_id, &param);
  if (ret != HI_SUCCESS) {
    printf("> ERROR: Unable to set channel param\n");
  }

  // Start VDEC at 1920x1080 maximum resolution
  VDEC_CHN_ATTR_S config;
  memset(&config, 0x00, sizeof(config));
  config.enType = codec_id;
  config.u32BufSize = vdec_max_width * vdec_max_height * 3 / 2;
  config.u32Priority = 128;
  config.u32PicWidth = vdec_max_width;
  config.u32PicHeight = vdec_max_height;

  config.stVdecVideoAttr.bTemporalMvpEnable = (codec_id == PT_H265) ? HI_TRUE : HI_FALSE;
  config.stVdecVideoAttr.enMode = codec_mode_stream ? VIDEO_MODE_STREAM : VIDEO_MODE_FRAME;
  config.stVdecVideoAttr.u32RefFrameNum = 1;

  // Create VDEC channel
  ret = HI_MPI_VDEC_CreateChn(vdec_channel_id, &config);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to create VDEC channel\n");
    return 1;
  }

  // Set display mode
  ret = HI_MPI_VDEC_SetDisplayMode(vdec_channel_id, VIDEO_DISPLAY_MODE_PREVIEW);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to set VDEC display mode\n");
    return 1;
  }

  // Read decoder protocol information
  VDEC_PRTCL_PARAM_S protocol;
  HI_MPI_VDEC_GetProtocolParam(vdec_channel_id, &protocol);

  switch (protocol.enType) {
    case PT_H264:
      protocol.stH264PrtclParam.s32MaxPpsNum = 256;
      protocol.stH264PrtclParam.s32MaxSpsNum = 32;
      protocol.stH264PrtclParam.s32MaxSliceNum = 100;
      ret = HI_MPI_VDEC_SetProtocolParam(vdec_channel_id, &protocol);
      if (ret != HI_SUCCESS) {
        printf("ERROR: Unable to set VDEC protocol parameters\n");
        return 1;
      }

      HI_MPI_VDEC_GetProtocolParam(vdec_channel_id, &protocol);
      printf("> VDEC Protocol = Type: %s, PPS: %d, SLICE: %d, SPS: %d\n",
        (codec_id == PT_H264 ? "H264" : (codec_id == PT_H265 ? "H265" : "Unknown")),
        protocol.stH264PrtclParam.s32MaxPpsNum,
        protocol.stH264PrtclParam.s32MaxSliceNum,
        protocol.stH264PrtclParam.s32MaxSpsNum);
      break;

    case PT_H265:
      protocol.stH265PrtclParam.s32MaxPpsNum = 64;
      protocol.stH265PrtclParam.s32MaxSpsNum = 16;
      protocol.stH265PrtclParam.s32MaxVpsNum = 16;
      protocol.stH265PrtclParam.s32MaxSliceSegmentNum = 100;
      ret = HI_MPI_VDEC_SetProtocolParam(vdec_channel_id, &protocol);
      if (ret != HI_SUCCESS) {
        printf("ERROR: Unable to set VDEC protocol parameters\n");
        return 1;
      }

      HI_MPI_VDEC_GetProtocolParam(vdec_channel_id, &protocol);
      printf("> VDEC Protocol = Type: %s, PPS: %d, SLICE: %d, SPS: %d\n",
        (codec_id == PT_H264 ? "H264" : (codec_id == PT_H265 ? "H265" : "Unknown")),
        protocol.stH265PrtclParam.s32MaxPpsNum,
        protocol.stH265PrtclParam.s32MaxSliceSegmentNum,
        protocol.stH265PrtclParam.s32MaxSpsNum);
      break;
  }

  // Assemble pipeline
  MPP_CHN_S src;
  MPP_CHN_S dst;

  src.enModId = HI_ID_VDEC;
  src.s32DevId = 0;
  src.s32ChnId = vdec_channel_id;

  dst.enModId = HI_ID_VOU;
  dst.s32DevId = vo_layer_id;
  dst.s32ChnId = vo_channel_id;

  ret = HI_MPI_SYS_Bind(&src, &dst);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to bind VDEC -> VO\n");
    return 1;
  }

  // Start VDEC
  ret = HI_MPI_VDEC_StartRecvStream(vdec_channel_id);
  if (ret != HI_SUCCESS) {
    printf("ERROR: Unable to start VDEC channel\n");
    return 1;
  }

  // Create socket
  int port = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  struct sockaddr_in address;
  memset(&address, 0x00, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(listen_port);
  bind(port, (struct sockaddr*)&address, sizeof(struct sockaddr_in));

  if (fcntl(port, F_SETFL, O_NONBLOCK) == -1) {
    printf("ERROR: Unable to set non-blocking mode\n");
    return 1;
  }

  uint8_t* rx_buffer = malloc(1024 * 1024);
  uint8_t* nal_buffer = malloc(1024 * 1024);

  // Open write file
  if (codec_id == PT_H265 && write_stream_path) {
    recorder_int(write_stream_path);
  }

  // Start ISP service thread
  pthread_t osd_thread, crsf_thread;
  if (enable_osd) {
    if (crsf_port != -1) {
    // Запуск потоков CRSF
    pthread_create(&crsf_thread, NULL, crsfThread, 0);
    pthread_create(&osd_thread, NULL, crsfOsdThread, 0);
  } else {
    // Запуск стандартных потоков OSD и MAVLink
    pthread_create(&osd_thread, NULL, __OSD_THREAD__, 0);
    pthread_create(&osd_thread, NULL, __MAVLINK_THREAD__, 0);
  }
  }

  uint32_t write_buffer_capacity = 1024 * 1024 * 2;
  uint8_t* write_buffer = malloc(write_buffer_capacity);
  uint32_t write_buffer_size = 0;

  while (1) {
    int rx = recv(port, rx_buffer+8, 4096, 0);
    if (rx <= 0) {
      usleep(1);
      continue;
    }

    VDEC_STREAM_S stream;
    memset(&stream, 0x00, sizeof(stream));
    stream.bEndOfStream = HI_FALSE;
    stream.bEndOfFrame = codec_mode_stream ? HI_FALSE : HI_TRUE;

    uint32_t rtp_header = 0;
    if (rx_buffer[8] & 0x80 && rx_buffer[9] & 0x60) {
      rtp_header = 12;
    }

    // Decode UDP stream
    stream.pu8Addr = decode_frame(rx_buffer + 8, rx,
      rtp_header, nal_buffer, &stream.u32Len);
    if (!stream.pu8Addr) {
      continue;
    }

    if (stream.u32Len < 5) {
      printf("> Broken frame\n");
    }

    stats_rx_bytes += stream.u32Len;

    recorder_input_data(&stream);

    // Send frame into decoder
    int ret = HI_MPI_VDEC_SendStream(vdec_channel_id, &stream, 0);
    if (ret != HI_SUCCESS) {
      printf("WARN: Unable to send data into VDEC = 0x%x\n", ret);
    }
  }

  return 0;
}

float telemetry_altitude = 0;
float telemetry_pitch = 0;
float telemetry_roll = 0;
float telemetry_yaw = 0;
float telemetry_battery = 0;
float telemetry_current = 0;
float telemetry_current_consumed = 0;
double telemetry_lat = 0;
double telemetry_lon = 0;
double telemetry_lat_base = 0;
double telemetry_lon_base = 0;
double telemetry_hdg = 0;
double telemetry_distance = 0;
double s1_double = 0;
double s2_double = 0;
double s3_double = 0;
double s4_double = 0;
float telemetry_sats = 0;
float telemetry_gspeed = 0;
float telemetry_vspeed = 0;
float telemetry_rssi = 0;
float telemetry_throttle = 0;
float telemetry_raw_imu = 0;
float telemetry_resolution = 0;
float telemetry_arm = 0;
float armed = 0;
char c1[30] = "0";
char c2[30] = "0";
char s1[30] = "0";
char s2[30] = "0";
char s3[30] = "0";
char s4[30] = "0";
char* ptr;

void* __MAVLINK_THREAD__(void* arg) {
  // Create socket
  int fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    printf("ERROR: Unable to create MavLink socket: %s\n", strerror(errno));
    return 0;
  }

  // Bind port
  struct sockaddr_in addr = {};
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  inet_pton(AF_INET, "0.0.0.0", &(addr.sin_addr));
  addr.sin_port = htons(mavlink_port);

  if (bind(fd, (struct sockaddr*)(&addr), sizeof(addr)) != 0) {
    printf("ERROR: Unable to bind MavLink port: %s\n", strerror(errno));
    return 0;
  }

  // Set Rx timeout
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
    printf("ERROR: Unable to bind MavLink rx timeout: %s\n", strerror(errno));
    return 0;
  }

  char buffer[2048];
  while (1) {
    memset(buffer, 0x00, sizeof(buffer));
    int ret = recv(fd, buffer, sizeof(buffer), 0);
    if (ret < 0) {
      continue;
    } else if (ret == 0) {
      // peer has done an orderly shutdown
      return 0;
    }

    // Parse
    mavlink_message_t message;
    mavlink_status_t status;
    for (int i = 0; i < ret; ++i) {
      if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &message, &status) == 1) {
        switch (message.msgid) {
          case MAVLINK_MSG_ID_HEARTBEAT:
            // handle_heartbeat(&message);
            break;
          
          case MAVLINK_MSG_ID_RAW_IMU:
            {
              mavlink_raw_imu_t imu;
              mavlink_msg_raw_imu_decode(&message, &imu);
              telemetry_raw_imu = imu.temperature;
            }
            break;
          
          case MAVLINK_MSG_ID_SYS_STATUS:
            mavlink_sys_status_t bat;
            mavlink_msg_sys_status_decode(&message, &bat);
            telemetry_battery = bat.voltage_battery;
            telemetry_current = bat.current_battery;
            break;

          case MAVLINK_MSG_ID_BATTERY_STATUS:
            mavlink_battery_status_t batt;
            mavlink_msg_battery_status_decode(&message, &batt);
            telemetry_current_consumed = batt.current_consumed;
            break;

          case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
            mavlink_rc_channels_raw_t rc_channels_raw;
            mavlink_msg_rc_channels_raw_decode( &message, &rc_channels_raw);
            telemetry_rssi = rc_channels_raw.rssi;
            telemetry_throttle = (rc_channels_raw.chan4_raw - 1000) / 10;

            if (telemetry_throttle < 0) {
              telemetry_throttle = 0;
            }
            telemetry_arm = rc_channels_raw.chan5_raw;
            telemetry_resolution = rc_channels_raw.chan8_raw;
            if (telemetry_resolution > 1700) {
              system("/root/resolution.sh");
            }
            break;

          case MAVLINK_MSG_ID_GPS_RAW_INT:
            mavlink_gps_raw_int_t gps;
            mavlink_msg_gps_raw_int_decode(&message, &gps);
            telemetry_sats = gps.satellites_visible;
            telemetry_lat = gps.lat;
            telemetry_lon = gps.lon;
            if (telemetry_arm > 1700) {
              if (armed < 1) {
                armed = 1;
                telemetry_lat_base = telemetry_lat;
                telemetry_lon_base = telemetry_lon;
              }

              sprintf(s1, "%.00f", telemetry_lat);
              if (telemetry_lat < 10000000) {
                insertString(s1, "0.", 0);
              }
              if (telemetry_lat > 9999999) {
                if (numOfChars(s1) == 8) {
                  insertString(s1, ".", 1);
                } else {
                  insertString(s1, ".", 2);
                }
              }

              sprintf(s2, "%.00f", telemetry_lon);
              if (telemetry_lon < 10000000) {
                insertString(s2, "0.", 0);
              }
              if (telemetry_lon > 9999999) {
                if (numOfChars(s2) == 8) {
                  insertString(s2, ".", 1);
                } else {
                  insertString(s2, ".", 2);
                }
              }

              sprintf(s3, "%.00f", telemetry_lat_base);
              if (telemetry_lat_base < 10000000) {
                insertString(s3, "0.", 0);
              }
              if (telemetry_lat_base > 9999999) {
                if (numOfChars(s3) == 8) {
                  insertString(s3, ".", 1);
                } else {
                  insertString(s3, ".", 2);
                }
              }

              sprintf(s4, "%.00f", telemetry_lon_base);
              if (telemetry_lon_base < 10000000) {
                insertString(s4, "0.", 0);
              }

              if (telemetry_lon_base > 9999999) {
                if (numOfChars(s4) == 8) {
                  insertString(s4, ".", 1);
                } else {
                  insertString(s4, ".", 2);
                }
              }

              s1_double = strtod(s1, &ptr);
              s2_double = strtod(s2, &ptr);
              s3_double = strtod(s3, &ptr);
              s4_double = strtod(s4, &ptr);
            }
            telemetry_distance = distanceEarth(s1_double, s2_double, s3_double, s4_double);
            break;

          case MAVLINK_MSG_ID_VFR_HUD:
            mavlink_vfr_hud_t vfr;
            mavlink_msg_vfr_hud_decode(&message, &vfr);
            telemetry_gspeed = vfr.groundspeed * 3.6;
            telemetry_vspeed = vfr.climb;
            telemetry_altitude = vfr.alt;
            break;

          case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
            mavlink_global_position_int_t global_position_int;
            mavlink_msg_global_position_int_decode( &message, &global_position_int);
            telemetry_hdg = global_position_int.hdg / 100;
            break;

          case MAVLINK_MSG_ID_ATTITUDE:
            mavlink_attitude_t att;
            mavlink_msg_attitude_decode(&message, &att);
            telemetry_pitch = att.pitch * (180.0 / 3.141592653589793238463);
            telemetry_roll = att.roll * (180.0 / 3.141592653589793238463);
            telemetry_yaw = att.yaw * (180.0 / 3.141592653589793238463);
            break;

          default:
            printf("> MavLink message %d from %d/%d\n",
              message.msgid, message.sysid, message.compid);
            break;
        }
      }
    }

    usleep(1);
  }

  return 0;
}

extern const char font_14_23[];
extern uint32_t frames_lost;
float rx_rate = 0;

// array size is 7602
extern const char openipc[];

int32_t map_range(int32_t x, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void* __OSD_THREAD__(void* arg) {
  struct _fbg* fbg = fbg_fbdevSetup(NULL, HI_FALSE, vo_width, vo_height);
  struct _fbg_img* bb_font_img = fbg_loadPNGFromMemory(fbg, font_14_23, 26197);
  struct _fbg_font* bbfont = fbg_createFont(fbg, bb_font_img, 14, 23, 33);
  struct _fbg_img* openipc_img = fbg_loadPNGFromMemory(fbg, openipc, 11761);
  int hours = 0 , minutes = 0 , seconds = 0 , milliseconds = 0;
  double resX_multiplier = 1;
  double resY_multiplier = 1;
  if (fbg->height == 1080) {
    resX_multiplier = 1.5;
    resY_multiplier = 1.5;
  }

  while (1) {
    fbg_clear(fbg, 0);
    fbg_draw(fbg);
    uint32_t x_center = fbg->width / 2;
    char msg[16];
    memset(msg, 0x00, sizeof(msg));

    if (osd_element18x > 0){
      // Artificial Horizon
      int32_t offset_pitch = telemetry_pitch * 4;
      int32_t offset_roll = telemetry_roll * 4;
      int32_t y_pos_left = ((int32_t)fbg->height / 2 - osd_element18x*resX_multiplier + offset_pitch + offset_roll);
      int32_t y_pos_right = ((int32_t)fbg->height / 2 - osd_element18y*resY_multiplier + offset_pitch - offset_roll);

      for (int i = 0; i < 4; i++) {
        if (y_pos_left > 0 && y_pos_left < fbg->height &&
          y_pos_right > 0 && y_pos_right < fbg->height) {
          fbg_line(fbg, x_center - 180, y_pos_left + i,
            x_center + 180, y_pos_right + i, 255, 255, 255);
        }
      }

      // Vertical Speedometer
      int32_t offset_vspeed = telemetry_vspeed * 5;
      int32_t y_pos_vspeed = ((int32_t)fbg->height / 2 - offset_vspeed);
      for (int i = 0; i < 8; i++) {
        if (y_pos_vspeed > 0 && y_pos_vspeed < fbg->height) {
          fbg_line(fbg, x_center + 242 + i, fbg->height / 2,
            x_center + 242 + i, y_pos_vspeed, 255, 255, 255);
        }
      }

      for (int i = 0; i < 25; i++) {
        uint32_t width = (i == 12) ? 10 : 0;

        fbg_line(fbg, x_center - 240 - width,
          fbg->height / 2 - 120 + i * 10, x_center - 220,
          fbg->height / 2 - 120 + i * 10, 255, 255, 255);
        fbg_line(fbg, x_center - 240 - width,
          fbg->height / 2 - 120 + i * 10 + 1, x_center - 220,
          fbg->height / 2 - 120 + i * 10 + 1, 255, 255, 255);

        fbg_line(fbg, x_center + 220, fbg->height / 2 - 120 + i * 10,
          x_center + 240 + width, fbg->height / 2 - 120 + i * 10, 255, 255, 255);
        fbg_line(fbg, x_center + 220, fbg->height / 2 - 120 + i * 10 + 1,
          x_center + 240 + width, fbg->height / 2 - 120 + i * 10 + 1, 255, 255, 255);
      }
    }
    
    // OSD telemetry
    sprintf(msg, "ALT:%.00fM", telemetry_altitude);
    if (osd_element1x > 0){fbg_write(fbg, msg, osd_element1x*resX_multiplier, osd_element1y*resY_multiplier);}
    sprintf(msg, "SPD:%.00fKM/H", telemetry_gspeed);
    if (osd_element2x > 0){fbg_write(fbg, msg, osd_element2x*resX_multiplier, osd_element2y*resY_multiplier);}
    sprintf(msg, "VSPD:%.00fM/S", telemetry_vspeed);
    if (osd_element3x > 0){fbg_write(fbg, msg, osd_element3x*resX_multiplier, osd_element3y*resY_multiplier);}
    sprintf(msg, "BAT:%.02fV", telemetry_battery / 1000);
    if (osd_element4x > 0){fbg_write(fbg, msg, osd_element4x, osd_element4y*resY_multiplier);}
    sprintf(msg, "CONS:%.00fmAh", telemetry_current_consumed);
    if (osd_element5x > 0){fbg_write(fbg, msg, osd_element5x, osd_element5y*resY_multiplier);}
    sprintf(msg, "CUR:%.02fA", telemetry_current / 100);
    if (osd_element6x > 0){fbg_write(fbg, msg, osd_element6x, osd_element6y*resY_multiplier);}
    sprintf(msg, "THR:%.00f%%", telemetry_throttle);
    if (osd_element7x > 0){fbg_write(fbg, msg, osd_element7x, osd_element7y*resY_multiplier);}
    sprintf(msg, "SATS:%.00f", telemetry_sats);
    if (osd_element8x > 0){fbg_write(fbg, msg, osd_element8x*resX_multiplier, osd_element8y*resY_multiplier);}
    sprintf(msg, "HDG:%.00f", telemetry_hdg);
    if (osd_element9x > 0){fbg_write(fbg, msg, osd_element9x*resX_multiplier, osd_element9y*resY_multiplier);}
    sprintf(c1, "%.00f", telemetry_lat);

    if (telemetry_lat < 10000000) {
      insertString(c1, "LAT:0.", 0);
    }

    if (telemetry_lat > 9999999) {
      if (numOfChars(c1) == 8) {
        insertString(c1, ".", 1);
      } else {
        insertString(c1, ".", 2);
      }
      insertString(c1, "LAT:", 0);
    }

    if (osd_element10x > 0){fbg_write(fbg, c1, osd_element10x*resX_multiplier, osd_element10y*resY_multiplier);}
    sprintf(c2, "%.00f", telemetry_lon);
    if (telemetry_lon < 10000000) {
      insertString(c2, "LON:0.", 0);
    }

    if (telemetry_lon > 9999999) {
      if (numOfChars(c2) == 8) {
        insertString(c2, ".", 1);
      } else {
        insertString(c2, ".", 2);
      }
      insertString(c2, "LON:", 0);
    }

    if (osd_element11x > 0){fbg_write(fbg, c2, osd_element11x*resX_multiplier, osd_element11y*resY_multiplier);}
    sprintf(msg, "DIST:%.03fM", telemetry_distance);
    if (osd_element12x > 0){fbg_write(fbg, msg, osd_element12x*resX_multiplier, osd_element12y*resY_multiplier);}
    sprintf(msg, "RSSI:%.00f", telemetry_rssi);
    if (osd_element13x > 0){fbg_write(fbg, msg, osd_element13x*resX_multiplier, osd_element13y*resY_multiplier);}
    if (osd_element14x > 0){fbg_image(fbg, openipc_img, osd_element14x*resX_multiplier, osd_element14y*resY_multiplier);}

    // Print rate stats
    struct timespec current_timestamp;
    if (!clock_gettime(CLOCK_MONOTONIC_COARSE, &current_timestamp)) {
      double interval = getTimeInterval(&current_timestamp, &last_timestamp);
      if (telemetry_arm > 1700){
        seconds = seconds + interval;
      }
      if (interval > 1) {
        last_timestamp = current_timestamp;
        rx_rate = ((float)stats_rx_bytes+(((float)stats_rx_bytes*25)/100)) / 1024.0f * 8;
        stats_rx_bytes = 0;
      }
    }

    char hud_frames_rx[32];
    memset(hud_frames_rx, 0, sizeof(hud_frames_rx));
    sprintf(hud_frames_rx, "RX Packets %d", frames_received);
    if (osd_element15x > 0){fbg_write(fbg, hud_frames_rx, osd_element15x*resX_multiplier, osd_element15y*resY_multiplier);}
    memset(hud_frames_rx, 0, sizeof(hud_frames_rx));
    sprintf(hud_frames_rx, "Rate %.02f Kbit/s", rx_rate);
    if (osd_element16x > 0){fbg_write(fbg, hud_frames_rx, osd_element16x*resX_multiplier, osd_element16y*resY_multiplier);}
    sprintf(msg, "TIME:%.2d:%.2d", minutes,seconds);
    if (osd_element17x > 0){fbg_write(fbg, msg, osd_element17x*resX_multiplier, osd_element17y*resY_multiplier);}
    if(seconds > 59){
       seconds = 0;
       ++minutes;
    }
    if(minutes > 59){
       seconds = 0;
       minutes = 0;
    }
    float percent = rx_rate / (1024 * 10);
    if (percent > 1) {
      percent = 1;
    }

    sprintf(msg, "TEMP:%.00fC", telemetry_raw_imu/100);
    if (osd_element19x > 0){fbg_write(fbg, msg, osd_element19x, osd_element19y*resY_multiplier);}
      

    uint32_t width = (strlen(hud_frames_rx) * 16) * percent;
    fbg_rect(fbg, (osd_element16x*resX_multiplier)-25, (osd_element16y*resY_multiplier)+25, width, 5, 255, 255, 255);
    fbg_flip(fbg);
    usleep(1);
  }
}


void* crsfThread(void* arg) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("Ошибка: не удалось создать сокет CRSF");
        return NULL;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(crsf_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Ошибка: не удалось привязать сокет CRSF");
        close(sock);
        return NULL;
    }

    uint8_t input[4096];
    size_t input_len = 0;

    while (1) {
        ssize_t len_received = recv(sock, input + input_len, sizeof(input) - input_len, 0);
        if (len_received > 0) {
            input_len += len_received;

            while (input_len > 2) {
                size_t expected_len = input[1] + 2;
                if (expected_len > 64 || expected_len < 4) {
                    printf("Invalid packet length: %zu. Resetting buffer.\n", expected_len);
                    input_len = 0;
                } else if (input_len >= expected_len) {
                    uint8_t single[64];
                    memcpy(single, input, expected_len);
                    memmove(input, input + expected_len, input_len - expected_len);
                    input_len -= expected_len;

                    if (!crsf_validate_frame(single, expected_len)) {
                        printf("CRC error\n");
                    } else {
                        handleCrsfPacket(single[2], single, expected_len);
                    }
                } else {
                    break; // Не хватает данных для полного пакета, ждем больше



}
            }
        } else if (len_received < 0) {
            perror("Ошибка при получении данных");
        }
        usleep(1000); // Ожидание
    }

    close(sock);
    return NULL;
}


void* crsfOsdThread(void* arg) {
    puts("Start OSD");
    struct _fbg* fbg = fbg_fbdevSetup(NULL, HI_FALSE, vo_width, vo_height);
    if (!fbg) {
        fprintf(stderr, "Ошибка: не удалось инициализировать OSD\n");
        return NULL;
    }
   puts("Started OSD");
    struct _fbg_img* bb_font_img = fbg_loadPNGFromMemory(fbg, font_14_23, 26197);
    struct _fbg_font* bbfont = fbg_createFont(fbg, bb_font_img, 14, 23, 33);
    struct _fbg_img* openipc_img = fbg_loadPNGFromMemory(fbg, openipc, 11761);

    double resX_multiplier = 1;
    double resY_multiplier = 1;
    if (fbg->height == 1080) {
        resX_multiplier = 1.5;
        resY_multiplier = 1.5;
    }

    char msg[64];

    while (1) {
	puts("Started OSD LOOP");
        fbg_clear(fbg, 0);  // Очистка экрана
	fbg_draw(fbg);
        // Наложение данных о батарее
        sprintf(msg, "Battery: %.2fV %.1fA", crsf_battery_voltage, crsf_battery_current);
        fbg_write(fbg, msg, 10, 10);

        // Наложение данных об ориентации
        sprintf(msg, "Attitude: Pitch=%.2f Roll=%.2f Yaw=%.2f", crsf_pitch, crsf_roll, crsf_yaw);
        fbg_write(fbg, msg, 10, 30);

        // Наложение режима полета
        sprintf(msg, "Flight Mode: %s", crsf_flight_mode);
        fbg_write(fbg, msg, 10, 50);

        fbg_flip(fbg);  // Обновление дисплея
        usleep(100000);  // Обновление каждые 100 мс
    }

    // Освобождение ресурсов
    fbg_freeFont(bbfont);
    fbg_freeImage(bb_font_img);
    fbg_freeImage(openipc_img);
    fbg_close(fbg);

    return NULL;
}
