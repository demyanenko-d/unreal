#pragma once
#include <Poco/Path.h>
#include "Poco/Util/LayeredConfiguration.h"
#include "config.h"


extern char load_errors;
extern const char* sshot_ext[4];

void addpath(char *dst, const char *fname = nullptr);
void load_ula_preset();
void save_ram();
void save_nv();
void load_romset(CONFIG *conf, const char *romset);
void applyconfig();
void load_config(const Poco::Path& root_path, const Poco::Util::LayeredConfiguration &config);
void autoload();
void load_ay_vols();
void load_ay_stereo();
