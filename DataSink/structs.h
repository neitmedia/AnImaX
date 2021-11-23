#ifndef STRUCTS_H
#define STRUCTS_H
#include <string>

// define "settingsdata" struct
struct settingsdata {
    int scanHeight;
    int scanWidth;
    int ccdHeight;
    int ccdWidth;
    int sddChannels;
    std::string ROIdefinitions;
    std::string scantype;
};

struct metadata {
    int aquisition_number;
    int beamline_energy;
};

#endif // STRUCTS_H
