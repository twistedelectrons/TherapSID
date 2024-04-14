# TherapSID

This is the Atmega1284 firmware for the [TherapSID synthesizer](https://www.twistedelectrons.com/therapsid).

Find the official releases at [the official product page](https://www.twistedelectrons.com/therapsid) or the [github release page](https://github.com/twistedelectrons/TherapSID/releases).

![Photo of the TherapSID](https://static.wixstatic.com/media/b8c32b_83978e994e24423d991c01e184dd30ce~mv2.jpg)

## ASID support - remix sid tunes however you like

This fork contains ASID enhancements which is based on the original firmware of Alex Smith and the ASID support of Thomas Jansson:

![ASID Quick Guide - Page 1](https://github.com/rio-rattenrudel/TherapSID/blob/asid-enhancements/doc/asid_quick_guide_01.png)


![ASID Quick Guide - Page 2](https://github.com/rio-rattenrudel/TherapSID/blob/asid-enhancements/doc/asid_quick_guide_02.png)

Some of the topics that were focussed on:

 * TherapSID gives direct visual feedback on the chip selection

 * TherapSID's SOLO mode automatically selects the chip by RETRIG (A1) or LOOP (A2) or the combination of both (A3)
   
 * A shift (SH) key has been introduced (LFO NOISE button) that allows you to combine waveforms or restore individual areas

 * Shift can RESET each POT value while SH is held and POT is turned

 * The dot indicator (left, right) gives real feedback on the remix status of the chip

## Building and flashing the source code:

Install `platformio` and `python2`. (Yes. I am so sorry :(.)

To compile the firmware and to generate the firmware update sysex, run:

```
make firmware.syx
```

Then put the device into bootloader mode by holding down the "filter" button while turning it on,
and send the sysex. Note that you need to wait about 200msec between each sysex.

## Unit tests

Parts of the code can be tested on your development machine. We use the
[Catch2](https://github.com/catchorg/Catch2) unit testing framework. To run the tests that live
in the `test/` folder, just type:

```
make test
```

Note that the tests also serve as a kind of documentation, as the section names usually give a
good hint of what the class is supposed to do.

## Contributing

If you want to fix a bug, add a feature or contribute in any other way to the development
of this firmware, we are glad to accept a pull request in this repository! Please make sure
your contribution adheres to the following:

- Please update [CHANGELOG.md](CHANGELOG.md) to contain a brief description of your changes.
- Ensure you do not break loading of old patches.
- Format your code using `make fmt` before submitting.
- Run the unit tests using `make test` before submitting.

Thank you, we are looking forward to your contribution! :)

## License

The TherapSID firmware under `src/` and `include/` is free software. It is released under the terms of
the [ISC License](LICENSE.md). The libraries and tools distributed in the `3rdparty/` and `lib/`
directories have their own copyright and license notes.
