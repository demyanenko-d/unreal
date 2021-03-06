#include "std.h"
#include "emul.h"
#include "vars.h"
#include "engine/audio/sound.h"
#include "engine/video/draw.h"
#include "engine/video/dx.h"
#include "leds.h"
#include "memory.h"
#include "emulkeys.h"
#include "hardware/sound/vs1001.h"
#include "hardware/z80/z80.h"
#include "hardware/clones/zxevo.h"
#include "hardware/clones/visuals.h"
#include "engine/utils/util.h"

void spectrum_frame()
{
   if (!temp.inputblock || input.keymode != K_INPUT::KM_DEFAULT)
      input.make_matrix();

   init_snd_frame();
   init_frame();

   zxevo_set_readfont_pos(); // init readfont position to simulate correct font reading for zxevo -- lvd
   if (cpu.dbgchk)
   {
       cpu.SetDbgMemIf();
       z80dbg::z80loop();
   }
   else
   {
       cpu.SetFastMemIf();
       z80fast::z80loop();
   }

   if (zf232.open_port)
       zf232.io();

	flush_snd_frame();

	if (videosaver_state)
      savevideo_gfx();  // flush clean gfx to video saver

	if (conf.bordersize == 5)
		show_memcycles();
   //flush_frame();
   //showleds();

   if (!cpu.iff1 || // int disabled in CPU
        ((conf.memmodel == mem_model::atm710/* || conf.memmodel == atm3*/) && !(comp.pFF77 & 0x20))) // int disabled by ATM hardware -- lvd removed int disabling in pentevo (atm3)
   {
      u8 *mp = am_r(cpu.pc);
      if (cpu.halted)
      {
          strcpy(statusline, "CPU HALTED");
          statcnt = 10;
      }
      if (*(u16*)mp == WORD2(0x18,0xFE) ||
          ((*mp == 0xC3) && *(u16*)(mp+1) == (u16)cpu.pc))
      {
         strcpy(statusline, "CPU STOPPED");
         statcnt = 10;
      }
   }

   comp.t_states += conf.frame;
   cpu.t -= conf.frame;
   cpu.eipos -= conf.frame;
   comp.frame_counter++;
   if (conf.memmodel == mem_model::tsl)
   {
     comp.ts.intctrl.last_cput -= conf.frame;
   }
}

void do_idle()
{
   static volatile u64 last_cpu = rdtsc();
   for (;;)
   {
      asm_pause();
      const volatile u64 cpu = rdtsc();
      if ((cpu-last_cpu) >= temp.ticks_frame)
          break;

      if (conf.sleepidle)
      {
	      const ULONG delay = ULONG(((temp.ticks_frame - (cpu-last_cpu)) * 1000ULL) / temp.cpufq);

          if (delay != 0)
          {
              Sleep(delay);
 //             printf("sleep: %lu\n", Delay);
              break;
          }
      }
   }
   last_cpu = rdtsc();
}

void mainloop(const bool &exit)
{
   u8 skipped = 0;
   while (!exit)
	{
		temp.sndblock = !conf.sound.enabled;
		temp.inputblock = temp.vidblock && conf.sound.enabled;

		spectrum_frame();
    flip_visuals();
		//VideoSaver();

		if (skipped < temp.frameskip)
		{
			skipped++;
			temp.vidblock = 1;
		}
		else
			skipped = temp.vidblock = 0;

		// message handling before flip (they paint to rbuf)
		if (!temp.inputblock)
		{
			dispatch(conf.atm.xt_kbd ? ac_main_xt : ac_main);
		}

		if (!temp.vidblock)
			flip();

		if (!temp.sndblock)
		{
      if (videosaver_state)
        savevideo_snd();  // flush snd to video saver
			do_sound();
			Vs1001.Play();
		}

		if (!temp.sndblock)
		{
			if (conf.sound.do_sound == do_sound_none)
				do_idle();
		}
	}
   correct_exit();
}
