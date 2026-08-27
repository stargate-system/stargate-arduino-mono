#include "GateSelect.h"

GateSelect::GateSelect(OutputBuffer* outputBuffer) : GateValue() {
    this->type = "select";
    this->outputBuffer = outputBuffer;
    this->value = -1;
    this->nothingSelectedLabel = "";
    this->values = "";
}

String GateSelect::toString() {
    return this->value == -1 ? "" : String(this->value);
}

void GateSelect::fromRemote(String textValue) {
    if (textValue.length() > 0) {
        this->setValue((int) textValue.toInt());
    } else {
        this->setValue(-1);
    }
    this->onRemoteChange();
}

String GateSelect::toManifest() {
    if ((this->nothingSelectedLabel.length() == 0) && (this->value == -1)) {
        this-> value = 0;
    }
    String manifest = this->toPartialManifest();
    manifest += ",\"options\":{\"values\":[";
    if (this->values.length() > 0) {
        this->values.replace(",", "\",\"");
        manifest += "\"" + this->values + "\"";
    }
    manifest += "]";
    if (this->nothingSelectedLabel.length() > 0) {
        manifest += ",\"nothingSelectedLabel\":\"" + this->nothingSelectedLabel + "\"}}";
    } else {
        manifest += "}}";
    }
    return manifest;
}

int GateSelect::getValue() {
    return this->value;
}

void GateSelect::setValue(int value) {
    this->setValue(value, true);
}

void GateSelect::setValue(int value, bool equalityCheck) {
    if(!equalityCheck || (this->value != value)) {
        this->value = value;
        this->outputBuffer->sendValue(this);
    }
}

