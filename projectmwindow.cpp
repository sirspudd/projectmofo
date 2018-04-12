#include "projectmwindow.h"

#include <libprojectM/projectM.hpp>

#include <abomination.hpp>

#include <QStandardPaths>

std::string ProjectMWindow::configPath = QString(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first() + "/.projectM/config.inp").toStdString();

ProjectMWindow::ProjectMWindow()
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate)
{
    pulseReader = new QThread(this);
    AbominationFromTheDarkLordsTailPipe *pulseAudioGremlin = AbominationFromTheDarkLordsTailPipe::instance(this);
    pulseAudioGremlin->moveToThread(pulseReader);
    QMetaObject::invokeMethod(pulseAudioGremlin, "run", Qt::QueuedConnection);
    connect(pulseAudioGremlin, &AbominationFromTheDarkLordsTailPipe::pcmDataGenerated, this, &ProjectMWindow::forwardPCMfloat, Qt::QueuedConnection);
}

ProjectMWindow::~ProjectMWindow()
{
    delete projectMInstance;
    pulseReader->exit();
    delete pulseReader;
}

void ProjectMWindow::forwardPCMfloat(float *PCMdata, int samples)
{
    projectMInstance->pcm()->addPCMfloat(PCMdata, samples);
}

void ProjectMWindow::paintGL() {
    projectMInstance->renderFrame();
    update();
}

void ProjectMWindow::resizeGL(int w, int h) {
    projectMInstance = new projectM(configPath);
    init(w, h);
    projectMInstance->projectM_resetGL (w, h);
    pulseReader->start();
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
