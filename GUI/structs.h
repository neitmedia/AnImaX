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
    std::string roidefinitions;
    std::string scantype;
};

#endif // STRUCTS_H
