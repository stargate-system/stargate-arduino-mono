#ifndef GateFloat_h
#define GateFloat_h

#include <Arduino.h>
#include "GateValue.h"
#include "../OutputBuffer.h"

class GateFloat : public GateValue
{
    public:
        GateFloat(OutputBuffer* outputBuffer);
        String toString() override;
        void fromRemote(String textValue) override;
        String toManifest() override;
        double getValue();
        void setValue(double value);
        void setValue(double value, bool equalityCheck);
        void setMin(double min);
        void setMax(double max);
        unsigned int decimalPlaces;

    private:
        double min;
        double max;
        bool minSet;
        bool maxSet;
        OutputBuffer* outputBuffer;
        double value;
};

#endif