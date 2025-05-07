#include "server.h"
#include "auth_manager.h"
#include "../device/driver_manager.h"
#include "../device/driver_bytecode.h"
#include "../device/driver_bridge.h"
#include "../tool_system/tool_registry.h"
#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Initialize the MCP server with configuration
 * 
 * @param config Server configuration
 * @return int 0 on success, negative error code on failure
 */
int MCP_ServerInit(const MCP_ServerConfig* config) {
    if (config == NULL) {
        return -1;
    }
    
    printf("Initializing MCP server...\n");
    
    // Initialize tool registry
    printf("Initializing tool registry...\n");
    if (MCP_ToolRegistryInit(config->maxTools) != 0) {
        printf("Failed to initialize tool registry\n");
        return -2;
    }
    
    // Initialize driver manager
    printf("Initializing driver manager...\n");
    if (MCP_DriverManagerInit(config->maxDrivers) != 0) {
        printf("Failed to initialize driver manager\n");
        return -3;
    }
    
    // Initialize bytecode driver system
    printf("Initializing bytecode driver system...\n");
    if (MCP_BytecodeDriverInit() != 0) {
        printf("Failed to initialize bytecode driver system\n");
        return -4;
    }
    
    // Initialize driver bridge system
    printf("Initializing driver bridge system...\n");
    if (MCP_DriverBridgeInit() != 0) {
        printf("Failed to initialize driver bridge system\n");
        return -5;
    }
    
    // Initialize driver bridge tools
    printf("Initializing driver bridge tools...\n");
    if (MCP_DriverBridgeToolsInit() != 0) {
        printf("Failed to initialize driver bridge tools\n");
        return -6;
    }
    
    // Initialize authentication manager - start with completely open access
    printf("Initializing authentication manager with open access...\n");
    if (MCP_AuthManagerInit(true) != 0) {
        printf("Failed to initialize authentication manager\n");
        return -7;
    }
    
    // Initialize transports if enabled in configuration
    
    // Initialize USB transport if enabled
    if (config->enableUSB) {
        printf("Initializing USB transport...\n");
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
        
        MCP_ServerTransport* usbTransport = MCP_USBTransportInit(&usbConfig);
        if (usbTransport != NULL) {
            if (MCP_USBTransportStart(usbTransport) == 0) {
                printf("USB transport started successfully\n");
                
                // Register transport with server (in a real implementation)
                // MCP_ServerRegisterTransport(usbTransport);
            } else {
                printf("Failed to start USB transport\n");
            }
        } else {
            printf("Failed to initialize USB transport\n");
        }
    }
    
    // Initialize Ethernet transport if enabled
    if (config->enableEthernet) {
        printf("Initializing Ethernet transport...\n");
        MCP_EthernetTransportConfig ethernetConfig = {
            .mode = MCP_ETHERNET_MODE_DHCP,
            .port = 5555,
            .maxConnections = 4,
            .connectionTimeout = 30000,
            .enableMDNS = true,
            .mdnsServiceName = "mcp-device"
        };
        
        MCP_ServerTransport* ethernetTransport = MCP_EthernetTransportInit(&ethernetConfig);
        if (ethernetTransport != NULL) {
            if (MCP_EthernetTransportStart(ethernetTransport) == 0) {
                printf("Ethernet transport started successfully\n");
                
                // Register transport with server (in a real implementation)
                // MCP_ServerRegisterTransport(ethernetTransport);
            } else {
                printf("Failed to start Ethernet transport\n");
            }
        } else {
            printf("Failed to initialize Ethernet transport\n");
        }
    }
    
    printf("MCP server initialized successfully\n");
    printf("System is in open authentication mode - ALL OPERATIONS ENABLED WITHOUT SECURITY\n");
    printf("Configure security later if needed using system.setAuth tool\n");
    return 0;
}

/**
 * @brief Validate authentication for MCP requests
 * 
 * This function validates authentication credentials for MCP requests by calling
 * the authentication manager. By default, the system starts with open authentication
 * where all operations are allowed without security checks. This behavior can be
 * configured through the authentication manager.
 * 
 * @param method Authentication method (0=none, 1=bearer, 2=api_key, 3=basic, 4=oauth, 5=custom)
 * @param token Authentication token or credentials
 * @return bool True if authentication is valid or not required, false otherwise
 */
bool MCP_ValidateAuth(int method, const char* token) {
    return MCP_AuthManagerValidate((MCP_AuthMethod)method, token);
}