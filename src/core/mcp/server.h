#ifndef MCP_SERVER_H
#define MCP_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief MCP server transport type
 */
typedef enum {
    MCP_TRANSPORT_UART,
    MCP_TRANSPORT_TCP,
    MCP_TRANSPORT_BLUETOOTH,
    MCP_TRANSPORT_USB,
    MCP_TRANSPORT_ETHERNET,
    MCP_TRANSPORT_CUSTOM
} MCP_ServerTransportType;

/**
 * @brief USB device class for MCP transport
 */
typedef enum {
    MCP_USB_CLASS_CDC = 0x02,    // Communications Device Class (CDC)
    MCP_USB_CLASS_HID = 0x03,    // Human Interface Device (HID)
    MCP_USB_CLASS_VENDOR = 0xFF  // Vendor-specific class
} MCP_USBClass;

/**
 * @brief USB transport configuration
 */
typedef struct {
    uint16_t vendorId;       // USB vendor ID
    uint16_t productId;      // USB product ID
    MCP_USBClass deviceClass; // USB device class
    uint32_t readTimeout;    // Read timeout in milliseconds
    uint32_t writeTimeout;   // Write timeout in milliseconds
    bool nonBlocking;        // Non-blocking mode
    uint16_t inEndpoint;     // Input endpoint
    uint16_t outEndpoint;    // Output endpoint
    char* serialNumber;      // Device serial number
    char* manufacturer;      // Manufacturer string
    char* productName;       // Product name string
} MCP_USBTransportConfig;

/**
 * @brief Ethernet interface configuration mode
 */
typedef enum {
    MCP_ETHERNET_MODE_DHCP,     // Use DHCP to configure network
    MCP_ETHERNET_MODE_STATIC,   // Use static IP configuration
    MCP_ETHERNET_MODE_AUTO      // Try DHCP, fall back to static
} MCP_EthernetMode;

/**
 * @brief Ethernet transport configuration
 */
typedef struct {
    MCP_EthernetMode mode;    // Network configuration mode
    uint8_t macAddress[6];    // MAC address (can be null for default)
    char* staticIp;           // Static IP address (used if mode is STATIC or AUTO)
    char* subnetMask;         // Subnet mask
    char* gateway;            // Default gateway
    char* dnsServer;          // DNS server
    uint16_t port;            // TCP port to listen on
    uint16_t maxConnections;  // Maximum number of connections
    uint32_t connectionTimeout; // Connection timeout in milliseconds
    bool enableMDNS;          // Enable mDNS/Bonjour discovery
    char* mdnsServiceName;    // mDNS service name
} MCP_EthernetTransportConfig;

/**
 * @brief MCP server transport interface
 */
typedef struct {
    MCP_ServerTransportType type;
    
    // Read function - returns number of bytes read or negative error code
    int (*read)(uint8_t* buffer, size_t maxLength, uint32_t timeout);
    
    // Write function - returns number of bytes written or negative error code
    int (*write)(const uint8_t* data, size_t length);
    
    // Close function - returns 0 on success or negative error code
    int (*close)(void);
    
    // Get status function - returns transport status (implementation-specific)
    uint32_t (*getStatus)(void);
    
    // Transport-specific configuration
    void* config;
    
    // User data (for transport implementations)
    void* userData;
} MCP_ServerTransport;

/**
 * @brief MCP server configuration
 */
typedef struct {
    char* deviceName;                  // Device name
    char* version;                     // Version string
    uint16_t maxSessions;              // Maximum number of concurrent sessions
    uint16_t maxOperationsPerSession;  // Maximum operations per session
    uint16_t maxContentSize;           // Maximum content size in bytes
    uint32_t sessionTimeout;           // Session timeout in milliseconds
    bool enableTools;                  // Enable tool exposure
    bool enableResources;              // Enable resource exposure
    bool enableEvents;                 // Enable event streaming
    bool enableAutomation;             // Enable automation rules
    char* deviceId;                    // Unique device identifier
    uint16_t maxTools;                 // Maximum number of registered tools
    uint16_t maxDrivers;               // Maximum number of registered drivers
    bool initialOpenAccess;            // Whether to start with open authentication
    bool enableUSB;                    // Enable USB transport
    bool enableEthernet;               // Enable Ethernet transport
    bool enableWifi;                   // Enable WiFi transport
    bool enableBluetooth;              // Enable Bluetooth transport
    bool enableSerial;                 // Enable Serial transport
} MCP_ServerConfig;

/**
 * @brief MCP session state
 */
typedef enum {
    MCP_SESSION_STATE_ACTIVE,
    MCP_SESSION_STATE_CLOSING,
    MCP_SESSION_STATE_CLOSED
} MCP_SessionState;

/**
 * @brief MCP operation type
 */
typedef enum {
    MCP_OPERATION_TYPE_CONTENT_REQUEST,
    MCP_OPERATION_TYPE_CONTENT_RESPONSE,
    MCP_OPERATION_TYPE_TOOL_INVOKE,
    MCP_OPERATION_TYPE_TOOL_RESULT,
    MCP_OPERATION_TYPE_EVENT_SUBSCRIBE,
    MCP_OPERATION_TYPE_EVENT_DATA,
    MCP_OPERATION_TYPE_EVENT_UNSUBSCRIBE,
    MCP_OPERATION_TYPE_RESOURCE_GET,
    MCP_OPERATION_TYPE_RESOURCE_SET,
    MCP_OPERATION_TYPE_RESOURCE_DATA,
    MCP_OPERATION_TYPE_ERROR
} MCP_OperationType;

/**
 * @brief Initialize the MCP server
 * 
 * @param config Server configuration
 * @return int 0 on success, negative error code on failure
 */
int MCP_ServerInit(const MCP_ServerConfig* config);

/**
 * @brief Connect a transport to the MCP server
 * 
 * @param transport Transport interface
 * @return int Session ID or negative error code
 */
int MCP_ServerConnect(MCP_ServerTransport* transport);

/**
 * @brief Process MCP server (check for messages, handle timeouts)
 * 
 * @param timeout Maximum time to block in milliseconds (0 for non-blocking)
 * @return int Number of operations processed or negative error code
 */
int MCP_ServerProcess(uint32_t timeout);

/**
 * @brief Register an operation with the MCP server
 * 
 * @param sessionId Session ID
 * @param type Operation type
 * @return const char* Operation ID or NULL on failure (caller must NOT free)
 */
const char* MCP_ServerRegisterOperation(const char* sessionId, MCP_OperationType type);

/**
 * @brief Complete an operation
 * 
 * @param sessionId Session ID
 * @param operationId Operation ID
 * @param success Operation success flag
 * @param data Operation result data
 * @param dataLength Length of result data
 * @return int 0 on success, negative error code on failure
 */
int MCP_ServerCompleteOperation(const char* sessionId, const char* operationId, 
                              bool success, const uint8_t* data, size_t dataLength);

/**
 * @brief Close a session
 * 
 * @param sessionId Session ID
 * @return int 0 on success, negative error code on failure
 */
int MCP_ServerCloseSession(const char* sessionId);

/**
 * @brief Get server status as JSON
 * 
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of buffer
 * @return int Number of bytes written or negative error code
 */
int MCP_ServerGetStatus(char* buffer, size_t bufferSize);

/**
 * @brief Send an event to all subscribed sessions
 * 
 * @param eventType Event type
 * @param eventData Event data
 * @param eventDataLength Length of event data
 * @return int Number of sessions event was sent to or negative error code
 */
int MCP_ServerSendEvent(const char* eventType, const uint8_t* eventData, size_t eventDataLength);

/**
 * @brief Validate authentication for MCP requests
 * 
 * @param method Authentication method (0=none, 1=bearer, 2=api_key, 3=basic, 4=oauth, 5=custom)
 * @param token Authentication token
 * @return bool True if authentication is valid or not required
 */
bool MCP_ValidateAuth(int method, const char* token);

/**
 * @brief Initialize USB transport
 * 
 * @param config USB transport configuration
 * @return MCP_ServerTransport* Initialized transport or NULL on failure
 */
MCP_ServerTransport* MCP_USBTransportInit(const MCP_USBTransportConfig* config);

/**
 * @brief Start USB transport
 * 
 * @param transport USB transport instance
 * @return int 0 on success, negative error code on failure
 */
int MCP_USBTransportStart(MCP_ServerTransport* transport);

/**
 * @brief Initialize Ethernet transport
 * 
 * @param config Ethernet transport configuration
 * @return MCP_ServerTransport* Initialized transport or NULL on failure
 */
MCP_ServerTransport* MCP_EthernetTransportInit(const MCP_EthernetTransportConfig* config);

/**
 * @brief Start Ethernet transport
 * 
 * @param transport Ethernet transport instance
 * @return int 0 on success, negative error code on failure
 */
int MCP_EthernetTransportStart(MCP_ServerTransport* transport);

#endif /* MCP_SERVER_H */