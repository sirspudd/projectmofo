# projectmofo

Simplified version of [qprojectm-pulseaudio](https://sourceforge.net/projects/projectm/)

Their code is hosted here:

git@github.com:projectM-visualizer/projectm.git

and their license, the LGPL 2.1 is included to reflect this.

Simplified in the sense that the code has been drastically reduced in complexity and accrued layers of cruft. There is now a single QOpenGLWindow which renders the projectm visuals, which makes running this project on the Raspberry Pi (and other platforms where eglfs is the most performant option available). All the pulseaudio handling, which has to be seen to be believed, has been isolated in the abomination.cpp file, where I intend for it to live out its days, dutifully telling me what pulseaudio is hearing and passing this information on to the projectm library.

# Development

Install projectm
Install qt for development (with qmake)
qmake
make

I use Arch Linux on the Raspberry Pi 3 aarch64, and am running this against their packaged projectm to good effect

# Requirements

* Qt
* Full blown OpenGL (vc4 baby. ProjectM has GLES(v1) support, but then you will probably want to compile that yourself too)
