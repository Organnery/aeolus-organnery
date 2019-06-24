# Debian packaging instructions

The following instructions have been tested on Debian sid
and buster build hosts. Older build hosts (e.g. Debian
stretch) are not recommended, as their tools are getting
old now.

## Development

Please note, the 'libjack..' package does not currently
work for Debian sid (unstable) as of 8th May 2019.

To build and run a modified Aeolus from this git repository
on the local machine, first remove any official Debian
package of 'aeolus'. Removing Debian's package 'stops' is
optional, as you will need some stops to test Aeolus.
```
sudo apt purge aeolus
```

Change to the directory containing the Aeolus source, then
clean, build and install:
```
cd aeolus/source
make clean
make
sudo make install
```

Then test the new `aeolus` binary before packaging it, for
example on a desktop PC running PulseAudio:
```
pasuspender -- aeolus -A -d hw:1 -S /usr/share/aeolus/stops/
aconnect 24:0 128:0
```

## Packaging paths
------------------

Aeolus permits packagers to install a working binary without
touching a users's home directory, as follows:

*  Install the aeolus binary in /usr/bin and the two plugins in /usr/lib
*  Install the stops directory in /usr/share/aeolus (Debian packages stops separately).
*  Install documentation into /usr/share/doc/aeolus
*  Create the file /etc/aeolus.conf containing, for example:

```
# Aeolus default options
-u -A -S /usr/share/aeolus/stops
```

These options will use the default instrument 'Aeolus', save
presets in `.aeolus-presets` in the users's home directory,
use ALSA output, and use the packager's default stops.

## Debian packaging for production

As well as the build dependencies for aeolus itself, the
`build-essential`, `debhelper` & `pkgconf` packages are
required to build Debian packages. (`pkgconf` is a
replacement for `pkg-config`). You can install all these
packages with the command:
```
sudo apt install build-essential pkgconf debhelper libasound2-dev libclthreads-dev libclxclient-dev libjack-jackd2-dev libreadline-dev libzita-alsa-pcmi-dev
```

The `pbuilder-dist` tool needs the user to choose a
dependency resolver. Create a file in your home directory
`~/.pbuilderrc` containing this line:
```
PBUILDERSATISFYDEPENDSCMD=/usr/lib/pbuilder/pbuilder-satisfydepends-apt
```

If the build host has root and /home directories on separate
partitions, disable hard linking for the Apt cache by adding
the following line to the `~/.pbuilderrc` file. Otherwise,
you may see the error `Invalid cross-device link` in the
creation of the chroot.
```
export APTCACHEHARDLINK=no
```

Install pbuilder-dist and create a clean buster chroot for
the ARM hard float architecture:
```
sudo apt install ubuntu-dev-tools qemu-user-static
pbuilder-dist buster armhf create
```

Before the next build, update the buster base tarball to
avoid 404 errors during the package download step:
```
pbuilder-dist buster armhf update
```

Change into the directory checked out from git and build
the source package (warning - everything in the repository
is included):
```
cd aeolus
dpkg-source -b .
```

Build an unsigned binary package in the buster chroot, for
example from the 0.9.8 source package:
```
pbuilder-dist buster armhf build --debbuildopts "--no-sign" ../aeolus_0.9.8.dsc
ls ~/pbuilder/buster-armhf_result/ -lah
```

The new aeolus .deb file should be listed in the
`buster-armhf_result` directory if the package build was
successful. If so, copy it into your project and commit
the new package.
