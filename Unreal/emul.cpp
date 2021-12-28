#include "std.h"
#include "mods.h"
#include "emul.h"
#include "init.h"
#include "vars.h"
#include "engine/video/dx.h"
#include "engine/video/draw.h"
#include "mainloop.h"
#include "engine/utils/iehelp.h"
#include "engine/utils/util.h"
#include "memory.h"

#include <Poco/Util/Application.h>

#include "config.h"
#include "emulkeys.h"
#include "leds.h"
#include "debugger/dbgbpx.h"
#include "debugger/dbglabls.h"
#include "engine/loaders/snapshot.h"
#include "engine/loaders/tape.h"
#include "engine/utils/getopt.h"
#include "hardware/ft812.h"
#include "hardware/clones/visuals.h"
#include "hardware/gs/gs.h"
#include "hardware/z80/tables.h"

using Poco::Util::Application;

#define SND_TEST_FAILURES
//#define SND_TEST_SHOWSTAT

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER (DWORD)-1
#endif

unsigned frametime = 111111; //Alone Coder (GUI value for conf.frame)

#ifdef LOG_FE_OUT
	FILE *f_log_FE_out;
#endif
#ifdef LOG_FE_IN
	FILE *f_log_FE_in;
#endif
#ifdef LOG_TAPE_IN
	FILE *f_log_tape_in;
#endif

int nmi_pending = 0;

bool ConfirmExit();
BOOL WINAPI ConsoleHandler(DWORD ctrl_type);

void m_nmi(rom_mode page);
void showhelp(const char *anchor)
{
   sound_stop(); //Alone Coder 0.36.6
   showhelppp(anchor); //Alone Coder 0.36.6
   sound_play(); //Alone Coder 0.36.6
}

LONG __stdcall filter(EXCEPTION_POINTERS *pp)
{
   color(CONSCLR_ERROR);
   printf("\nexception %08X at eip=%p\n",
                pp->ExceptionRecord->ExceptionCode,
                pp->ExceptionRecord->ExceptionAddress);
#if _M_IX86
   printf("eax=%08X ebx=%08X ecx=%08X edx=%08X\n"
          "esi=%08X edi=%08X ebp=%08X esp=%08X\n",
          pp->ContextRecord->Eax, pp->ContextRecord->Ebx,
          pp->ContextRecord->Ecx, pp->ContextRecord->Edx,
          pp->ContextRecord->Esi, pp->ContextRecord->Edi,
          pp->ContextRecord->Ebp, pp->ContextRecord->Esp);
#endif
#if _M_IX64
   printf("rax=%08X rbx=%08X rcx=%08X rdx=%08X\n"
          "rsi=%08X rdi=%08X rbp=%08X rsp=%08X\n",
          pp->ContextRecord->Rax, pp->ContextRecord->Rbx,
          pp->ContextRecord->Rcx, pp->ContextRecord->Rdx,
          pp->ContextRecord->Rsi, pp->ContextRecord->Rdi,
          pp->ContextRecord->Rbp, pp->ContextRecord->Rsp);
#endif
   color();
   return EXCEPTION_CONTINUE_SEARCH;
}

static bool Exit = false;

bool ConfirmExit()
{
    if (!conf.confirm_exit)
        return true;

    return MessageBox(wnd, "Exit ?", "Unreal", MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND) == IDYES;
}

BOOL WINAPI ConsoleHandler(const DWORD ctrl_type)
{
    switch(ctrl_type)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        if (ConfirmExit())
            Exit = true;
        return TRUE;
    default: ;
    }
    return FALSE;
}

class App: public Application
{

protected:
    

	int main(const std::vector<std::string>& args) override
	{
        const DWORD ver = GetVersion();

        WinVerMajor = DWORD(LOBYTE(LOWORD(ver)));
        WinVerMinor = DWORD(HIBYTE(LOWORD(ver)));

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        nowait = *(unsigned*)&csbi.dwCursorPosition;

        SetThreadAffinityMask(GetCurrentThread(), 1);

        SetConsoleCtrlHandler(ConsoleHandler, TRUE);

        color(CONSCLR_TITLE);
        printf("UnrealSpeccy by SMT and Others\nBuild date: %s, %s\n\n", __DATE__, __TIME__);
        color();



        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
        rand_ram();
        load_spec_colors();
        //   applyconfig();
        sound_play();
        color();

        SetForegroundWindow(wnd);
        mainloop(Exit);

        return Application::EXIT_OK;
	}

	void initialize(Application& self) override
	{
        loadConfiguration(); // load default configuration files, if present
        Application::initialize(self);

        //+++++++++++++++++++++++++++++++++++++++++++cpu_info();

        

        temp.minimized = false;

        init_z80tables();
        init_ie_help();
        //+++++++++++++++++++++++++++++++++++++++++++load_config(config);
        //make_samples();


		/*init_gs();
        init_leds();
        init_tape();
        init_hdd_cd();
        start_dx();
        init_debug();
        init_visuals();
        applyconfig();
        main_reset();
        autoload();

        temp.gdiplus = GdiplusStartup();
        if (!temp.gdiplus)
        {
            color(CONSCLR_WARNING);
            printf("warning: gdiplus.dll was not loaded, only SCR and BMP screenshots available\n");
        }

        if (comp.ts.vdac2)
        {
            const char* ver = 0;

            if (const int rc = vdac2::open_ft8xx(&ver); rc == 0)
            {
                if (ver)
                    logger().information("FT library: " + std::string(ver));
            }
            else
            {
                logger().warning("Warning: FT8xx emulator failed! (error: %d, %lX)\n", rc, GetLastError());
                comp.ts.vdac2 = false;
            }
        }
        */
        load_errors = 0;
        trd_toload = 0;
        *(DWORD*)trd_loaded = 0; // clear loaded flags, don't see autoload'ed images

        for (auto it = argv().begin(); it != argv().end(); ++it)
        {
            trd_toload = DefaultDrive; // auto-select
            if (!loadsnap(it->c_str())) {
                logger().error("error loading :" + *it);
                load_errors = 1;
            }
        }

        if (load_errors) {
	        const int code = MessageBox(wnd, "Some files, specified in\r\ncommand line, failed to load\r\n\r\nContinue emulation?", "File loading error", MB_YESNO | MB_ICONWARNING);
            if (code != IDYES) exit();
        }

        InitializeCriticalSection(&tsu_toggle_cr);
	}

	void defineOptions(Poco::Util::OptionSet& options) override
	{
        Application::defineOptions(options);

        options.addOption(
	        Poco::Util::Option("bpx", "b", "define breakpoints file")
            .required(false)
            .repeatable(false)
            .argument("file")
            .callback(Poco::Util::OptionCallback<App>(this, &App::handle_bpx)));

        options.addOption(
	        Poco::Util::Option("label", "l", "define labels file")
            .required(false)
            .repeatable(false)
            .argument("file")
            .callback(Poco::Util::OptionCallback<App>(this, &App::handle_label)));

        options.addOption(
	        Poco::Util::Option("ini", "i", "load configuration data from a file")
            .required(false)
            .repeatable(true)
            .argument("file")
            .callback(Poco::Util::OptionCallback<App>(this, &App::handle_config)));
	}

    void handle_config(const std::string& name, const std::string& value)
    {
        loadConfiguration(value);
    }

    void handle_bpx(const std::string& name, const std::string& value)
    {
        init_bpx(value.c_str());
    }

    void handle_label(const std::string& name, const std::string& value)
    {
        init_labels(value.c_str());
    }
};

POCO_APP_MAIN(App)
