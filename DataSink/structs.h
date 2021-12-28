#ifndef STRUCTS_H
#define STRUCTS_H
#include <string>

// define "settingsdata" struct
struct settingsdata {
    int scanHeight;
    int scanWidth;
    int ccdHeight;
    int ccdWidth;
    int energycount;
    float *energies;
    std::string datasinkIP;
    int datasinkPort;
    std::string sddIP;
    int sddPort;
    std::string ccdIP;
    int ccdPort;
    std::string roidefinitions;
    std::string scantype;
};

struct metadata {
    int acquisition_number;
    int beamline_energy;
};

#endif // STRUCTS_H
