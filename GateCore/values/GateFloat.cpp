#include "GateFloat.h"

GateFloat::GateFloat(OutputBuffer* outputBuffer) : GateValue() {
    this->type = "float";
    this->outputBuffer = outputBuffer;
    this->decimalPlaces = 2;
}

String GateFloat::toString() {
    return String(this->value, this->decimalPlaces);
}

void GateFloat::fromRemote(String textValue) {
    this->setValue(textValue.toDouble());
    this->onRemoteChange();
}

String GateFloat::toManifest() {
    String manifest = this->toPartialManifest();
    if (this->minSet || this->maxSet) {
        String rangeMin = this->minSet ? String(this->min) : "null";
        String rangeMax = this->maxSet ? String(this->max) : "null";
        manifest += ",\"options\":{\"range\":[" + rangeMin + "," + rangeMax + "]}}";
    } else {
        manifest += "}";
    }
    return manifest;
}

double GateFloat::getValue() {
    return this->value;
}

void GateFloat::setValue(double value) {
    this->setValue(value, true);
}

void GateFloat::setValue(double value, bool equalityCheck) {
    if(!equalityCheck || (this->value != value)) {
        if (this->minSet && value < this->min) {
            value = this->min;
        } else if (this->maxSet && value > this->max) {
            value = this->max;
        }
        this->value = value;
        this->outputBuffer->sendValue(this);
    }
}

void GateFloat::setMin(double min) {
    this->minSet = true;
    this->min = min;
    if (this->value < min) {
        this->value = min;
    }
}

void GateFloat::setMax(double max) {
    this->maxSet = true;
    this->max = max;
    if (this->value > max) {
        this->value = max;
    }
}
