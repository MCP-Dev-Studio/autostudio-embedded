# MCP Server Transport Implementation

This document describes the implementation of USB and Ethernet transport for the MCP server on embedded systems.

## Overview

The MCP server now supports five transport methods:
1. UART (Serial)
2. TCP (Network)
3. Bluetooth
4. USB (New)
5. Ethernet (New)

These transport options allow the MCP server to communicate with clients (LLMs) using a variety of physical interfaces, making it highly versatile for embedded systems.

## Transport Architecture

Each transport follows the same architecture:
1. **Transport Type** - Defined in the `MCP_ServerTransportType` enum
2. **Transport Configuration** - Specific configuration structures (e.g., `MCP_USBTransportConfig`)
3. **Transport Interface** - A standardized interface defined by `MCP_ServerTransport`
4. **Transport Initialization** - Functions to initialize and start the transport (e.g., `MCP_USBTransportInit`)
5. **Transport Functions** - Low-level functions for reading/writing data and managing the transport

## USB Transport

The USB transport allows the MCP server to communicate over USB, supporting:

- **USB Classes**:
  - Communications Device Class (CDC)
  - Human Interface Device (HID)
  - Vendor-specific classes

- **Configuration Options**:
  - Vendor ID and Product ID
  - USB device class
  - Endpoints and descriptors
  - Timeouts and other parameters

- **Example Configuration**:
  ```c
  MCP_USBTransportConfig usbConfig = {
      .vendorId = 0x1234,
      .productId = 0x5678,
      .deviceClass = MCP_USB_CLASS_CDC,
      .readTimeout = 5000,
      .writeTimeout = 5000,
      .nonBlocking = false,
      .inEndpoint = 0x81,
      .outEndpoint = 0x01,
      .serialNumber = "MCP123456",
      .manufacturer = "MCP Embedded",
      .productName = "MCP USB Device"
  };
  ```

- **Initialization**:
  ```c
  MCP_ServerTransport* usbTransport = MCP_USBTransportInit(&usbConfig);
  if (usbTransport != NULL) {
      if (MCP_USBTransportStart(usbTransport) == 0) {
          // USB transport started successfully
      }
  }
  ```

## Ethernet Transport

The Ethernet transport allows the MCP server to communicate over wired networks, supporting:

- **Network Configuration**:
  - DHCP or static IP configuration
  - TCP port and connection settings
  - Optional mDNS/Bonjour discovery

- **Configuration Options**:
  - Network configuration mode (DHCP, static, auto)
  - MAC address
  - IP settings (for static mode)
  - TCP port and connection limits
  - mDNS settings

- **Example Configuration**:
  ```c
  MCP_EthernetTransportConfig ethernetConfig = {
      .mode = MCP_ETHERNET_MODE_DHCP,
      .port = 5555,
      .maxConnections = 4,
      .connectionTimeout = 30000,
      .enableMDNS = true,
      .mdnsServiceName = "mcp-device"
  };
  ```

- **Initialization**:
  ```c
  MCP_ServerTransport* ethernetTransport = MCP_EthernetTransportInit(&ethernetConfig);
  if (ethernetTransport != NULL) {
      if (MCP_EthernetTransportStart(ethernetTransport) == 0) {
          // Ethernet transport started successfully
      }
  }
  ```

## Implementation Details

### Server Configuration

Transport options are enabled through the server configuration structure:

```c
MCP_ServerConfig config = {
    // Other configuration options...
    .initialOpenAccess = true,  // Start with open authentication
    .enableUSB = true,          // Enable USB transport
    .enableEthernet = true,     // Enable Ethernet transport
    .enableWifi = false,
    .enableBluetooth = true,
    .enableSerial = true
};
```

### Transport Implementation

Each transport implements four key functions:
1. **read** - Reads data from the transport with optional timeout
2. **write** - Writes data to the transport
3. **close** - Closes the transport connection
4. **getStatus** - Gets the current status of the transport

For example, the USB implementation:

```c
static int usbRead(uint8_t* buffer, size_t maxLength, uint32_t timeout) {
    // Implementation details...
}

static int usbWrite(const uint8_t* data, size_t length) {
    // Implementation details...
}

static int usbClose(void) {
    // Implementation details...
}

static uint32_t usbGetStatus(void) {
    // Implementation details...
}
```

### Server Initialization

During server initialization, enabled transports are initialized and started:

```c
// Initialize USB transport if enabled
if (config->enableUSB) {
    MCP_USBTransportConfig usbConfig = { /* ... */ };
    
    MCP_ServerTransport* usbTransport = MCP_USBTransportInit(&usbConfig);
    if (usbTransport != NULL) {
        if (MCP_USBTransportStart(usbTransport) == 0) {
            // Transport started successfully
        }
    }
}

// Initialize Ethernet transport if enabled
if (config->enableEthernet) {
    MCP_EthernetTransportConfig ethernetConfig = { /* ... */ };
    
    MCP_ServerTransport* ethernetTransport = MCP_EthernetTransportInit(&ethernetConfig);
    if (ethernetTransport != NULL) {
        if (MCP_EthernetTransportStart(ethernetTransport) == 0) {
            // Transport started successfully
        }
    }
}
```

## Integration with Authentication System

The authentication system is configured to start with open access, allowing for initial configuration through any transport method. Security can be applied later once the device is set up.

```c
// Initialize authentication manager - start with completely open access
printf("Initializing authentication manager with open access...\n");
if (MCP_AuthManagerInit(true) != 0) {
    printf("Failed to initialize authentication manager\n");
    return -7;
}
```

## Future Enhancements

Potential enhancements for the transport system:
1. **Transport Dynamic Registration** - Allow transports to be registered at runtime
2. **Transport Security** - Add transport-specific security options (TLS for Ethernet, etc.)
3. **Transport Failover** - Automatically switch to alternative transports if primary fails
4. **Transport-Specific Tools** - Add tools for configuring transports at runtime

## Conclusion

The USB and Ethernet transport implementations significantly enhance the connectivity options for the MCP server on embedded systems. These transports, combined with the open authentication system, make it easy to set up and configure MCP-enabled devices through a variety of interfaces.