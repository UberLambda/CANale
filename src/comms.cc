#include "comms.hh"

#include "moc_comms.cpp"

namespace ca
{

Comms::Comms(QObject *parent)
    : QObject(parent)
{
}

Comms::~Comms() = default;

void Comms::progStart(unsigned devId, DeviceStats *outDeviceStats)
{
    // FIXME IMPLEMENT
}

void Comms::progEnd(unsigned devId)
{
    // FIXME IMPLEMENT
}

void Comms::flashPage(unsigned devId, uint32_t pageAddr, size_t pageLen, const uint8_t pageData[],
                      QTime timeout)
{
    // FIXME IMPLEMENT
}

}

