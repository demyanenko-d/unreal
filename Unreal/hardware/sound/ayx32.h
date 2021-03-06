
#pragma once

// AYX32 emulator for Unreal Speccy

#include "..\sysdefs.h"

typedef void (*TASK)();

class PTR
{
public:
  u8 *addr;  // pointer to array
  u32 max;   // max number of bytes
  u32 cnt;   // counter of bytes

  void nul()
  {
    max = 0;
  }

  void init(u8 *a, u32 m)
  {
    addr = a;
    max = m;
    cnt = 0;
  }

  u8 read()
  {
    if (cnt < max)
    {
      cnt++;
      return *addr++;
    }
    else
      return 0xFF;
  }

  void write(u8 d)
  {
    if (cnt < max)
    {
      cnt++;
      *addr++ = d;
    }
  }

  void write_t(u8 d, TASK t)
  {
    if (cnt < max)
    {
      cnt++;
      *addr++ = d;

      if (cnt == max) t();
    }
  }
};

class SNDAYX32
{
public:

  /// Register definitions
  enum REG
  {
    // AY generic
    R_PSG_TP_AL   = 0x00,
    R_PSG_TP_AH   = 0x01,
    R_PSG_TP_BL   = 0x02,
    R_PSG_TP_BH   = 0x03,
    R_PSG_TP_CL   = 0x04,
    R_PSG_TP_CH   = 0x05,
    R_PSG_NP      = 0x06,
    R_PSG_MX      = 0x07,
    R_PSG_V_A     = 0x08,
    R_PSG_V_B     = 0x09,
    R_PSG_V_C     = 0x0A,
    R_PSG_EPL     = 0x0B,
    R_PSG_EPH     = 0x0C,
    R_PSG_EC      = 0x0D,
    R_PSG_IOA     = 0x0E,
    R_PSG_IOB     = 0x0F,

    // AY extended
    R_PSG_TDC_A   = 0x10,
    R_PSG_TDC_B   = 0x11,
    R_PSG_TDC_C   = 0x12,
    R_PSG_VOL_AL  = 0x19,
    R_PSG_VOL_AR  = 0x1A,
    R_PSG_VOL_BL  = 0x1B,
    R_PSG_VOL_BR  = 0x1C,
    R_PSG_VOL_CL  = 0x1D,
    R_PSG_VOL_CR  = 0x1E,

    // DAC
    R_DACCTRL     = 0x40,
    R_DACVOLL     = 0x41,
    R_DACVOLR     = 0x42,
    R_DACSMPR     = 0x43,
    R_DACFREE     = 0x44,
    R_DACUSED     = 0x45,
    R_DACDATA     = 0x46,

    // Device Control
    R_PSG_SEL     = 0xD0,
    R_PSG_CCTRL   = 0xD1,
    R_PSG_BCTRL   = 0xD2,
    R_PSG_ACTRL   = 0xD3,
    R_M_VOL_L     = 0xD4,
    R_M_VOL_R     = 0xD5,
    R_PSG_AMP_TAB = 0xD6,

    // System
    R_DEV_SIG     = 0xE0,
    R_CMD         = 0xE1,
    R_STATUS      = 0xE1,
    R_ERROR       = 0xE2,
    R_PARAM       = 0xE3,
    R_RESP        = 0xE3,
    R_DATA        = 0xE4,
    R_UPTIME      = 0xEB,
    R_VER         = 0xEC,
    R_CPR_STR     = 0xED,
    R_BLD_STR     = 0xEE,
    R_CORE_FRQ    = 0xEF,
  };

  /// Commands
  enum CMD
  {
    // System
    C_BREAK     = 0x00,

    // PSG
    C_PSG_INIT  = 0x10,

    // WS
    C_WS_INIT   = 0x20,
    C_WS_UPDATE = 0x21,

    // DAC
    C_DAC_CLR_FIFO = 0x40,

    // Device
    C_LOCK      = 0xE4,
    C_UPLD_FW   = 0xE8,
    C_FLASH_FW  = 0xE9,
    C_SAVE_CFG  = 0xEA,
    C_RESET     = 0xEB,
  };

  /// Error codes
  enum ERR
  {
    E_NONE    = 0x00,
    E_DONE    = 0x01,
    E_BREAK   = 0x02,
    E_BUSY    = 0x03,
    E_CMDERR  = 0x04,
    E_MODERR  = 0x05,
    E_PARMERR = 0x06,
    E_DATAERR = 0x07,
    E_SEQERR  = 0x08,
    E_EXECERR = 0x09,
  };

  /// Const definitions
  enum
  {
    DEV_SIG       = 0xAA55,     // Device signature
    HW_VER        = 1,          // Hardware version
    FW_VER        = 1,          // Firmware version
    FWHDR_VER     = 1,          // Firmware header version
    CF_VER        = 1,          // Config Pad version
  };

  /// Constants
  enum
  {
    MAGIC_FFW = 0x7841AA55, // 'Flash Firmware' parameter
    MAGIC_CFG = 0x37C855AA, // 'Save Config' parameter
    MAGIC_LCK = 0xC0DEFACE, // 'Unlock' parameter
    MAGIC_RES = 0xDEADBEEF, // 'Reset' parameter
    CORE_FRQ  = 168000000,  // ARM core frequency, kHz
  };

  typedef union
  {
    struct
    {
      u8 busy:1;   // bit 0: Command in progress
      u8 drq:1;    // bit 1: Data request
      u8 xxx:5;    // bit 2..6
      u8 boot:1;   // bit 7: Boot mode
    };
    u8 b;
  } STATUS;

  SNDAYX32();
  void write_addr(REG);
  void write(unsigned, u8);
  u8 read();

private:
  PTR r_ptr;
  PTR w_ptr;
  PTR rd_ptr;
  PTR wd_ptr;

  u8 param[256];
  u8 resp[256];

  STATUS status;

  REG reg;
  ERR error = E_NONE;

  bool is_locked = true;

  u32 temp_32;
  u16 temp_16;

  static const char cpr_str[];
  static const char bld_str[];
};