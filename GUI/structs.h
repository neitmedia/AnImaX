#ifndef STRUCTS_H
#define STRUCTS_H
#include <string>

// define "ccdsettings" struct
struct ccdsettings {
    // ccd settings
    float set_kinetic_cycle_time;
    float exposure_time;
    float accumulation_time;
    float kinetic_time;
};

// define "settingsdata" struct
struct settingsdata {

    // general scan settings
    int scanHeight;
    int scanWidth;
    std::string save_path;
    std::string save_file;
    bool file_compression;
    int file_compression_level;
    int energycount;
    float *energies;
    std::string scantype;
    std::string roidefinitions;
    std::string scantitle;
    float x_step_size;
    float y_step_size;

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
    int ccd_binning_x;
    int ccd_binning_y;
    int ccd_height;
    int ccd_width;
    int ccd_pixelcount;
    int ccd_frametransfer_mode;
    int ccd_number_of_accumulations;
    int ccd_number_of_scans;
    float ccd_set_kinetic_cycle_time;
    int ccd_read_mode;
    int ccd_acquisition_mode;
    int ccd_shutter_mode;
    int ccd_shutter_output_signal;
    float ccd_shutter_open_time;
    float ccd_shutter_close_time;
    int ccd_triggermode;
    float ccd_exposure_time;
    float ccd_accumulation_time;
    float ccd_kinetic_time;
    int ccd_min_temp;
    int ccd_max_temp;
    int ccd_target_temp;
    int ccd_pre_amp_gain;
    int ccd_em_gain_mode;
    int ccd_em_gain;

    // sdd settings
    int sdd_sebitcount;
    int sdd_filter;
    int sdd_energyrange;
    int sdd_tempmode;
    int sdd_zeropeakperiod;
    int sdd_acquisitionmode;
    int sdd_checktemperature;
    bool sdd_sdd1;
    bool sdd_sdd2;
    bool sdd_sdd3;
    bool sdd_sdd4;

    // source settings
    std::string source_name;
    std::string source_probe;
    std::string source_type;

    // additional settings
    std::string notes;
    std::string userdata;
};

#endif // STRUCTS_H
