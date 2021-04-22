/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "windoweffects.h"
#include "waylandintegration.h"

#include <QDebug>
#include <QWidget>
#include <QGuiApplication>
#include <QExposeEvent>

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/compositor.h>
#include <KWayland/Client/surface.h>
#include <KWayland/Client/blur.h>
#include <KWayland/Client/contrast.h>
#include <KWayland/Client/region.h>
#include <KWayland/Client/slide.h>

WindowEffects::WindowEffects()
    : QObject(),
      KWindowEffectsPrivate()
{
}

WindowEffects::~WindowEffects()
{}

QWindow *WindowEffects::windowForId(WId wid)
{
    QWindow *window = nullptr;

    for (auto win : qApp->allWindows()) {
        if (win->winId() == wid) {
            window = win;
            break;
        }
    }
    return window;
}

void WindowEffects::trackWindow(QWindow *window)
{
    if (!m_windowWatchers.contains(window)) {
        window->installEventFilter(this);
        auto conn = connect(window, &QObject::destroyed, this, [this, window]() {
            m_blurRegions.remove(window);
            m_backgroundConstrastRegions.remove(window);
            m_windowWatchers.remove(window);
        });
        m_windowWatchers[window] = conn;
    }
}

void WindowEffects::releaseWindow(QWindow *window)
{
    if (!m_blurRegions.contains(window) && !m_backgroundConstrastRegions.contains(window)) {
        disconnect(m_windowWatchers[window]);
        window->removeEventFilter(this);
        m_windowWatchers.remove(window);
    }
}

bool WindowEffects::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Expose) {
        auto ee = static_cast<QExposeEvent*>(event);

        if ((ee->region().isNull())) {
            return false;
        }

        auto window = qobject_cast<QWindow *>(watched);
        if (!window) {
            return false;
        }

        {
            auto it = m_blurRegions.constFind(window);
            if (it != m_blurRegions.constEnd()) {
                enableBlurBehind(window, true, *it);
            }
        }
        {
            auto it = m_backgroundConstrastRegions.constFind(window);
            if (it != m_backgroundConstrastRegions.constEnd()) {
                enableBackgroundContrast(window, true, it->contrast, it->intensity, it->saturation, it->region);
            }
        }
    }
    return false;
}

bool WindowEffects::isEffectAvailable(KWindowEffects::Effect effect)
{
    switch (effect) {
    case KWindowEffects::BackgroundContrast:
        return WaylandIntegration::self()->waylandContrastManager() != nullptr;
    case KWindowEffects::BlurBehind:
        return WaylandIntegration::self()->waylandBlurManager() != nullptr;
    case KWindowEffects::Slide:
        return WaylandIntegration::self()->waylandSlideManager() != nullptr;
    default:
        return false;
    }
}

void WindowEffects::slideWindow(WId id, KWindowEffects::SlideFromLocation location, int offset)
{
    if (!WaylandIntegration::self()->waylandSlideManager()) {
        return;
    }
    KWayland::Client::Surface *surface = KWayland::Client::Surface::fromQtWinId(id);
    if (surface) {
        if (location != KWindowEffects::SlideFromLocation::NoEdge) {
            auto slide = WaylandIntegration::self()->waylandSlideManager()->createSlide(surface, surface);

            KWayland::Client::Slide::Location convertedLoc;
            switch (location) {
            case KWindowEffects::SlideFromLocation::TopEdge:
                convertedLoc = KWayland::Client::Slide::Location::Top;
                break;
            case KWindowEffects::SlideFromLocation::LeftEdge:
                convertedLoc = KWayland::Client::Slide::Location::Left;
                break;
            case KWindowEffects::SlideFromLocation::RightEdge:
                convertedLoc = KWayland::Client::Slide::Location::Right;
                break;
            case KWindowEffects::SlideFromLocation::BottomEdge:
            default:
                convertedLoc = KWayland::Client::Slide::Location::Bottom;
                break;
            }

            slide->setLocation(convertedLoc);
            slide->setOffset(offset);
            slide->commit();
        } else {
            WaylandIntegration::self()->waylandSlideManager()->removeSlide(surface);
        }

        WaylandIntegration::self()->waylandConnection()->flush();
    }
}


#if KWINDOWSYSTEM_BUILD_DEPRECATED_SINCE(5, 81)
QList<QSize> WindowEffects::windowSizes(const QList<WId> &ids)
{
    Q_UNUSED(ids)
    QList<QSize> sizes;
    return sizes;
}
#endif

void WindowEffects::presentWindows(WId controller, const QList<WId> &ids)
{
    Q_UNUSED(controller)
    Q_UNUSED(ids)
}

void WindowEffects::presentWindows(WId controller, int desktop)
{
    Q_UNUSED(controller)
    Q_UNUSED(desktop)
}

void WindowEffects::highlightWindows(WId controller, const QList<WId> &ids)
{
    Q_UNUSED(controller)
    Q_UNUSED(ids)
}

void WindowEffects::enableBlurBehind(WId winId, bool enable, const QRegion &region)
{
    auto window = windowForId(winId);
    if (!window) {
        return;
    }
    if (enable) {
        trackWindow(window);
        m_blurRegions[window] = region;
    } else {
        m_blurRegions.remove(window);
        releaseWindow(window);
    }

    enableBlurBehind(window, enable, region);
}

void WindowEffects::enableBlurBehind(QWindow *window, bool enable, const QRegion &region)
{
    if (!WaylandIntegration::self()->waylandBlurManager()) {
        return;
    }
    KWayland::Client::Surface *surface = KWayland::Client::Surface::fromWindow(window);

    if (surface) {
        if (enable) {
            auto blur = WaylandIntegration::self()->waylandBlurManager()->createBlur(surface, surface);
            blur->setRegion(WaylandIntegration::self()->waylandCompositor()->createRegion(region, nullptr));
            blur->commit();
        } else {
            WaylandIntegration::self()->waylandBlurManager()->removeBlur(surface);
        }

        WaylandIntegration::self()->waylandConnection()->flush();
    }
}

void WindowEffects::enableBackgroundContrast(WId winId, bool enable, qreal contrast, qreal intensity, qreal saturation, const QRegion &region)
{
    auto window = windowForId(winId);
    if (!window) {
        return;
    }
    if (enable) {
        trackWindow(window);
        m_backgroundConstrastRegions[window].contrast = contrast;
        m_backgroundConstrastRegions[window].intensity = intensity;
        m_backgroundConstrastRegions[window].saturation = saturation;
        m_backgroundConstrastRegions[window].region = region;
    } else {
        m_backgroundConstrastRegions.remove(window);
        releaseWindow(window);
    }

    enableBackgroundContrast(window, enable, contrast, intensity, saturation, region);
}

void WindowEffects::enableBackgroundContrast(QWindow *window, bool enable, qreal contrast, qreal intensity, qreal saturation, const QRegion &region)
{
    if (!WaylandIntegration::self()->waylandContrastManager()) {
        return;
    }
    KWayland::Client::Surface *surface = KWayland::Client::Surface::fromWindow(window);
    if (surface) {
        if (enable) {
            auto backgroundContrast = WaylandIntegration::self()->waylandContrastManager()->createContrast(surface, surface);
            backgroundContrast->setRegion(WaylandIntegration::self()->waylandCompositor()->createRegion(region, nullptr));
            backgroundContrast->setContrast(contrast);
            backgroundContrast->setIntensity(intensity);
            backgroundContrast->setSaturation(saturation);
            backgroundContrast->commit();
        } else {
            WaylandIntegration::self()->waylandContrastManager()->removeContrast(surface);
        }

        WaylandIntegration::self()->waylandConnection()->flush();
    }
}

#if KWINDOWSYSTEM_BUILD_DEPRECATED_SINCE(5, 67)
void WindowEffects::markAsDashboard(WId window)
{
    Q_UNUSED(window)
}
#endif
