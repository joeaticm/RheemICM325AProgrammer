# RheemICM325AProgrammer

This is a quick project made for Rheem. It runs on their production line so that that they can configure ICM325A's to one of a couple configurations.

The NUCLEO folder contains the code that runs on the ST Nucleo-L053R8 dev board with an X-NUCLEO-NFC06A1 hat.
It builds using STM32CubeIDE 1.11.2.
The NUCLEO code is based off a copy of the NFC-Reader-Writer-Attila project with added support for providing configuration files over UART.

The UI is a C# WPF app built using Visual Studio Code 2022.
