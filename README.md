LEDtris
======

**ledtris** is a basic Tetris game for an LED panel connected to a Raspberry Pi.
It was written more as a hack than a serious project, but it's actually
playable, and includes a basic demo mode/autoplay.

## Features

* Basic gameplay with a connected joystick
* Autoplay/attract mode
* Animations (inspired by the NES version)
* Themed, color-changing levels

## Hardware

* Raspberry Pi 2
* [Adafruit RGB Matrix Hat](https://www.amazon.com/gp/product/B00SK69C6E)
* [64x32 LED panel](https://www.amazon.com/gp/product/B07SDMWX9R/)
* [2.4GHz Retro Controller](https://www.amazon.com/gp/product/B07MBF7FN1/)

Game uses [hzeller](https://github.com/hzeller)'s excellent
[rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix) library
with SDL for input.

## Compiling

* Install and configure
[rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix)
* `cd` to directory `rpi-rgb-led-matrix` (location where you cloned the library)
* `git clone https://github.com/neokobe/ledtris`
* `cd ledtris`
* `make`

`Makefile` is configured to look for `rpi-rgb-led-matrix` files in the parent
directory, but you can build it anywhere, as long as you tweak the first 2
lines in the makefile.

## AI

ledtris includes a very basic autoplay mode with a limited AI:

* Uses "snugness" to determine optimal placement, favoring locations with more
neighboring squares, rather than fewer
* Considers only straight drops (will not shimmy left or right, nor rotate)
* Only scores the current piece, making no assumptions about any of the
following (or preceding) pieces

License
-------
```
Copyright (c) 2020 NeoKobe

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
