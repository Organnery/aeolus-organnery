# Aeolus 0.9.8 build instructions (GNU/Linux)

To build this version, you need to have the shared
libraries (or newer releases with the same major version):

*  libzita-alsa-pcmi.so.0.3.*
*  libclthreads.so.2.4.*
*  libclxclient.so.3.9.*

and the corresponding header files installed.
They should be available from the same place where
you obtained the Aeolus sources, e.g. Debian packages.

Three binary files will be made:

*  aeolus            (main program) 
*  aeolus_x11.so     (GUI plugin)
*  aeolus_txt.so     (text mode user interface)

In this version the latter is functional but not complete.

The default Makefile will install the Aeolus binary
in /usr/local/bin, and the user interface plugins in
/usr/local/lib (or /usr/local/lib64 on 64-bit systems).

To modify this you can edit the PREFIX definition in
the Makefile. It is not possible to just move the plugins
to another location without recompilation as the path
to them will be compiled into the `aeolus` binary.

To make:

*  cd to the directory containing the source files
*  make
*  (as root) make install

For development, to load the libraries from the
working directory:
```
$ make clean
$ LIBDIR=. make -j6
$ ./aeolus
```

After a successful install you can do a 'make clean' before rebuilding.

Please report any problems (and solutions) to <fons@linuxaudio.org>.

See also the [usage tips](USAGE.md) for run-time configuration.
