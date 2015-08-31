/*
 * Copyright 2014 Martin Gräßlin <mgraesslin@kde.org>
 * Copyright 2015 Marco Martin <mart@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "windoweffects.h"

#include <QDebug>
#include <QGuiApplication>

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/compositor.h>
#include <KWayland/Client/surface.h>
#include <KWayland/Client/blur.h>
#include <KWayland/Client/region.h>

WindowEffects::WindowEffects()
    : KWindowEffectsPrivate(),
      QObject(0),
      m_waylandConnection(0),
      m_waylandBlurManager(0),
      m_waylandCompositor(0),
      m_blurSupported(false)
{
    setupKWaylandIntegration();
}

WindowEffects::~WindowEffects()
{}

void WindowEffects::setupKWaylandIntegration()
{
    if (!QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive)) {
        return;
    }
    using namespace KWayland::Client;
    m_waylandConnection = ConnectionThread::fromApplication(this);
    if (!m_waylandConnection) {
        return;
    }
    Registry *registry = new Registry(this);
    registry->create(m_waylandConnection);
    connect(registry, &Registry::compositorAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            m_waylandCompositor = registry->createCompositor(name, version, this);
        }
    );
    connect(registry, &Registry::blurAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            m_waylandBlurManager = registry->createBlurManager(name, version, this);
            m_blurSupported = true;
        }
    );
    registry->setup();
}

bool WindowEffects::isEffectAvailable(KWindowEffects::Effect effect)
{
    switch (effect) {
    //TODO: implement BackgroundContrast
    //FIXME: isEffectAvailable gets asked for before blurAnnounced is emitted;
    case KWindowEffects::BackgroundContrast:
        return true;
    case KWindowEffects::BlurBehind:
        return m_blurSupported;
    default:
        return false;
    }
}

void WindowEffects::slideWindow(WId id, KWindowEffects::SlideFromLocation location, int offset)
{
    
}

void WindowEffects::slideWindow(QWidget *widget, KWindowEffects::SlideFromLocation location)
{
    
}

QList<QSize> WindowEffects::windowSizes(const QList<WId> &ids)
{
    QList<QSize> sizes;
    return sizes;
}

void WindowEffects::presentWindows(WId controller, const QList<WId> &ids)
{
    
}

void WindowEffects::presentWindows(WId controller, int desktop)
{
    
}

void WindowEffects::highlightWindows(WId controller, const QList<WId> &ids)
{
    
}

void WindowEffects::enableBlurBehind(WId window, bool enable, const QRegion &region)
{
    if (!m_waylandBlurManager) {
        return;
    }
    KWayland::Client::Surface *surface = KWayland::Client::Surface::fromQtWinId(window);
    if (surface) {
        auto blur = m_waylandBlurManager->createBlur(surface, surface);
        blur->setRegion(*m_waylandCompositor->createRegion(region));
        blur->commit();
        surface->commit(KWayland::Client::Surface::CommitFlag::None);

        m_waylandConnection->flush();
    }
}

void WindowEffects::enableBackgroundContrast(WId window, bool enable, qreal contrast, qreal intensity, qreal saturation, const QRegion &region)
{
    
}

void WindowEffects::markAsDashboard(WId window)
{
    
}


