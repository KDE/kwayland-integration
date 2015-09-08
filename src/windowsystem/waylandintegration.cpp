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

#include "waylandintegration.h"
#include "logging.h"

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/compositor.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/surface.h>
#include <KWayland/Client/blur.h>
#include <KWayland/Client/contrast.h>
#include <KWayland/Client/region.h>

#include <KWindowSystem/KWindowSystem>

class WaylandIntegrationSingleton
{
public:
    WaylandIntegration self;
};

Q_GLOBAL_STATIC(WaylandIntegrationSingleton, privateWaylandIntegrationSelf)

WaylandIntegration::WaylandIntegration()
    : QObject(),
      m_waylandConnection(nullptr),
      m_waylandBlurManager(nullptr),
      m_waylandCompositor(nullptr)
{
    setupKWaylandIntegration();
}

WaylandIntegration::~WaylandIntegration()
{}

void WaylandIntegration::setupKWaylandIntegration()
{
    using namespace KWayland::Client;
    m_waylandConnection = ConnectionThread::fromApplication(this);
    if (!m_waylandConnection) {
        qCWarning(KWAYLAND_KWS) << "Failed getting Wayland connection from QPA";
        return;
    }
    m_registry = new Registry(this);
    m_registry->create(m_waylandConnection);
    m_waylandCompositor = Compositor::fromApplication(this);
    connect(m_registry, &Registry::blurAnnounced, this,
        [this] (quint32 name, quint32 version) {
            m_waylandBlurManager = m_registry->createBlurManager(name, version, this);

            connect(m_waylandBlurManager, &BlurManager::removed, this,
                [this] () {
                    m_waylandBlurManager->deleteLater();
                    m_waylandBlurManager = nullptr;
                }
            );
        }
    );
    connect(m_registry, &Registry::contrastAnnounced, this,
        [this] (quint32 name, quint32 version) {
            m_waylandContrastManager = m_registry->createContrastManager(name, version, this);

            connect(m_waylandContrastManager, &ContrastManager::removed, this,
                [this] () {
                    m_waylandContrastManager->deleteLater();
                    m_waylandContrastManager = nullptr;
                }
            );
        }
    );

    connect(m_registry, &Registry::interfacesAnnounced, this,
        [this] {
            if (!m_wm) {
                qCWarning(KWAYLAND_KWS) << "This compositor does not support the Plasma Window Management interface";
            }
        }
    );

    connect(m_registry, &Registry::plasmaShellAnnounced, this,
        [this] (quint32 name, quint32 version) {
            m_waylandPlasmaShell = m_registry->createPlasmaShell(name, version, this);
        }
    );

    m_registry->setup();
    m_waylandConnection->roundtrip();
}

WaylandIntegration *WaylandIntegration::self()
{
    return &privateWaylandIntegrationSelf()->self;
}


KWayland::Client::ConnectionThread *WaylandIntegration::waylandConnection() const
{
    return m_waylandConnection;
}

KWayland::Client::BlurManager *WaylandIntegration::waylandBlurManager() const
{
    return m_waylandBlurManager;
}

KWayland::Client::ContrastManager *WaylandIntegration::waylandContrastManager() const
{
    return m_waylandContrastManager;
}

KWayland::Client::Compositor *WaylandIntegration::waylandCompositor() const
{
    return m_waylandCompositor;
}

KWayland::Client::PlasmaWindowManagement *WaylandIntegration::plasmaWindowManagement()
{
    using namespace KWayland::Client;

    if (!m_wm) {
        connect(m_registry, &Registry::plasmaWindowManagementAnnounced, this,
            [this] (quint32 name, quint32 version) {
                m_wm = m_registry->createPlasmaWindowManagement(name, version, this);
                connect(m_wm, &PlasmaWindowManagement::windowCreated, this,
                    [this] (PlasmaWindow *w) {
                        emit KWindowSystem::self()->windowAdded(w->internalId());
                        emit KWindowSystem::self()->stackingOrderChanged();
                        connect(w, &PlasmaWindow::unmapped, this,
                            [w] {
                                emit KWindowSystem::self()->windowRemoved(w->internalId());
                                emit KWindowSystem::self()->stackingOrderChanged();
                            }
                        );
                    }
                );
                connect(m_wm, &PlasmaWindowManagement::activeWindowChanged, this,
                    [this] {
                        if (PlasmaWindow *w = m_wm->activeWindow()) {
                            emit KWindowSystem::self()->activeWindowChanged(w->internalId());
                        } else {
                            emit KWindowSystem::self()->activeWindowChanged(0);
                        }
                    }
                );
                connect(m_wm, &PlasmaWindowManagement::showingDesktopChanged, KWindowSystem::self(), &KWindowSystem::showingDesktopChanged);
                emit KWindowSystem::self()->compositingChanged(true);
                emit KWindowSystem::self()->showingDesktopChanged(m_wm->isShowingDesktop());
                emit KWindowSystem::self()->stackingOrderChanged();
                if (PlasmaWindow *w = m_wm->activeWindow()) {
                    emit KWindowSystem::self()->activeWindowChanged(w->internalId());
                }
                qCDebug(KWAYLAND_KWS) << "Plasma Window Management interface bound";
            }
        );
    }
    
    return m_wm;
}

KWayland::Client::PlasmaShell *WaylandIntegration::waylandPlasmaShell() const
{
    return m_waylandPlasmaShell;
}
