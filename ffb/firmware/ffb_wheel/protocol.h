#pragma once
// ============================================================================
//  YurFFB — protokol serial v1 (shared host <-> device)
//  Dipakai oleh firmware (Arduino) dan aplikasi PC (FfbBridge, lihat
//  ffb/app/FfbBridge/Protocol.cs — HARUS identik dengan file ini).
//
//  Format frame (little-endian):
//    [0xAA][0x55][LEN][CMD][PAYLOAD ...][CRC8]
//    LEN   = jumlah byte SETELAH LEN, termasuk CMD dan CRC8 (2..40)
//    CRC8  = CRC-8 poly 0x07, init 0x00, no-reflect, no-xorout,
//            dihitung terhadap CMD + PAYLOAD (LEN byte pertama dari blok itu).
//
//  Semua nilai multi-byte little-endian (AVR & x86 sama-sama LE).
// ============================================================================

#define YURFFB_SOF1 0xAA
#define YURFFB_SOF2 0x55
#define YURFFB_MAX_LEN 40

// ---- Perintah host -> device ----
enum YurFfbCmdH2D : uint8_t {
  YURFFB_CMD_PING        = 0x01, // -              -> PONG
  YURFFB_CMD_ENABLE      = 0x02, // u8 on          (1=aktif, 0=matikan motor)
  YURFFB_CMD_TORQUE      = 0x03, // i16 mPct       (-1000..+1000 = -100..+100% duty)
  YURFFB_CMD_SET_CONFIG  = 0x04, // 16 byte config -> di-echo sebagai CONFIG
  YURFFB_CMD_GET_STATE   = 0x05, // -              -> STATE langsung dikirim
  YURFFB_CMD_SET_CENTER  = 0x06, // -              (posisi sekarang = 0 deg)
  YURFFB_CMD_SET_TELEM   = 0x07, // u16 period_ms  (0 = telemetri off)
  YURFFB_CMD_SAVE_CONFIG = 0x08, // -              (simpan config ke EEPROM)
  YURFFB_CMD_RESET_CFG   = 0x09, // -              (muat default + hapus EEPROM)
};

// ---- Pesan device -> host ----
enum YurFfbCmdD2H : uint8_t {
  YURFFB_MSG_PONG   = 0x81, // 'Y','U','R','F','F','B', proto, fwMaj, fwMin, enc, drv (11 byte)
  YURFFB_MSG_STATE  = 0x82, // seq u32, angle_mdeg i32, vel_ddeg_s i16, torque i16, flags u8, loop_us u16 (15 byte)
  YURFFB_MSG_LOG    = 0x83, // string ASCII max 47 char + NUL
  YURFFB_MSG_CONFIG = 0x84, // echo 15 byte config (layout sama SET_CONFIG)
};

// ---- Layout payload SET_CONFIG / CONFIG (16 byte) ----
//  off  ukuran  field
//  0    u16     maxTorque_mPct        (1..1000) batas absolut torsi host+lokal
//  2    i16     slew_mPctPerMs        (0..1000) batas laju perubahan torsi
//  4    i16     softMin_deg           endstop lunak kiri  (derajat, mis. -420)
//  6    i16     softMax_deg           endstop lunak kanan (derajat, mis. +420)
//  8    i16     endstopK_mPctPerDeg   kekakuan pegas endstop
//  10   i16     endstopD_mPctPerDps   redaman endstop (per derajat/detik)
//  12   i16     damper_mPctPerDps     damper lokal (per derajat/detik)
//  14   u8      flags                 bit0=invert motor, bit1=invert encoder
//  15   u8      reserved (0)
// LEN frame SET_CONFIG = 1(CMD) + 16(payload) + 1(CRC) = 18.

#define YURFFB_CONFIG_PAYLOAD_LEN 16

// ---- flags pada STATE (u8) ----
#define YURFFB_FL_WATCHDOG   0x01  // tidak ada frame valid dari host > WATCHDOG_MS
#define YURFFB_FL_DISABLED   0x02  // motor tidak di-enable
#define YURFFB_FL_ENDSTOP    0x04  // endstop lunak sedang aktif
#define YURFFB_FL_ENC_FAULT  0x08  // encoder tidak terbaca
#define YURFFB_FL_CLAMPED    0x10  // torsi dibatasi (maxTorque/saturation)

// ---- flags config ----
#define YURFFB_CFG_INV_MOTOR   0x01
#define YURFFB_CFG_INV_ENCODER 0x02

// ---- versi protokol & firmware ----
#define YURFFB_PROTO_VER  1
#define YURFFB_FW_MAJOR   1
#define YURFFB_FW_MINOR   0
