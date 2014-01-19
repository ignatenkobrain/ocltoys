                              jugCLer 1.0
                         A Classic Reanimated

                by Holger Bettag (hobold@vectorizer.org)

Modified to fit OCLToys framework by David "Dade" Bucciarelli (dade916.at.gmail.com).

Welcome to my first OpenCL program! Hopefully by the time you read this, I will
have cleaned up the source code a bit, so that it can serve as a starting point
for your own exploration of general purpose GPU programming with OpenCL.

The juggler was originally created by Eric Graham in 1986 on a Commodore Amiga
home computer. For historical notes, see:

   http://home.comcast.net/~erniew/juggler.html

The juggler was an amazing sight at that time, and might have prompted quite a
few dazzled onlookers to learn more about computer graphics. One of those
people, Michael Birken, much later decided to resurrect the juggler, as a
project to teach programming and mathematics:

  http://meatfighter.com/juggler/

In doing so, he had to reverse engineer the juggler's shape and movements, as
the original model data had never been released to the public.


Some time ago, I had the opportunity to learn CUDA programming, which is
Nvidia's proprietary programming environment for general purpose computing on
their graphics processing units. After I had familiarized myself with that
platform (with what seems to be the standard beginner's project: a Mandelbrot
fractal generator), I went on to write a simple raytracer from scratch, that
could handle the usual mirrored spheres over a checkerboard floor.

The raytracer was a mere toy, but on the GPU it ran blazingly _fast_. It is one
thing to abstractly know that the floating point throughput of modern hardware
is tens or hundreds of millions times higher than it was in the home computer
days. It is a completely different thing to _see_ that performance being put to
use.

Eventually I realized that the juggler would be the perfect messenger to
demonstrate the immense power of modern throughput processors. As an icon of
the past, it would bring back memories of what used to be the latest and
greatest back then, but appears as old as a handaxe today. No one but the
juggler could better embody the stark contrast.

Plus, good ole' juggler would probably bring a smile to those who still
remembered him from their own past. :-)


My own computer has an AMD graphics card, so it cannot run CUDA programs. If I
wanted to see the juggler juggle again in realtime, I had to start over in
OpenCL. So that's what I did, and here is the result: jugCLer 1.0!

This code has been developed on MacOS X, as a command line program using
OpenCL, and OpenGL (via GLUT). I based it on David Bucciarelli's portable
MandelGPU:

  http://davibu.interfree.it/opencl/mandelgpu/mandelGPU.html

So some precautions have been taken for Linux and Windows, as the jugCLer is
ultimately meant to be cross-platform. But no testing has been done as of now,
and all bug reports and suggestions are welcome.

Enjoy!

  Holger
