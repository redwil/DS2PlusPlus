#include <stdexcept>

#include <QDebug>

#include "result.h"

namespace DS2PlusPlus {
    void Result::setUuid(const QString &aUuid)
    {
        _uuid = aUuid;
    }

    const QString Result::uuid() const
    {
        return _uuid;
    }

    void Result::setName(const QString &aName)
    {
        _name = aName;
    }

    const QString Result::name() const
    {
        return _name;
    }

    void Result::setType(const QString &aType)
    {
        _type = aType;
    }

    const QString Result::type() const
    {
        return _type;
    }

    bool Result::isType(const QString &aType) const
    {
        return _type == aType;
    }

    void Result::setDisplayFormat(const QString &aDisplayFormat)
    {
        _displayFormat = aDisplayFormat;
    }

    const QString Result::displayFormat() const
    {
        return _displayFormat;
    }

    void Result::setStartPosition(int aStartPosition)
    {
        _startPosition = aStartPosition;
    }

    int Result::startPosition() const
    {
        return _startPosition;
    }

    void Result::setLength(int aLength)
    {
        _length = aLength;
    }

    int Result::length() const
    {
        return _length;
    }

    void Result::setMask(const QString &aMask)
    {
        if (aMask.isEmpty()) {
            _mask = 0xff;
        } else {
            bool ok;
            _mask = aMask.toUInt(&ok, 16);
            if (!ok) {
                qDebug() << "Problem setting result mask: " << ok << " for " << aMask;
            }
        }
    }

    int Result::mask() const
    {
        return _mask;
    }

    void Result::setFactorA(double aFactor)
    {
        _factorA = aFactor;
    }

    double Result::factorA() const
    {
        return _factorA;
    }

    void Result::setFactorB(double aFactor)
    {
        _factorB = aFactor;
    }

    double Result::factorB() const
    {
        return _factorB;
    }

    void Result::setLevels(QHash<QString, QString> someLevels)
    {
        _levels = someLevels;
    }

    const QString Result::stringForLevel(quint8 aLevel) const
    {
        // Boolean
        if (_levels.contains("yes") and _levels.contains("no") and _levels.count() == 2) {
                return (aLevel == true) ? _levels.value("yes") : _levels.value("no");
        }

        qint32 ourLevelCount = 0;
        QStringList ourLevels;
        for (QHash<QString, QString>::const_iterator it = _levels.begin(); it != _levels.end(); ++it) {
            if ((it.key() == "else") or (it.key() == "all")) {
                continue;
            }

            ourLevelCount++;

            bool ok;
            quint8 mask = it.key().toUShort(&ok, 16);

            if (!ok) {
                throw std::invalid_argument("Invalid mask found");
            }

            if ((aLevel & mask) > 0) {
                ourLevels.append(it.value());
            }
        }

        if ((ourLevels.isEmpty()) and (_levels.contains("else"))) {
            return _levels.value("else");
        }

        if (ourLevels.isEmpty()) {
            return QString::null;
        }

        if ((ourLevels.count() == ourLevelCount) and _levels.contains("all")) {
            return _levels.value("all");
        }

        return ourLevels.join(",");
    }
}
