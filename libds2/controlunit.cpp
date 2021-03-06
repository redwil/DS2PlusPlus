/*
 * This file is part of libds2
 * Copyright (C) 2014
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to:
 * Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301 USA
 *
 * Or see <http://www.gnu.org/licenses/>.
 */

#include <stdexcept>

#include <QDebug>

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>

#include <QtEndian>

#include <QRegularExpression>

#include <ds2/operation.h>
#include <ds2/controlunit.h>
#include <ds2/manager.h>
#include <ds2/dpp_v1_parser.h>

namespace DS2PlusPlus {

    QHash<QString, QList<quint8> > ControlUnit::_familyDictionary;
    QHash<QString, QString> ControlUnit::_familyNames;
    const QChar ControlUnit::zeroPadding = QChar('0');

    const QString ControlUnit::ROOT_UUID = "00001111-0000-0000-0000-000000000000";

    QList<quint8> ControlUnit::addressForFamily(const QString &aFamily)
    {
        if (_familyDictionary.isEmpty()) {
            _familyDictionary.insert("AIC",     QList<quint8>() << 0xE8        ); // Rain sensor? Automatic Interval Control?
            _familyDictionary.insert("DME",     QList<quint8>() << 0x12        ); // Digital Motor Electronics
            _familyDictionary.insert("DDE",     QList<quint8>() << 0x12        ); // Digital Diesel Electronics
            _familyDictionary.insert("DSC",     QList<quint8>() << 0x56 << 0x29); // Dynamic Stability Control
            _familyDictionary.insert("EHC",     QList<quint8>() << 0xAC << 0x82); // Elektronische Hohenstands Control / Electronic Height Control (EHC,EHC2,EHC2N,EHC2N2=AC, EHC2RR=82
            _familyDictionary.insert("EGS",     QList<quint8>() << 0x32        ); // Electronic Transmission Control - Electronische Getriebe Steuerung
            _familyDictionary.insert("EWS",     QList<quint8>() << 0x44        ); // Electronic Immobiliser / Elektronische Wegfahrsperre
            _familyDictionary.insert("IHKA",    QList<quint8>() << 0x5B        ); // Auto Climate Control / Integrierte Heizung Kühlung?
            _familyDictionary.insert("IKE",     QList<quint8>() << 0x80        ); // Instrument Cluster
            _familyDictionary.insert("IKI",     QList<quint8>() << 0x80        ); // Instrument Cluster
            _familyDictionary.insert("KOMBI",   QList<quint8>() << 0x80        ); // Instrument Cluster
            _familyDictionary.insert("LCM",     QList<quint8>() << 0xD0        ); // Light Switching Center / Lichtschaltzentrum
//            _familyDictionary.insert("LRA",     QList<quint8>() << 0xXX        ); // Vertical Headlight Aiming
            _familyDictionary.insert("LSZ",     QList<quint8>() << 0xD0        ); // Light Switching Center / Lichtschaltzentrum
            _familyDictionary.insert("LWS",     QList<quint8>() << 0x57        ); // Steering Angle Sensor / Lenkwinkelsensor
            _familyDictionary.insert("MID",     QList<quint8>() << 0xC0        ); // Multi Information Display
            _familyDictionary.insert("MFL",     QList<quint8>() << 0x50        ); // Multi function steering wheel?
            _familyDictionary.insert("MRS",     QList<quint8>() << 0xA4        ); // Multiple Restraint System
            _familyDictionary.insert("PDC",     QList<quint8>() << 0x60        ); // Park distance control?
            _familyDictionary.insert("RADIO",   QList<quint8>() << 0x68        ); // Radio
            _familyDictionary.insert("RDC",     QList<quint8>() << 0x70        ); // Tire pressure monitoring system?
            _familyDictionary.insert("RLS",     QList<quint8>() << 0xE8        ); // Rain and light sensor
            _familyDictionary.insert("SHD",     QList<quint8>() << 0x08        ); // Sunroof?
            _familyDictionary.insert("SMG",     QList<quint8>() << 0x32        ); // Sequential M Gearbox
            _familyDictionary.insert("SPMFT",   QList<quint8>() << 0x9B        ); // Driver's side mirror module
            _familyDictionary.insert("SPMBT",   QList<quint8>() << 0x51        ); // Pax side mirror module
            _familyDictionary.insert("SZM",     QList<quint8>() << 0xF5        ); // Center Console Switching Center?
            _familyDictionary.insert("ZKE",     QList<quint8>() << 0x00        ); // Central Body Electronics / Zentrale Karosserieelektronik
        }

        if (_familyDictionary.contains(aFamily)) {
            return _familyDictionary.value(aFamily);
        }

        return QList<quint8>();
    }

    const QStringList ControlUnit::knownFamilies()
    {
        addressForFamily("");
        return _familyDictionary.keys();
    }

    const QList<quint8> ControlUnit::knownAddresses()
    {
        QSet<quint8> set;

        // Load the address family info if needed
        addressForFamily("");

        foreach (const QList<quint8> addressList, _familyDictionary.values()) {
            foreach (const quint8 address, addressList) {
                set.insert(address);
            }
        }

        QList<quint8> ret(set.toList());
        qSort(ret);

        return ret;
    }

    const QString ControlUnit::familyForAddress(quint8 anAddress)
    {
        QStringList ret;

        // Load the address family info if needed
        addressForFamily("");

        QHashIterator<QString, QList<quint8> > familyList(_familyDictionary);
         while (familyList.hasNext()) {
             familyList.next();

             foreach(const quint8 address, familyList.value()) {
                if (address == anAddress) {
                    ret.append(familyList.key());
                }
             }
         }
         if (ret.isEmpty()) {
            return QString::null;
         } else {
             ret.sort();
             return ret.join(", ");
         }
    }

    const QString ControlUnit::nameForFamily(const QString &aFamily)
    {
        if (_familyNames.isEmpty()) {
            _familyNames.insert("AIC",     "Rain Sensor");
            _familyNames.insert("DME",     "Engine - Gas");
            _familyNames.insert("DDE",     "Engine - Diesel");
            _familyNames.insert("DSC",     "Stability Control");
            _familyNames.insert("EHC",     "Electronic Height Control");
            _familyNames.insert("EGS",     "Auto Transmission");
            _familyNames.insert("EWS",     "Immobiliser");
            _familyNames.insert("IHKA",    "Climate Control - Auto");
            _familyNames.insert("IKE",     "Instrument Cluster - High");
            _familyNames.insert("IKI",     "Instrument Cluster - High");
            _familyNames.insert("KOMBI",   "Instrument Cluster");
            _familyNames.insert("LCM",     "Light Module");
            _familyNames.insert("LSZ",     "Light Module");
            _familyNames.insert("LWS",     "Steering Angle Sensor");
            _familyNames.insert("MID",     "Multi Information Display");
            _familyNames.insert("MFL",     "Multi-function Steering Wheel");
            _familyNames.insert("MRS",     "Airbag / SRS");
            _familyNames.insert("PDC",     "Park distance control");
            _familyNames.insert("RADIO",   "Radio");
            _familyNames.insert("RDC",     "TPMS");
            _familyNames.insert("RLS",     "Rain Sensor");
            _familyNames.insert("SHD",     "Sunroof");
            _familyNames.insert("SMG",     "Sequential M Gearbox");
            _familyNames.insert("SPMFT",   "Power Mirror - Driver");
            _familyNames.insert("SPMBT",   "Power Mirror - Passenger");
            _familyNames.insert("SZM",     "Center Console");
            _familyNames.insert("ZKE",     "Chassis Electronics");
        }

        if (_familyNames.contains(aFamily)) {
            return _familyNames.value(aFamily);
        }

        return aFamily;
    }

    ControlUnit::ControlUnit(const QString &aUuid, Manager *aParent) :
        QObject(aParent), _protocol(BasePacket::ProtocolDS2), _manager(aParent)
    {
        if (_manager == NULL) {
            _manager = new Manager();
        }

        if (!aUuid.isNull()) {
            loadByUuid(aUuid);
        }
    }

    void ControlUnit::loadByUuid(const QString &aUuid)
    {
        QTextStream qOut(stdout);
        QTextStream qErr(stderr);

        _operations.clear();

        QString moduleParent = aUuid;

        // Add a "find root UUID" method to dbm
        while (!moduleParent.isEmpty()) {
            QHash<QString, QVariant> theRecord = _manager->findModuleRecordByUuid(moduleParent);
            if (theRecord.isEmpty()) {
                throw std::runtime_error("Find parent failed");
            }

            if (aUuid == moduleParent) {
                _dppVersion = theRecord.value("dpp_version").toInt();
                _fileVersion = theRecord.value("file_version").toInt();
                _uuid = DPP_V1_Parser::rawUuidToString(theRecord.value("uuid").toByteArray());
                _address = theRecord.value("address").toChar().toLatin1();
                _family = theRecord.value("family").toString();
                _name = theRecord.value("name").toString();

                QString ourProtocol = theRecord.value("protocol").toString();
                if (ourProtocol == "DS2") {
                    _protocol = BasePacket::ProtocolDS2;
                } else if (ourProtocol == "KWP0") {
                    _protocol = BasePacket::ProtocolKWP;
                } else if (ourProtocol.isEmpty()) {
                    // Skip for now, hope it comes in via the parent.
                } else {
                    throw std::invalid_argument(qPrintable(QString("Invalid protocol.  ECU=%1, protocol=%2").arg(_uuid).arg(ourProtocol)));
                }

                _partNumbers = QSet<quint64>();

                QSqlQuery partNumbersForModuleQuery(_manager->sqlDatabase());
                partNumbersForModuleQuery.prepare("SELECT part_number FROM modules_part_numbers WHERE module_uuid = :module_uuid");
                partNumbersForModuleQuery.bindValue(":module_uuid", DPP_V1_Parser::stringToUuidVariant(moduleParent));
                partNumbersForModuleQuery.exec();

                while (partNumbersForModuleQuery.next()) {
                    QSqlRecord pnRecord = partNumbersForModuleQuery.record();
                    const quint64 partNumber = pnRecord.value("part_number").toULongLong();
                    _partNumbers.insert(partNumber);
                }

                _diagIndexes = QSet<quint64>();

                QSqlQuery diagIndexesForModuleQuery(_manager->sqlDatabase());

                diagIndexesForModuleQuery.prepare("SELECT diag_index FROM modules_diag_indexes WHERE module_uuid = :module_uuid");
                diagIndexesForModuleQuery.bindValue(":module_uuid", DPP_V1_Parser::stringToUuidVariant(moduleParent));
                diagIndexesForModuleQuery.exec();

                while (diagIndexesForModuleQuery.next()) {
                    QSqlRecord diRecord = diagIndexesForModuleQuery.record();
                    const quint64 diagIndex = diRecord.value("diag_index").toULongLong();
                    _diagIndexes.insert(diagIndex);
                }

                _hardwareNumber = theRecord.value("hardware_num").toULongLong();
                _softwareNumber = theRecord.value("software_num").toULongLong();
                _codingIndex = theRecord.value("coding_index").toULongLong();
                _bigEndian = (theRecord.value("big_endian").toULongLong() == 1) ? true : false;
                time_t mtimeInt = theRecord.value("mtime").toULongLong();
                _fileLastModified.setTime_t(mtimeInt);
            }

            if (getenv("DPP_TRACE")) {
                qErr << "Module: " << moduleParent << " (from: " << aUuid << ")" << endl;
            }

            QSqlQuery operationsForModuleQuery(_manager->sqlDatabase());
            operationsForModuleQuery.prepare("SELECT * FROM operations WHERE module_id = :module_id");
            operationsForModuleQuery.bindValue(":module_id", DPP_V1_Parser::stringToUuidVariant(moduleParent));
            operationsForModuleQuery.exec();

            while (operationsForModuleQuery.next()) {
                QSqlRecord opRecord = operationsForModuleQuery.record();

                const QString opName = opRecord.value("name").toString();
                const QString opUuid = DPP_V1_Parser::rawUuidToString(opRecord.value("uuid").toByteArray());
                //const QString opModule = DPP_V1_Parser::rawUuidToString(opRecord.value("module_id").toByteArray());
                const QString opParent = DPP_V1_Parser::rawUuidToString(opRecord.value("parent_id").toByteArray());
                const QByteArray opCommand = opRecord.value("command").toByteArray();

                OperationPtr op;
                if (_operations.contains(opName)) {
                    op = _operations.value(opName);
                    if (op->parentId() == opUuid) {
                        if (getenv("DPP_TRACE")) {
                            qErr << "\tMerging operation: '" << opName << "' (" << opUuid << ")" << endl;
                            qErr << "\t\tParent ID: " << op->uuid() << endl;
                        }

                        // If we've not yet set the command from a higher priority operation, use this one.
                        if (op->command().isEmpty()) {
                            op->setCommand(opCommand);
                        }
                    } else
                    {
                        if (getenv("DPP_TRACE")) {
                            qErr << "\tSkipping operation: '" << opName << "' (" << opUuid << ")" << endl;
                        }
                        continue;
                    }
                } else {
                    if (getenv("DPP_TRACE")) {
                        qErr << "\tAdding operation: '" << opName << "' (" << opUuid << ")" << endl;
                    }
                    op = OperationPtr(new Operation(opUuid, _address, opName, opCommand, _protocol));
                    op->setParentId(opParent);
                }

                QString curOpUuid = opUuid;
                QSqlRecord curOpRecord = opRecord;

                while (!curOpUuid.isEmpty()) {

                    QSqlQuery resultsForOperationQuery(_manager->sqlDatabase());
                    resultsForOperationQuery.prepare("SELECT * FROM results WHERE operation_id = :operation_id");
                    resultsForOperationQuery.bindValue(":operation_id", DPP_V1_Parser::stringToUuidVariant(curOpUuid));
                    resultsForOperationQuery.exec();

                    while (resultsForOperationQuery.next()) {
                        QSqlRecord resultRecord = resultsForOperationQuery.record();
                        QString ourUuid = DPP_V1_Parser::rawUuidToString(resultRecord.value("uuid").toByteArray());
                        Result result;

                        QString resultId = ourUuid;
                        while (!resultId.isEmpty()) {
                            if (ourUuid == resultId) {
                                result.setName(resultRecord.value("name").toString());
                                const QString resultUuid = DPP_V1_Parser::rawUuidToString(resultRecord.value("uuid").toByteArray());
                                result.setUuid(resultUuid);
                            }

                            if (result.startPosition() == -1) {
                                result.setStartPosition(resultRecord.value("start_pos").toInt());
                            }

                            if (result.type().isEmpty()) {
                                result.setType(resultRecord.value("type").toString());
                                result.setDisplayFormat(resultRecord.value("display").toString());
                                result.setLength(resultRecord.value("length").toInt());
                                result.setMask(resultRecord.value("mask").toString());
                                result.setRpn(resultRecord.value("rpn").toString());
                                result.setUnits(resultRecord.value("units").toString());

                                QJsonParseError jsonError;
                                QHash<QString, QString> ourLevels;
                                QByteArray jsonByteArray(qPrintable(resultRecord.value("levels").toString()));
                                QJsonDocument levelsDoc = QJsonDocument::fromJson(jsonByteArray, &jsonError);
                                QJsonObject ourLevelsJson = levelsDoc.object();

                                QJsonObject::Iterator levelsIterator = ourLevelsJson.begin();
                                while (levelsIterator != ourLevelsJson.end()) {
                                    QJsonValue level = levelsIterator.value();
                                    if (level.isString()) {
                                        ourLevels.insert(levelsIterator.key(), levelsIterator.value().toString());
                                    }
                                    levelsIterator++;
                                }

                                result.setLevels(ourLevels);
                            }

                            resultId = DPP_V1_Parser::rawUuidToString(resultRecord.value("parent_id").toByteArray());
                            if (resultId.isEmpty()) {
                                break;
                            }

                            QSqlQuery parentResultsQuery(_manager->sqlDatabase());
                            parentResultsQuery.prepare("SELECT * FROM results WHERE uuid = :uuid");
                            parentResultsQuery.bindValue(":uuid", DPP_V1_Parser::stringToUuidVariant(resultId));
                            parentResultsQuery.exec();
                            parentResultsQuery.first();
                            resultRecord = parentResultsQuery.record();
                        }

                        if (op->results().contains(result.name())) {
                            if (getenv("DPP_TRACE")) {
                                qErr << "\t\tSkipping result " << result.name() << " as we've a higher priority implementation" << endl;
                            }
                        } else {
                            if (getenv("DPP_TRACE")) {
                                qErr << "\t\tAdding result: " << result.name() << endl;
                            }
                            op->insertResult(result.name(), result);
                        }
                    }

                    curOpUuid = DPP_V1_Parser::rawUuidToString(curOpRecord.value("parent_id").toByteArray());
                    if (curOpUuid.isEmpty()) {
                        break;
                    }

                    QSqlQuery subOps(_manager->sqlDatabase());
                    subOps.prepare("SELECT * FROM operations WHERE uuid = :uuid");
                    subOps.bindValue(":uuid", DPP_V1_Parser::stringToUuidVariant(curOpUuid));
                    subOps.exec();
                    subOps.first();
                    curOpRecord = subOps.record();
                }

                _operations.insert(opName, op);
            }
            moduleParent = DPP_V1_Parser::rawUuidToString(theRecord.value("parent_id").toByteArray());
        }
    }

    PacketResponse ControlUnit::executeOperation(const QString &name)
    {
        QTextStream qOut(stdout);
        QTextStream qErr(stderr);

        const OperationPtr ourOp(_operations.value(name));
        if (ourOp.isNull()) {
            throw std::invalid_argument(qPrintable(QString("Operation '%1' could not be found in ECU %2").arg(name).arg(_uuid)));
        }

        BasePacketPtr ourOutgoingPacket(ourOp->queryPacket());

        if (getenv("DPP_TRACE")) {
            qErr << ">> " << ourOp->name() << ": " << ourOp->command().join(" ") << endl;
        }

        BasePacketPtr ourIncomingPacket(_manager->query(ourOutgoingPacket));

        return parseOperation(ourOp, ourIncomingPacket);
    }

    PacketResponse ControlUnit::parseOperation(const QString &name, const BasePacketPtr packet)
    {
        const OperationPtr theOp = (_operations.value(name));

        if (theOp.isNull()) {
            throw std::invalid_argument(qPrintable(QString("Operation '%1' could not be found in ECU %2").arg(name).arg(_uuid)));
        }

        return parseOperation(theOp, packet);
    }

    PacketResponse ControlUnit::parseOperation(const OperationPtr theOp, const BasePacketPtr packet)
    {
        if (theOp.isNull()) {
            throw std::invalid_argument(qPrintable(QString("parseOperation requires a valid operation.")));
        }

        QTextStream qOut(stdout);
        QTextStream qErr(stderr);

        PacketResponse ret;

        if (packet->targetAddress() != address()) {
            QString errorString = QString("WARNING: RECV'D PACKET FOR ECU AT 0x%1. OUR ADDR IS 0x%2").arg(packet->targetAddress(), 2, 16, QChar('0')).arg(address(), 2, 16, QChar('0'));
            //qErr << errorString << endl;
        }

        if (getenv("DPP_TRACE")) {
            qOut << "<< REPLY: " << *packet << endl;
        }

        foreach(const Result &result, theOp->results()) {
            //qDebug() << "Result: " << result.name;

            if ((result.startPosition() >= packet->data().length() ) || ((result.startPosition() + result.length() - 1) >= packet->data().length())) {
                //qDebug() << "Skipping out of range result (" << result.uuid << "/" << result.name;
                continue;
            }
            if (result.isType("byte") || result.isType("signed_byte")) {
                ret.insert(result.name(), resultByteToVariant(packet, result));
            } else if (result.isType("short") || result.isType("signed_short")) {
                ret.insert(result.name(), resultShortToVariant(packet, result));
            } else if (result.isType("hex_string")) {
                ret.insert(result.name(), resultHexStringToVariant(packet, result));
            } else if (result.isType("string")) {
                QString string;
                for (int i=0; i < result.length(); i++) {
                    QChar byte(packet->data().at(result.startPosition() + i));
                    string.append(byte);
                }
                if (result.displayFormat() == "string") {
                    ret.insert(result.name(), QVariant(string));
                } else if (result.displayFormat() == "hex_string") {
                    string.prepend("0x");
                    ret.insert(result.name(), QVariant(string));
                } else if (result.displayFormat() == "int") {
                    ret.insert(result.name(), QVariant(string.toULongLong()) );
                } else {
                    QString errorString = QString("Unknown display type for string type: ").arg(result.displayFormat());
                    throw std::invalid_argument(qPrintable(errorString));
                }
            } else if (result.isType("short_vin")) {
                QString vin;
                vin.append(packet->data().at(result.startPosition()));
                vin.append(packet->data().at(result.startPosition() + 1));

                quint32 number_part;
                memcpy(&number_part, packet->data().mid(result.startPosition() + 1, 4), 4);
                number_part = qFromBigEndian(number_part);
                number_part = (number_part & 0x00ffffff) >> 4;
                vin.append(QString::number(number_part, 16));

                ret.insert(result.name(), vin);
            } else if (result.isType("6bit-string")) {
                QByteArray encodedString = packet->data().mid(result.startPosition(), result.length());

                quint16 numBits = result.length() * 8;
                QString decodedString;

                for (int i=0; i < (numBits / 6); i++) {
                  int j = ((result.length()* 8) - (6*(i+1))) - 1;
                  char foo = decode_vin_char(j, encodedString);
                  decodedString.prepend(QChar(foo));
                }
                ret.insert(result.name(), QVariant(decodedString));

            } else if (result.isType("boolean")) {
                if (result.length() != 1) {
                    throw std::invalid_argument("Incorrect length for boolean type encountered");
                }
                unsigned char byte = packet->data().at(result.startPosition());
                bool condition = ((byte & result.mask()) > 0);
                if (result.displayFormat() == "string") {
                    ret.insert(result.name(), QVariant(result.stringForLevel(condition)));
                } else if (result.displayFormat() == "raw") {
                    ret.insert(result.name(), QVariant(condition));
                }
            } else {
                qErr << "Unknown result type: " << result.type() << endl;
            }
        }

        return ret;
    }

    quint32 ControlUnit::dppVersion() const
    {
        return _dppVersion;
    }

    quint32 ControlUnit::fileVersion() const
    {
        return _fileVersion;
    }

    QDateTime ControlUnit::fileLastModified() const
    {
        return _fileLastModified;
    }

    const QString ControlUnit::uuid() const
    {
        return _uuid;
    }

    quint8 ControlUnit::address() const
    {
        return _address;
    }
    void ControlUnit::setAddress(quint8 anAddress)
    {
        _address = anAddress;
        _family = familyForAddress(anAddress).split(", ").at(0);
        foreach (const QString &key, operations().keys()) {
            _operations[key]->setAddress(anAddress);
        }
    }

    QString ControlUnit::family() const
    {
        return _family;
    }

    bool ControlUnit::isRoot() const
    {
        return _uuid == ROOT_UUID;
    }

    const QString ControlUnit::name() const
    {
        return _name;
    }

    QHash<QString, OperationPtr > ControlUnit::operations() const
    {
        return _operations;
    }

    QSet<quint64> ControlUnit::partNumbers() const {
        return _partNumbers;
    }

    QSet<quint64> ControlUnit::diagIndexes() const {
        return _diagIndexes;
    }

    quint64 ControlUnit::hardwareNumber() const {
        return _hardwareNumber;
    }

    quint64 ControlUnit::softwareNumber() const {
        return _softwareNumber;
    }

    quint64 ControlUnit::codingIndex() const {
        return _codingIndex;
    }

    bool ControlUnit::bigEndian() const {
        return _bigEndian;
    }

    quint8 ControlUnit::matchFlags() const
    {
        return _matchFlags;
    }

    void ControlUnit::setMatchFlags(quint8 someFlags)
    {
        _matchFlags = someFlags;
    }

    char ControlUnit::decode_vin_char(int start, const QByteArray &bytes)
    {
      quint8 finish = start + 6;
      quint8 start_byte = start / 8;
      quint8 start_bit = (start % 8);
      quint8 finish_byte = finish / 8;
      quint8 finish_bit = finish % 8;

      if (start_byte == finish_byte) {
        quint8 mask = (1 << (finish_bit - start_bit)) - 1;
        return ControlUnit::getCharFrom6BitInt(bytes[start_byte] & mask);
      } else {
        start_bit++;

        quint8 high_nibble = bytes[start_byte] & (0xff >> start_bit);
        quint8 low_nibble = bytes[finish_byte] & (0xff << (7 - finish_bit));
        quint8 finish = (high_nibble << (6 - (8 - start_bit))) | (low_nibble >> (7 - finish_bit));

        return ControlUnit::getCharFrom6BitInt(finish);
      }

      return -1;
    }

    char ControlUnit::getCharFrom6BitInt(quint8 n)
    {
      if (n >= 0 && n <= 9) {
        return '0' + n;
      } else if (n >= 10 && n <= 35) {
        return 'A' + (n - 10);
      }
      return '!';
    }

    template <typename T> T ControlUnit::runRpnForResult(const Result &aResult, T aValue)
    {
        if (getenv("RPN_TRACE")) {
            qDebug() << "";
            qDebug() << "RPN IS: " << aResult.rpn() << " " << aValue;
        }

        T ourValue = aValue;

        if (aResult.type() == "signed_byte") {
            ourValue = static_cast<qint8>(aValue);
        }

        if (!aResult.rpn().isEmpty()) {
            QList<T> stack;
            foreach (const QString &command, aResult.rpn()) {
                if (command.startsWith("0x")) {
                    bool ok;
                    quint64 theNum = command.toULongLong(&ok, 16);
                    if (ok) {
                        stack.push_back(theNum);
                    } else {
                        throw std::invalid_argument("Argh, tried to parse an invalid hex string");
                    }
                } else if (command == "+")  {
                    T a, b;
                    a = stack.takeLast();
                    b = stack.takeLast();
                    stack.push_back(b + a);
                } else if (command == "-")  {
                    T a, b;
                    a = stack.takeLast();
                    b = stack.takeLast();
                    stack.push_back(b - a);
                } else if (command == "/")  {
                    T a, b;
                    a = stack.takeLast();
                    b = stack.takeLast();
                    stack.push_back(b / a);
                } else if (command == "*")  {
                    T a, b;
                    a = stack.takeLast();
                    b = stack.takeLast();
                    stack.push_back(b * a);
                } else if (command == "&")  {
                    T a, b;
                    a = stack.takeLast();
                    b = stack.takeLast();
                    stack.push_back(static_cast<quint64>(b) & static_cast<quint64>(a));
                } else if (command == ">>") {
                    T a, b;
                    a = stack.takeLast();
                    b = stack.takeLast();
                    stack.push_back(static_cast<quint64>(b) >> static_cast<quint64>(a));
                    if (getenv("RPN_TRACE")) {
                        qDebug() << ">> = " << stack.last();
                    }
                } else if (command == "<<") {
                    T a, b;
                    a = stack.takeLast();
                    b = stack.takeLast();
                    stack.push_back(static_cast<quint64>(b) << static_cast<quint64>(a));
                } else if (command == "N")  {
                    if (getenv("RPN_TRACE")) {
                        qDebug() << "N: " << ourValue;
                    }

                    stack.push_back(ourValue);
                } else {
                    // Assume it's a base 10 integer
                    bool ok;
                    T theNum;
                    if (command.indexOf('.') > -1) {
                        // Assume float
                        theNum = command.toDouble(&ok);
                    } else {
                        // Assume int
                        theNum = command.toLongLong(&ok, 10);
                    }

                    if (ok) {
                        stack.push_back(theNum);
                    } else {
                        QString errorString("Argh, tried to parse an invalid decimal string: %1");
                        throw std::invalid_argument(qPrintable(errorString.arg(command)));
                    }
                }
            }
            return stack.takeFirst();
        }
        return ourValue;
    }

    QVariant ControlUnit::resultShortToVariant(const BasePacketPtr aPacket, const Result &aResult)
    {
        if (aResult.length() != 2) {
            QString errorString = QString("Length for short data type must be 2.  Length was %1, Result was %2 (%3)").arg(aResult.length()).arg(aResult.name()).arg(aResult.uuid());
            throw std::invalid_argument(qPrintable(errorString));
        }

        quint16 ourNumber;
        memcpy(&ourNumber, aPacket->data().mid(aResult.startPosition(), aResult.length()), qMin(static_cast<size_t>(aResult.length()), sizeof(quint16)));

        if (_bigEndian) {
            ourNumber = qFromBigEndian(ourNumber);
        } else {
            ourNumber = qFromLittleEndian(ourNumber);
        }

        if (aResult.displayFormat() == "int") {
            if (aResult.isType("signed_short")) {
                qint64 value = runRpnForResult<float>(aResult, static_cast<qint16>(ourNumber));
                return QVariant(value);
            } else {
                quint64 value = runRpnForResult<float>(aResult, ourNumber);
                return QVariant(value);
            }
        } else if (aResult.displayFormat() == "float") {
            double ourFloat;
            if (aResult.isType("signed_short")) {
                ourFloat = runRpnForResult<float>(aResult, static_cast<qint16>(ourNumber));
            } else {
                ourFloat = runRpnForResult<float>(aResult, ourNumber);
            }

            return QVariant(ourFloat);
        } else {
            throw std::invalid_argument("Invalid display type for short specified.");
        }
    }

    QVariant ControlUnit::resultByteToVariant(const BasePacketPtr aPacket, const Result &aResult)
    {
        if (aResult.length() != 1) {
            QString ourError = QObject::tr("Length is not one for a byte data type.  Length is %1").arg(aResult.length());
            throw std::invalid_argument(qPrintable(ourError));
        }

        unsigned char byte = aPacket->data().at(aResult.startPosition());
        if (aResult.mask() != 0) {
            byte = (byte & aResult.mask()) & 0xff;
        }

        const qint64 num = runRpnForResult<qint64>(aResult, byte);

        if (aResult.displayFormat() == "hex_string") {
            const QString hex = QString("0x%1").arg(QString::number(byte, 16), 2, zeroPadding);
            return QVariant(hex);
        } else if (aResult.displayFormat() == "hex_int") {
            const QString hex = QString("%1").arg(QString::number(byte, 16), 2, zeroPadding);
            return QVariant(hex.toUInt());
        } else if (aResult.displayFormat() == "raw") {
            return QVariant(num);
        } else if (aResult.displayFormat().startsWith("string_table:")) {
            const QString tableName = aResult.displayFormat().mid(13);
            const QString stringValue = _manager->findStringByTableAndNumber(tableName, num);

            if (!stringValue.isEmpty()) {
               return QVariant(stringValue);
            } else {
                const QString hex = QString("0x%1").arg(QString::number(byte, 16), 2, zeroPadding);
                return QVariant(hex);
            }

            return QVariant(tableName);
        } else if (aResult.displayFormat() == "float") {
            double value;
            if (aResult.type() == "signed_byte") {
                value = runRpnForResult<double>(aResult, static_cast<qint8>(byte));
            } else {
                value = runRpnForResult<double>(aResult, static_cast<quint8>(byte));
            }

            return QVariant(value);
        } else if (aResult.displayFormat() == "enum") {
            return QVariant(aResult.stringForLevel(num));
        } else {
            const QString ourError = QObject::tr("Unknown display format for byte type: %1").arg(aResult.displayFormat());
            throw std::invalid_argument(qPrintable(ourError));
        }
    }

    QVariant ControlUnit::resultHexStringToVariant(const BasePacketPtr aPacket, const Result &aResult)
    {
        QString hex;
        bool isChkSum = false;
        bool isFull = false;

        QRegularExpression re;
        QRegularExpressionMatch match;
        QString display_format = aResult.displayFormat();

        re.setPattern("^chk_(.*)");
        match = re.match(display_format);
        if (match.hasMatch()) {
            display_format = match.captured(1);
            isChkSum = true;
        }

        re.setPattern("^full_(.*)");
        match = re.match(display_format);
        if (match.hasMatch()) {
            display_format = match.captured(1);
            isFull = true;
        }

        for (int i=0; i < aResult.length(); i++) {
            const unsigned char byte = aPacket->data().at(aResult.startPosition() + i);
            if ((i == 0) and (isChkSum == false) and (isFull == false)) {
                hex.append(QString("%1").arg(QString::number(byte & 0x0f, 16)));
            } else if ((i == aResult.length() - 1) and (isChkSum == true)) {
                hex.append(QChar(byte));
            } else {
                hex.append(QString("%1").arg(QString::number(byte, 16), 2, zeroPadding));
            }
        }

        if (display_format == "int") {
            const quint64 number = hex.toULongLong();
            return QVariant(number);
        } else if (display_format == "string") {
            return QVariant(hex);
        } else {
            const QString ourError = QObject::tr("Unknown display format for hex_string type: %1").arg(aResult.displayFormat());
            throw std::runtime_error(qPrintable(ourError));
        }
    }

    BasePacket::ProtocolType ControlUnit::protocol() const
    {
        return _protocol;
    }
}
