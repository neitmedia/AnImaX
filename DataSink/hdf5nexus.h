#ifndef HDF5NEXUS_H
#define HDF5NEXUS_H
#include <structs.h>
#include <QString>
#include "H5Cpp.h"
using namespace H5;


class hdf5nexus
{
public:
    hdf5nexus();
    void createDataFile(QString, settingsdata);
    void closeDataFile();

    H5File *file;

private:

};

#endif // HDF5NEXUS_H
