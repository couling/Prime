# Prime Number Scanner

## Overview 
This is a prime number scanner designed to rapidly find "small" primes very rapidly.  It was a project started out of boredom.  In its current form it can fill hard drives with 64 bit primes very rapidly.

## Building
Building requires `make` `gcc` `gzip` `ronn` (aka `ruby-ronn`).
Packaging for debian systems has been done using `dpkg-dev` and Philip Couling's `package-project` script.  The latter of these is not required as it is possible to simply lay out a dpkg using the [manifest](manifest)`.

## Running
See the [manual](prime.1.md).

## Author and Maintainer
This package was written and is maintained by Philip Couling <couling@gmail.com>



