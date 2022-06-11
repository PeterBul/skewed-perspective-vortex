# OPrintStream

The extendable, not a stream but behaves like a stream, [Arduino printing][0] convenience library.

## Abstract

Concept for this library is simple. If you want to write your Arduino print statements using less code and in one line using modern C++ practices, you install this library. :)

This library has a hard dependency on Arduino as it is written to specifically to use Arduino Serial printing methods. It is confirmed working for [Arduino MKR 1010 WiFi][1], but probably needs some very simple tweaking to work on other boards. Help is appreciated in that department.

## Dependencies

- [Arduino][0] Framework

## Building

* I wrote the project using [VS Code][4] with [the following extensions][5].
* The repository includes an automatically generated intellisense configuration.
* The project builds using the [Arduino Framework][7], so either the [Arduino IDE][8] or the [Arduino CLI][7] are required. See the [Arduino VS Code extension][9] documentation on how to configure the framework and perform a basic build.

## Using

There is a basic example [here][10].
It wraps all of the Arduino Serial Print functions with `<<` overloads so you can Print like you would using the OStringStream STL construct in modern C++.

```cpp
OPrintStream<> print(USBDevice);
print << bin << "Testing... " << 1 << " " << 2 << endl;
```

You can declare an `extern` instance of the class that you can reference in each of your compilation units, just as you would normally refer to Arduino's `Serial`.

## Extending

Extending the class to be able to print your own classes or structs is also pretty simple. You can either:
1. Have your object extend the Arduino `Printable` class and implement the `virtual size_t printTo(Print& p) const = 0;` method. In this case you would not have to extend OPrintStream.
2. If you want to print an object that you do not own, say [boost::string_view][3], you can extend OPrintStream using the Curiously Recurring Template Pattern to specify your class as the overload return type. Example:

```cpp
// "Copyright 2021 Francisco Aguilera"

#ifndef INCLUDE_LOGGER_HPP_
#define INCLUDE_LOGGER_HPP_

#include <USB/USBAPI.h>
#include <oprintstream.hpp>
#include <boost_1_76_0.h>
#include <boost/utility/string_view.hpp>

namespace falven {
namespace ad {

class Logger : public OPrintStream<Logger> {
 public:
  Logger(USBDeviceClass &usb) : OPrintStream(usb) {}
  ~Logger() override = default;
  ...

  /**
   * @brief Prints the string referenced by the provided boost::string_view.
   *
   * @param arg The boost::stringview to print.
   * @return OPrintStream& Returns the OPrintStream reference for
   * continuation.
   */
  OPrintStream &operator<<(const boost::string_view &arg) {
    print(arg.data());
    return *this;
  }

  // Bring our base class operator overloads into scope.
  using OPrintStream<Logger>::operator<<;
};

}  // namespace ad
}  // namespace falven

#endif  // INCLUDE_LOGGER_HPP_
```

[0]: https://www.arduino.cc/
[1]: https://store.arduino.cc/usa/mkr-wifi-1010
[2]: https://github.com/Falven/ad-boost
[3]: https://www.boost.org/
[4]: https://code.visualstudio.com/
[5]: https://github.com/Falven/ad-oprintstream/blob/main/.vscode/extensions.json
[6]: https://www.arduino.cc/en/software
[7]: https://arduino.github.io/arduino-cli/latest/getting-started/
[8]: https://www.arduino.cc/en/software
[9]: https://marketplace.visualstudio.com/items?itemName=vsciot-vscode.vscode-arduino
[10]: https://github.com/Falven/ad-oprintstream/blob/main/examples/basic_example/basic-example.ino