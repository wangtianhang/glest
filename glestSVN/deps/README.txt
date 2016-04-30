This package  includes the  needed portions  of various  libraries required  to
compile and run Glest.  Various (included) licenses apply.

* Ogg/Vorbis win32sdk v1.0.1
  site: http://www.vorbis.com
  binaries: http://www.vorbis.com/files/1.0.1/windows/oggvorbis-win32sdk-1.0.1.zip
  sources: http://downloads.xiph.org/releases/vorbis
* Apache Xerces C++ Parser v2.8.0 for Windows x86 VC8
  site: http://xerces.apache.org/xerces-c
  download: http://xerces.apache.org/xerces-c/download.cgi
* SGI's OpenGL Sample Implementation SDK
  site: http://oss.sgi.com/projects/ogl-sample
  sources: http://oss.sgi.com/projects/ogl-sample/GLsdk.zip
* Microsoft DirectX SDK Nov 2007

glprocs.c and glprocs.h have been  modified from their original form  using the
GLsdk.patch file included for informational purposes.

I have included static and debug  libraries & dlls where available.  Note  that
pre-compiled binaries  have been  compiled to  the lowest  common architecture,
usually either 486, 586  or 686, which is  really shitty.  If you  want to take
better advantage  of your  processor, I  reccomend ditching  everything in this
package except for  the  direct sound  and GL  stuff,  downloading the  sources
for  oggvorbis-win32sdk,  and  xerces-c  and  re-compiling  them  using  a more
advanced architecture setting, including MMX & SSE instruction sets.
