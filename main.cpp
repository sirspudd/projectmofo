#include "projectmwindow.h"

#include <abomination.hpp>

#include <QGuiApplication>
#include <QThread>
#include <QMetaObject>

#include <QSettings>

int main ( int argc, char*argv[] )
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Chaos Reins");
    app.setApplicationName("projectmofo");

    QSettings settings;

    if (settings.value("force32bpp", true).toBool()) {
        qDebug() << "force 32bpp";
        // Raspberry Pi defaults to 16bpp
        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        format.setAlphaBufferSize(0);
        format.setRedBufferSize(8);
        format.setGreenBufferSize(8);
        format.setBlueBufferSize(8);
        QSurfaceFormat::setDefaultFormat(format);
    } else if (settings.value("force16bpp", true).toBool()) {
        qDebug() << "force 16bpp";
        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        format.setAlphaBufferSize(0);
        format.setRedBufferSize(5);
        format.setGreenBufferSize(6);
        format.setBlueBufferSize(5);
        QSurfaceFormat::setDefaultFormat(format);
    }

    if (settings.value("forceSingleBuffer", false).toBool()) {
        qDebug() << "force single buffer";
        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
        QSurfaceFormat::setDefaultFormat(format);
    } else if (settings.value("forceDoubleBuffer", true).toBool()) {
        qDebug() << "force double buffer";
        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        QSurfaceFormat::setDefaultFormat(format);
    }else if (settings.value("forceTripleBuffer", true).toBool()) {
        qDebug() << "force triple buffer";
        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        format.setSwapBehavior(QSurfaceFormat::TripleBuffer);
        QSurfaceFormat::setDefaultFormat(format);
    }

    ProjectMWindow window;
    window.resize(640, 480);
    window.show();

    return app.exec();
}
