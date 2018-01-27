#include "projectmwindow.h"

#include <abomination.hpp>

#include <QGuiApplication>
#include <QThread>
#include <QMetaObject>

int main ( int argc, char*argv[] )
{
    //Raspberry Pi defaults to 16bpp

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setAlphaBufferSize(0);
    format.setRedBufferSize(8);
    format.setGreenBufferSize(8);
    format.setBlueBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

	QGuiApplication app ( argc, argv );

    ProjectMWindow window;
    window.resize(640, 480);
    window.show();

    return app.exec();
}
