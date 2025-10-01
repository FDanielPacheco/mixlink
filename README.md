# mixlink: Linux-based L2 stack for protocol-aware serial-based wireless communications

`mixlink` mixlink is a user-space binary, implemented in C, that enables the encapsulation and transport of TCP/IP traffic over serial-based wireless devices (e.g., LoRa transceivers). It allows a pair of Linux systems to interconnect using the standard networking stack, while the underlying information is transparently forwarded through non-conventional serial links.

Multiple instances of mixlink can be executed concurrently, each configured independently through an XML descriptor file. This configuration specifies runtime parameters such as the network interface card (NIC) to be bridged and the serial device (e.g., /dev/ttyS0).

The communication stack managed by mixlink integrates the following components:
- Multi-level Framing and Byte-Stuffing: Encapsulation of data streams to ensure frame boundary recognition over serial links.
- Header Optimization: Reduction of TCP/IP header overhead to improve efficiency over constrained links.
- Link-Layer QoS: Basic quality-of-service (QoS) mechanisms applied at the physical serial link.
- Device Drivers: Per-serial-port driver modules can be specified to support hardware that requires additional signaling or configuration (e.g., SPI control lines, transmission power adjustment). These driver modules have full authority over the serial link, allowing complete customization of link initialization, control, and data transfer procedures.

The system operates in a pipeline architecture, where each protocol module processes data sequentially. Modules are implemented as runtime-loadable dynamic libraries, enabling custom link-layer stacks per device and extensibility through user-contributed modules.
Each module must expose a small set of mandatory interface functions, which define how mixlink interacts with the module. These functions provide hooks for initialization, data processing, and control, ensuring interoperability between custom and core modules within the pipeline.

---
## Installation

### Pre-Requirements

---
## Usage Example 

---
## Contributing
If you find issues or have suggestions, please open an issue or submit a pull request.

---
## Author

Fábio D. Pacheco \
Email: fabio.d.pacheco@inesctec.pt

## License

[LGPL-2.1 license](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html)