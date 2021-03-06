#include "std.h"
#include "emul.h"
#include "vars.h"
#include "config.h"
#include "engine/video/dx.h"
#include "engine/utils/iehelp.h"
#include "hardware/gs/gs.h"
#include "leds.h"
#include "engine/loaders/tape.h"
#include "emulkeys.h"
#include "init.h"
#include "engine/loaders/snapshot.h"
#include "engine/audio/savesnd.h"
#include "hardware/wd93/wd93dat.h"
#include "hardware/z80/tables.h"
#include "debugger/dbgbpx.h"
#include "debugger/dbglabls.h"
#include "engine/utils/util.h"
#include "engine/utils/getopt.h"
#include "debugger/debug.h"
#include "hardware/clones/visuals.h"
#include "hardware/ft812.h"

void cpu_info()
{
   char idstr[64];
   idstr[0] = 0;

   fillCpuString(idstr);

   trim(idstr);

   unsigned cpuver = cpuid(1,0);
   unsigned features = cpuid(1,1);
   temp.mmx = (features >> 23) & 1;
   temp.sse = (features >> 25) & 1;
   temp.sse2 = (features >> 26) & 1;

   temp.cpufq = GetCPUFrequency();

   color(CONSCLR_HARDITEM); printf("cpu: ");

   color(CONSCLR_HARDINFO);
   printf("%s ", idstr);

   color(CONSCLR_HARDITEM);
   printf("%d.%d.%d [MMX:%s,SSE:%s,SSE2:%s] ",
      (cpuver>>8) & 0x0F, (cpuver>>4) & 0x0F, cpuver & 0x0F,
      temp.mmx ? "YES" : "NO",
      temp.sse ? "YES" : "NO",
      temp.sse2 ? "YES" : "NO");

   color(CONSCLR_HARDINFO);
   printf("at %d MHz\n", (unsigned)(temp.cpufq/1000000));

#ifdef MOD_SSE2
   if (!temp.sse2) {
      color(CONSCLR_WARNING);
      printf("warning: this is an SSE2 build, recompile or download non-P4 version\n");
   }
#else //MOD_SSE2
   if (temp.sse2) {
      color(CONSCLR_WARNING);
      printf("warning: SSE2 disabled in compile-time, recompile or download P4 version\n");
   }
#endif
}


void __declspec(noreturn) exit()
{
//   EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_ENABLED);
   exitflag = 1;
   if (savesndtype)
       savesnddialog();
   if (videosaver_state)
     main_savevideo();  // stop saving video

   if (!normal_exit)
       done_fdd(false);
   done_tape();
   done_dx();
   done_gs();
   done_leds();
   save_nv();
   zf232.rs_close();
   zf232.zf_close();
   done_ie_help();
   done_bpx();
   gdiplus_shutdown();

//   timeEndPeriod(1);
   if (ay[1].Chip2203) YM2203Shutdown(ay[1].Chip2203); //Dexus
   if (ay[0].Chip2203) YM2203Shutdown(ay[0].Chip2203); //Dexus
   if (comp.ts.vdac2) vdac2::close_ft8xx();

   color();
   printf("\nsee you later!\n");
   if (!nowait)
   {
       SetConsoleTitle("press a key...");
       FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
       getch();
   }
   fflush(stdout);
   SetConsoleCtrlHandler(ConsoleHandler, FALSE);
   exit(0);
}
