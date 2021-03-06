#ifndef HDF5NEXUS_H
#define HDF5NEXUS_H
#include <structs.h>
#include <QString>
#include "H5Cpp.h"
using namespace H5;

typedef QMap<std::string, QVector<uint32_t>> roidata;
typedef QVector<uint32_t> spectrumdata;

class hdf5nexus
{
public:
    hdf5nexus();

    void createDataFile(QString, settingsdata);
    void openDataFile(QString);
    void writeEndTimeStamp();
    void closeDataFile();

    void newNeXusAttribute(std::string, std::string);
    void newNeXusAttribute(Group*, std::string, std::string);
    void newNeXusAttribute(DataSet*, std::string, std::string);

    void writeScanIndexData(long, int, int);
    void writeLineBreakDataAndROIs(roidata, long, int, int, int, int);

    void writeMetadata(metadata);
    void writeSDDData(int32_t, spectrumdata);
    void writeCCDSettings(ccdsettings);

    void writeScanNote(std::string, int notecounter);

    DataSet* newNeXusScalarBooleanDataSet(std::string, std::string, bool, bool);
    DataSet* newNeXusScalarDataSet(std::string, std::string, std::string, bool);
    DataSet* newNeXusScalarDataSet(std::string, std::string, float, bool);
    DataSet* newNeXusScalarDataSet(std::string, std::string, int32_t, bool);

    Group* newNeXusGroup(std::string, std::string, std::string, bool);
    DataSet* newNeXusChunkedCCDDataSet(std::string, int, int, H5::PredType, std::string, bool, int, bool);
    DataSet* newNeXusChunkedTransmissionPreviewDataSet(std::string, H5::PredType, std::string, bool, int, bool);
    DataSet* newNeXusChunkedSpectraDataSet(std::string, H5::PredType, std::string, bool, int, bool);
    DataSet* newNeXusChunkedSDDLogDataSet(std::string, H5::PredType, std::string, bool, int, bool);
    DataSet* newNeXusROIDataSet(std::string, int, int, H5::PredType, std::string, bool, int, bool);
    DataSet* newNeXusChunked1DDataSet(std::string, H5::PredType, std::string, bool, int, bool);

    void appendValueTo1DDataSet(std::string, int, float);

    H5File *file;

private:

};

#endif // HDF5NEXUS_H
