/*
    SPDX-FileCopyrightText: 2016 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/
// Qt
#include <QSignalSpy>
#include <QtTest>
// Frameworks
#include <KIdleTime>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/fakeinput.h>
#include <KWayland/Client/registry.h>

using namespace KWayland::Client;

class IdleTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void init();
    void cleanup();

    void testCatchNextResumeEvent();
    void testTimeout();

private:
    ConnectionThread *m_connection = nullptr;
    FakeInput *m_fakeInput = nullptr;
};

void IdleTest::init()
{
    m_connection = ConnectionThread::fromApplication(this);
    QVERIFY(m_connection);
    Registry registry;
    registry.create(m_connection);
    QSignalSpy interfacesAnnounced(&registry, &Registry::interfacesAnnounced);
    QVERIFY(interfacesAnnounced.isValid());
    registry.setup();
    QVERIFY(interfacesAnnounced.wait());
    const auto fakeInterface = registry.interface(Registry::Interface::FakeInput);
    QVERIFY(fakeInterface.name != 0);
    m_fakeInput = registry.createFakeInput(fakeInterface.name, fakeInterface.version, this);
}

void IdleTest::cleanup()
{
    delete m_fakeInput;
    m_fakeInput = nullptr;
    delete m_connection;
    m_connection = nullptr;
}

void IdleTest::testCatchNextResumeEvent()
{
    // this test uses catch next resume event to get the resuming from idle signal
    QSignalSpy spy(KIdleTime::instance(), &KIdleTime::resumingFromIdle);
    QVERIFY(spy.isValid());
    KIdleTime::instance()->catchNextResumeEvent();
    // on Wayland there is a five sec minimum time
    QTest::qWait(6000);
    // now fake input
    QCOMPARE(spy.count(), 0);
    m_fakeInput->requestPointerMove(QSizeF(1, 2));
    QVERIFY(spy.wait());
}

void IdleTest::testTimeout()
{
    // this test verifies adding a timeout and firing it
    QVERIFY(KIdleTime::instance()->idleTimeouts().isEmpty());
    QSignalSpy timeout1Spy(KIdleTime::instance(), SIGNAL(timeoutReached(int)));
    QVERIFY(timeout1Spy.isValid());
    QSignalSpy timeout2Spy(KIdleTime::instance(), SIGNAL(timeoutReached(int, int)));
    QVERIFY(timeout2Spy.isValid());

    const auto id = KIdleTime::instance()->addIdleTimeout(6000);
    QCOMPARE(KIdleTime::instance()->idleTimeouts().size(), 1);
    QVERIFY(KIdleTime::instance()->idleTimeouts().contains(id));
    QCOMPARE(KIdleTime::instance()->idleTimeouts().value(id), 6000);
    // Wait some time
    QTest::qWait(4000);
    // now we should be able to wait for the timeout
    QVERIFY(timeout1Spy.wait());
    QCOMPARE(timeout1Spy.count(), 1);
    QCOMPARE(timeout2Spy.count(), 1);
    QCOMPARE(timeout1Spy.first().at(0).toInt(), id);
    QCOMPARE(timeout2Spy.first().at(0).toInt(), id);
    QCOMPARE(timeout2Spy.first().at(1).toInt(), 6000);

    // let's fake some input
    // first wait
    QTest::qWait(4000);
    m_fakeInput->requestPointerMove(QSizeF(1, 2));
    QVERIFY(!timeout1Spy.wait());

    // now let's remove the timeout
    KIdleTime::instance()->removeIdleTimeout(id);
    QVERIFY(KIdleTime::instance()->idleTimeouts().isEmpty());
    // now waiting should not trigger the timeout
    QTest::qWait(4000);
    QVERIFY(!timeout1Spy.wait());
}

QTEST_MAIN(IdleTest)
#include "idle_test.moc"
