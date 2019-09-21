#include "projectmwindow.h"
#include "abomination.hpp"

#include <libprojectM/projectM.hpp>

#include <QStandardPaths>
#include <QCoreApplication>

std::string ProjectMWindow::configPath = QString(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "/.projectM/config.inp").toStdString();

ProjectMWindow::ProjectMWindow()
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate)
{
    pulseReader = new QThread(this);
    AbominationFromTheDarkLordsTailPipe *pulseAudioGremlin = AbominationFromTheDarkLordsTailPipe::instance();
    connect(pulseAudioGremlin, &AbominationFromTheDarkLordsTailPipe::pcmDataGenerated, this, &ProjectMWindow::forwardPCMfloat, Qt::QueuedConnection);
    pulseAudioGremlin->moveToThread(pulseReader);
    QMetaObject::invokeMethod(pulseAudioGremlin, "run", Qt::QueuedConnection);
}

ProjectMWindow::~ProjectMWindow()
{
    pulseReader->exit();
    pulseReader->wait(10);

    delete projectMInstance;
    delete pulseReader;
}

void ProjectMWindow::forwardPCMfloat(float *PCMdata, int samples)
{
    if (projectMInstance) {
        projectMInstance->pcm()->addPCMfloat(PCMdata, samples);
    }
}

void ProjectMWindow::initialize()
{
    if (!projectMInstance) {
        projectMInstance = new projectM(configPath);
        resizeGL(width(), height());
        paintGL();
        pulseReader->start();
        projectMInstance->selectRandom(true);
    }
}

void ProjectMWindow::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_F) {
        if (windowState() == Qt::WindowFullScreen) {
            show();
        } else {
            showFullScreen();
        }
    } else if (ev->key() == Qt::Key_Escape) {
        qApp->quit();
    } else if (ev->key() == Qt::Key_Right) {
        projectMInstance->selectRandom(true);
    }
    QOpenGLWindow::keyPressEvent(ev);
}

void ProjectMWindow::paintGL() {
    if (projectMInstance) {
        projectMInstance->renderFrame();
        update();
    }
    QOpenGLWindow::paintGL();
}

void ProjectMWindow::resizeGL(int w, int h) {
    if (!projectMInstance) {
        QMetaObject::invokeMethod(this, "initialize", Qt::QueuedConnection);
    } else {
        projectMInstance->projectM_resetGL(w, h);
        init(w, h);
    }
    QOpenGLWindow::resizeGL(w, h);
}

void ProjectMWindow::init(int w, int h)
{
    // So follows stock crud taken from qprojectmwidget
    // adopted wholesale from qprojectmwidget.hpp

    /* Our shading model--Gouraud (smooth). */
    glShadeModel ( GL_SMOOTH );
    /* Culling. */
    //    glCullFace( GL_BACK );
    //    glFrontFace( GL_CCW );
    //    glEnable( GL_CULL_FACE );
    /* Set the clear color. */
    glClearColor ( 0, 0, 0, 0 );
    /* Setup our viewport. */
    glViewport ( 0, 0, w, h );
    /*
            * Change to the projection matrix and set
            * our viewing volume.
    */
    glMatrixMode ( GL_TEXTURE );
    glLoadIdentity();

    //    gluOrtho2D(0.0, (GLfloat) width, 0.0, (GLfloat) height);
    glMatrixMode ( GL_PROJECTION );
    glLoadIdentity();

    //    glFrustum(0.0, height, 0.0,width,10,40);
    glMatrixMode ( GL_MODELVIEW );
    glLoadIdentity();

    glDrawBuffer ( GL_BACK );
    glReadBuffer ( GL_BACK );
    glEnable ( GL_BLEND );

    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable ( GL_LINE_SMOOTH );
    glEnable ( GL_POINT_SMOOTH );
    glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );
//   glClear(GL_COLOR_BUFFER_BIT);

    // glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGB,0,0,texsize,texsize,0);
    //glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,texsize,texsize);
    glLineStipple ( 2, 0xAAAA );
}
