#include "projectmwindow.h"

#include <abomination.hpp>

#include <QGuiApplication>
#include <QThread>
#include <QMetaObject>

#include <QSettings>

int main ( int argc, char*argv[] )
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOverrideCursor(Qt::BlankCursor);
    app.setOrganizationName("Chaos Reins");
    app.setApplicationName("projectmofo");

    QSettings settings;

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    if (settings.value("force24bpp", true).toBool()) {
        qDebug() << "force 24bpp";
        // Raspberry Pi defaults to 16bpp
        format.setAlphaBufferSize(0);
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
    } else if (settings.value("force16bpp", true).toBool()) {
        qDebug() << "force 16bpp";
        format.setAlphaBufferSize(0);
        format.setRedBufferSize(5);
        format.setGreenBufferSize(6);
        format.setBlueBufferSize(5);
    }

    if (settings.value("forceSingleBuffer", false).toBool()) {
        qDebug() << "force single buffer";
        format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
    } else if (settings.value("forceDoubleBuffer", true).toBool()) {
        qDebug() << "force double buffer";
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    } else if (settings.value("forceTripleBuffer", true).toBool()) {
        qDebug() << "force triple buffer";
        format.setSwapBehavior(QSurfaceFormat::TripleBuffer);
    }
    QSurfaceFormat::setDefaultFormat(format);

    bool fullscreen = settings.value("fullscreen", false).toBool();
    settings.setValue("fullscreen", fullscreen);

    ProjectMWindow window;
    if (fullscreen) {
        window.showFullScreen();
    } else {
        window.resize(640, 480);
        window.show();
    }

    return app.exec();
}
