#ifndef STRUCTS_H
#define STRUCTS_H
#include <string>

// define "ccdsettings" struct
struct ccdsettings {
    // ccd settings
    int binning_x;
    int binning_y;
    int ccdHeight;
    int ccdWidth;
    int pixelcount;
    int frametransfer_mode;
    int number_of_accumulations;
    int number_of_scans;
    float set_kinetic_cycle_time;
    int read_mode;
    int acquision_mode;
    int shutter_mode;
    int shutter_output_signal;
    float shutter_open_time;
    float shutter_close_time;
    int triggermode;
    float set_integration_time;
    float exposure_time;
    float accumulation_time;
    float kinetic_time;
    int min_temp;
    int max_temp;
    int target_temp;
    int pre_amp_gain;
    int em_gain_mode;
    int em_gain;
};

// define "settingsdata" struct
struct settingsdata {

    // general scan settings
    int scanHeight;
    int scanWidth;
    std::string save_path;
    std::string save_file;
    int energycount;
    float *energies;
    std::string scantype;
    std::string roidefinitions;
    std::string scantitle;

    // network settings
    std::string datasinkIP;
    int datasinkPort;
    std::string sddIP;
    int sddPort;
    std::string ccdIP;
    int ccdPort;
    int guiPort;

    // sample settings
    std::string sample_name;
    std::string sample_type;
    std::string sample_note;
    float sample_width;
    float sample_height;
    float sample_rotation_angle;

    // ccd settings
    int binning_x;
    int binning_y;
    int ccdHeight;
    int ccdWidth;
    int pixelcount;
    int frametransfer_mode;
    int number_of_accumulations;
    int number_of_scans;
    float set_kinetic_cycle_time;
    int read_mode;
    int acquision_mode;
    int shutter_mode;
    int shutter_output_signal;
    float shutter_open_time;
    float shutter_close_time;
    int triggermode;
    float set_integration_time;
    float exposure_time;
    float accumulation_time;
    float kinetic_time;
    int min_temp;
    int max_temp;
    int target_temp;
    int pre_amp_gain;
    int em_gain_mode;
    int em_gain;

    // sdd settings
    int sebitcount;
    int filter;
    int energyrange;
    int tempmode;
    int zeropeakperiod;
    int acquisitionmode;
    int checktemperature;
    bool sdd1;
    bool sdd2;
    bool sdd3;
    bool sdd4;

    // additional settings
    std::string notes;
    std::string userdata;
};

#endif // STRUCTS_H
