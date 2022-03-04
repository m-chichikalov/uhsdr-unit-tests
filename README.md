# Collection of UNIT and INTEGRATION tests for UHSDR.
UHSDR stands for **U**niversal**H**am**S**oftware**D**efined**R**adio - [REPO](https://github.com/df8oe/UHSDR).
So far the UHSDR code cloned as a git submodule and will be updated regularly.
- SoftWare in use:   
    - [Catch2](https://github.com/catchorg/Catch2) - run tests on the HOST.
    - ~[Unity](https://github.com/ThrowTheSwitch/Unity) - on the target.~
    - ~[Segger RTT](https://wiki.segger.com/RTT) - output test results from target.~
    - [CMake](https://cmake.org/overview/) - as a build processing system.

## Objective
To cover as much as possible and at the same time do not write fragile tests.

## Motivation
When you start contribute into a project with huge history and legacy it's always very
easy to brake it while trying to implement/ enhance something, especialy when there is no any unit and/or integration tests.
Indeed, it happens when I addded some features. 

The last straw on the camel's back were attempts to catch the bugs in huge, NONE maintainable, glued from chunks the state machine which generate CW code.

To build the confidence on my PRs I'm going start covering by tests the code that I'm touching.
I believe it's a good practice, isn't it?

## Build
```basch
mkdir .build && cd .build
cmake .. -G "proper_to_you_generator"
make
```
