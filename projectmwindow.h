#pragma once

#include <string>

#include <QOpenGLWindow>

class AbominationFromTheDarkLordsTailPipe;
class projectM;

class ProjectMWindow : public QOpenGLWindow
{
    Q_OBJECT
public:
    ProjectMWindow();
    ~ProjectMWindow();

public slots:
    void forwardPCMfloat(float *PCMdata, int samples);
    void initialize();

protected:
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    void init(int w, int h);

    QThread *pulseReader = nullptr;
    static std::string configPath;
    projectM *projectMInstance = nullptr;
};
