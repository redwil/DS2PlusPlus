#include "zke5_ident.h"

using namespace DS2PlusPlus;

const char ZKE5_Ident::zke_ident[] = {0xA0, 0x86, 0x90, 0x76, 0x63, 0x11, 0x02, 0x40, 0x10, 0x25, 0x00, 0x27, 0x16};

ZKE5_Ident::ZKE5_Ident()
{
    packet = DS2PacketPtr(PACKET_FROM_CHARS(ControlUnit::ADDRESS_ZKE, zke_ident));
    json = ControlUnitPtr(new ControlUnit);
    json->loadByUuid("B9D20D07-B7DA-4207-B8E1-2142AD938AD2");
    results = json->parseOperation("identify", packet);

}

void ZKE5_Ident::partNumber()
{
    QVariant expectedValue((quint64)6907663);
    QCOMPARE(results.value("part_number"), expectedValue);
}

void ZKE5_Ident::hardwareNumber()
{
    QVariant expectedValue(QString("0x11"));
    QCOMPARE(results.value("hardware_number"), expectedValue);
}

void ZKE5_Ident::codingIndex()
{
    QVariant expectedValue(QString("0x02"));
    QCOMPARE(results.value("coding_index"), expectedValue);
}

void ZKE5_Ident::diagIndex()
{
    QVariant expectedValue(QString("0x40"));
    QCOMPARE(results.value("diag_index"), expectedValue);
}

void ZKE5_Ident::busIndex()
{
    QVariant expectedValue(QString("0x10"));
    QCOMPARE(results.value("bus_index"), expectedValue);
}

void ZKE5_Ident::mfrWeek()
{
    QVariant expectedValue((quint8)25);
    QCOMPARE(results.value("build_date.week"), expectedValue);
}

void ZKE5_Ident::mfrYear()
{
    QVariant expectedValue((quint8)0);
    QCOMPARE(results.value("build_date.year"), expectedValue);
}

void ZKE5_Ident::supplier()
{
    QVariant expectedValue(QString("Delphi PHI"));
    QCOMPARE(results.value("supplier"), expectedValue);
}

void ZKE5_Ident::softwareNumber()
{
    QVariant expectedValue(QString("0x16"));
    QCOMPARE(results.value("software_number"), expectedValue);
}
