/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QSignalSpy>
#include <q3dsqmlstream.h>
#include <QQuickItem>

class tst_Qt3DSQmlStream : public QObject
{
    Q_OBJECT
public:
    tst_Qt3DSQmlStream()
    {
        qRegisterMetaType<QQuickItem*>();
    }
    ~tst_Qt3DSQmlStream() {}
private slots:

    void checkInitialState()
    {
        // GIVEN
        Q3DSQmlStream qmlstream;

        // THEN
        QVERIFY(qmlstream.presentationId().isNull());
        QVERIFY(qmlstream.item() == nullptr);
    }

    void testSetPresentationId()
    {
        // GIVEN
        Q3DSQmlStream qmlstream;
        QString pid = "presentation0";
        QSignalSpy spy(&qmlstream, SIGNAL(presentationIdChanged(const QString&)));

        // WHEN
        qmlstream.setPresentationId(pid);

        // THEN
        QVERIFY(spy.isValid());
        QCOMPARE(qmlstream.presentationId(), pid);
        QCOMPARE(spy.count(), 1);

        // WHEN
        spy.clear();
        qmlstream.setPresentationId(pid);

        // THEN
        QVERIFY(spy.isValid());
        QCOMPARE(spy.count(), 0);
    }

    void testSetItem()
    {
        // GIVEN
        Q3DSQmlStream qmlstream;
        QScopedPointer<QQuickItem> item(new QQuickItem());
        QSignalSpy spy(&qmlstream, SIGNAL(itemChanged(QQuickItem *)));

        // WHEN
        qmlstream.setItem(item.data());

        // THEN
        QVERIFY(spy.isValid());
        QCOMPARE(qmlstream.item(), item.data());
        QCOMPARE(spy.count(), 1);

        // WHEN
        spy.clear();
        qmlstream.setItem(item.data());

        // THEN
        QVERIFY(spy.isValid());
        QCOMPARE(spy.count(), 0);
    }
};

QTEST_APPLESS_MAIN(tst_Qt3DSQmlStream)

#include "tst_qt3dsqmlstream.moc"
