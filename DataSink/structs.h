#ifndef STRUCTS_H
#define STRUCTS_H
#include <string>

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
    int ccdHeight;
    int ccdWidth;

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

struct metadata {
    int acquisition_number;
    std::string acquisition_time;
    float beamline_energy;
    float set_energy;
    float ringcurrent;
    bool horizontal_shutter;
    bool vertical_shutter;
};

#endif // STRUCTS_H
