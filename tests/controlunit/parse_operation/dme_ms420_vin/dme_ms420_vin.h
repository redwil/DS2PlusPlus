#ifndef DME_MS420_VIN_H
#define DME_MS420_VIN_H

#include <QObject>
#include <QString>
#include <QTest>

#include <ds2/ds2packet.h>
#include <ds2/manager.h>
#include <ds2/controlunit.h>

namespace Test_ControlUnit {
    namespace ParseOperation {

        class DME_MS420_VIN : public QObject
        {
            Q_OBJECT
        public:
            DME_MS420_VIN();

        protected:
            static const char dme_vin[];
            DS2PlusPlus::DS2PacketPtr packet;
            DS2PlusPlus::ControlUnitPtr ecu;
            DS2PlusPlus::PacketResponse results;

        private Q_SLOTS:
            void vin();
        };

    }
}

#endif // DME_MS420_VIN_H
