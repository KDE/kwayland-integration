/*
    SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "windowshadow.h"
#include "logging.h"
#include "waylandintegration.h"

#include <KWayland/Client/shm_pool.h>
#include <KWayland/Client/surface.h>

#include <wayland-client-core.h>

#include <QDebug>
#include <QExposeEvent>

WindowShadowTile::WindowShadowTile() {}
WindowShadowTile::~WindowShadowTile() {}

bool WindowShadowTile::create()
{
    m_shmPool.reset(WaylandIntegration::self()->createShmPool());
    if (!m_shmPool) {
        return false;
    }
    buffer = m_shmPool->createBuffer(image);
    return true;
}

void WindowShadowTile::destroy()
{
    buffer = nullptr;
}

WindowShadowTile *WindowShadowTile::get(const KWindowShadowTile *tile)
{
    auto d = static_cast<WindowShadowTile *>(KWindowShadowTilePrivate::get(tile));

    if (d && d->buffer && wl_proxy_get_is_defunct((wl_proxy *)d->buffer.toStrongRef()->buffer())) {
        d->destroy();
        d->create();
    }
    return d;
}

static KWayland::Client::Buffer::Ptr bufferForTile(const KWindowShadowTile::Ptr &tile)
{
    if (!tile) {
        return KWayland::Client::Buffer::Ptr();
    }
    WindowShadowTile *d = WindowShadowTile::get(tile.data());
    return d->buffer;
}

bool WindowShadow::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)
    if (event->type() == QEvent::Expose) {
        QExposeEvent *exposeEvent = static_cast<QExposeEvent *>(event);
        if (!exposeEvent->region().isNull()) {
            if (!internalCreate()) {
                qCWarning(KWAYLAND_KWS) << "Failed to recreate shadow for" << window;
            }
        }
    } else if (event->type() == QEvent::Hide) {
        internalDestroy();
    }
    return false;
}

bool WindowShadow::internalCreate()
{
    if (shadow) {
        return true;
    }
    KWayland::Client::ShadowManager *shadowManager = WaylandIntegration::self()->waylandShadowManager();
    if (!shadowManager) {
        return false;
    }
    KWayland::Client::Surface *surface = KWayland::Client::Surface::fromWindow(window);
    if (!surface) {
        return false;
    }

    shadow = shadowManager->createShadow(surface, surface);
    shadow->attachLeft(bufferForTile(leftTile));
    shadow->attachTopLeft(bufferForTile(topLeftTile));
    shadow->attachTop(bufferForTile(topTile));
    shadow->attachTopRight(bufferForTile(topRightTile));
    shadow->attachRight(bufferForTile(rightTile));
    shadow->attachBottomRight(bufferForTile(bottomRightTile));
    shadow->attachBottom(bufferForTile(bottomTile));
    shadow->attachBottomLeft(bufferForTile(bottomLeftTile));
    shadow->setOffsets(padding);
    shadow->commit();

    // Commit wl_surface at the next available time.
    window->requestUpdate();

    return true;
}

bool WindowShadow::create()
{
    if (!internalCreate()) {
        return false;
    }
    window->installEventFilter(this);
    return true;
}

void WindowShadow::internalDestroy()
{
    if (!shadow) {
        return;
    }

    KWayland::Client::ShadowManager *shadowManager = WaylandIntegration::self()->waylandShadowManager();
    if (shadowManager) {
        KWayland::Client::Surface *surface = KWayland::Client::Surface::fromWindow(window);
        if (surface) {
            shadowManager->removeShadow(surface);
        }
    }

    delete shadow;
    shadow = nullptr;

    if (window) {
        window->requestUpdate();
    }
}

void WindowShadow::destroy()
{
    if (window) {
        window->removeEventFilter(this);
    }
    internalDestroy();
}
