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

todo: build inside chroot
