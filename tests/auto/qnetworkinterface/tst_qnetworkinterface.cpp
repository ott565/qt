/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qnetworkinterface.h>
#include <qtcpsocket.h>
#include "../network-settings.h"

//TESTED_FILES=qnetworkinterface.cpp qnetworkinterface.h qnetworkinterface_unix.cpp qnetworkinterface_win.cpp

class tst_QNetworkInterface : public QObject
{
    Q_OBJECT

public:
    tst_QNetworkInterface();
    virtual ~tst_QNetworkInterface();

private slots:
    void dump();
    void loopbackIPv4();
    void loopbackIPv6();
    void localAddress();
    void interfaceFromXXX();
    void copyInvalidInterface();
};

tst_QNetworkInterface::tst_QNetworkInterface()
{
    Q_SET_DEFAULT_IAP
}

tst_QNetworkInterface::~tst_QNetworkInterface()
{
}


void tst_QNetworkInterface::dump()
{
    // This is for manual testing:
    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    foreach (const QNetworkInterface &i, allInterfaces) {
        QString flags;
        if (i.flags() & QNetworkInterface::IsUp) flags += "Up,";
        if (i.flags() & QNetworkInterface::IsRunning) flags += "Running,";
        if (i.flags() & QNetworkInterface::CanBroadcast) flags += "Broadcast,";
        if (i.flags() & QNetworkInterface::IsLoopBack) flags += "Loopback,";
        if (i.flags() & QNetworkInterface::IsPointToPoint) flags += "PointToPoint,";
        if (i.flags() & QNetworkInterface::CanMulticast) flags += "Multicast,";
        flags.chop(1);          // drop last comma

        QString friendlyName = i.humanReadableName();
        if (friendlyName != i.name()) {
            friendlyName.prepend('(');
            friendlyName.append(')');
        } else {
            friendlyName.clear();
        }
        qDebug() << "Interface:     " << i.name() << qPrintable(friendlyName);
        QVERIFY(i.isValid());

        qDebug() << "    index:     " << i.index();
        qDebug() << "    flags:     " << qPrintable(flags);
        qDebug() << "    hw address:" << qPrintable(i.hardwareAddress());

        int count = 0;
        foreach (const QNetworkAddressEntry &e, i.addressEntries()) {
            QDebug s = qDebug();
            s.nospace() <<    "    address "
                        << qSetFieldWidth(2) << count++ << qSetFieldWidth(0);
            s.nospace() << ": " << qPrintable(e.ip().toString());
            if (!e.netmask().isNull())
                s.nospace() << '/' << e.prefixLength()
                            << " (" << qPrintable(e.netmask().toString()) << ")";
            if (!e.broadcast().isNull())
                s.nospace() << " broadcast " << qPrintable(e.broadcast().toString());
        }
    }
}

void tst_QNetworkInterface::loopbackIPv4()
{
    QList<QHostAddress> all = QNetworkInterface::allAddresses();
    QVERIFY(all.contains(QHostAddress(QHostAddress::LocalHost)));
}

void tst_QNetworkInterface::loopbackIPv6()
{
#ifdef Q_OS_SYMBIAN
    QSKIP( "Symbian: IPv6 is not yet supported", SkipAll );
#else

    QList<QHostAddress> all = QNetworkInterface::allAddresses();

    bool loopbackfound = false;
    bool anyIPv6 = false;
    foreach (QHostAddress addr, all)
        if (addr == QHostAddress::LocalHostIPv6) {
            loopbackfound = true;
            anyIPv6 = true;
            break;
        } else if (addr.protocol() == QAbstractSocket::IPv6Protocol)
            anyIPv6 = true;

    QVERIFY(!anyIPv6 || loopbackfound);
#endif
}

void tst_QNetworkInterface::localAddress()
{
    QTcpSocket socket;
    socket.connectToHost(QtNetworkSettings::serverName(), 80);
    QVERIFY(socket.waitForConnected(5000));

    QHostAddress local = socket.localAddress();

    // make Apache happy on fluke
    socket.write("GET / HTTP/1.0\r\n\r\n");
    socket.waitForBytesWritten(1000);
    socket.close();

    // test that we can find the address that QTcpSocket reported
    QList<QHostAddress> all = QNetworkInterface::allAddresses();
    QVERIFY(all.contains(local));
}

void tst_QNetworkInterface::interfaceFromXXX()
{
    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();

    foreach (QNetworkInterface iface, allInterfaces) {
        QVERIFY(QNetworkInterface::interfaceFromName(iface.name()).isValid());
        foreach (QNetworkAddressEntry entry, iface.addressEntries()) {
            QVERIFY(!entry.ip().isNull());

            if (!entry.netmask().isNull()) {
                QCOMPARE(entry.netmask().protocol(), entry.ip().protocol());

                // if the netmask is known, the broadcast is known
                // but only for IPv4 (there is no such thing as broadcast in IPv6)
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    QVERIFY(!entry.broadcast().isNull());

                    // verify that the broadcast address is correct
                    quint32 ip = entry.ip().toIPv4Address();
                    quint32 mask = entry.netmask().toIPv4Address();
                    quint32 bcast = entry.broadcast().toIPv4Address();

                    QCOMPARE(bcast, ip | ~mask);
                }
            }

            if (!entry.broadcast().isNull())
                QCOMPARE(entry.broadcast().protocol(), entry.ip().protocol());
        }
    }
}

void tst_QNetworkInterface::copyInvalidInterface()
{
    // Force a copy of an interfaces that isn't likely to exist
    QNetworkInterface i = QNetworkInterface::interfaceFromName("plopp");
    QVERIFY(!i.isValid());

    QCOMPARE(i.index(), 0);
    QVERIFY(i.name().isEmpty());
    QVERIFY(i.humanReadableName().isEmpty());
    QVERIFY(i.hardwareAddress().isEmpty());
    QCOMPARE(int(i.flags()), 0);
    QVERIFY(i.addressEntries().isEmpty());
}

QTEST_MAIN(tst_QNetworkInterface)
#include "tst_qnetworkinterface.moc"
