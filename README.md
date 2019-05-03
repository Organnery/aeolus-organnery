# build instructions



## development

The `libjack..` package does not currently work for Debian Sid.
`build-essential` & `pkgconf` are also required.

```
sudo apt-get install build-essential pkgconf debhelper libasound2-dev libclthreads-dev libclxclient-dev libjack-jackd2-dev libreadline-dev libzita-alsa-pcmi-dev
```


Stops are required to be installed inside `source/stops`.
build like this (the LIBDIR uses locally build libraries):

```
$ make clean
$ LIBDIR=. make

```




## production

Install pbuilder-dist and create a clean buster chroot for the ARM hard float architecture:
```
$ sudo apt-get install ubuntu-dev-tools
$ pbuilder-dist buster armhf create
```


Before the next build, update the buster base tarball to avoid 404 errors during the package download step:
```
$ pbuilder-dist buster armhf update
```


Build source package (warning - everything in repository is included):
```
$ cd aeolus
$ dpkg-source -b .
```


Build binary package in chroot:
```
$ pbuilder-dist buster armhf build --debbuildopts "--no-sign" ../aeolus_0.9.7.dsc
$ ls ~/pbuilder/buster-armhf_result/ -lah
```
