#include <iostream>
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cstring>

// 你的结构
typedef struct {
    char     sbdf[32];     //!< Segment:Bus:Device.Function 字符串
    uint32_t segment;
    uint32_t bus;
    uint32_t device;
    uint32_t pciDeviceId;
    uint32_t pciSubsystemId;
    float    pciMaxSpeed;
    float    pciCurSpeed;
    uint32_t pciMaxWidth;
    uint32_t pciCurWidth;
    uint32_t pciMaxGen;
    uint32_t pciCurGen;
} ZxmlPciInfo;

// 安全获取 UInt32 属性
uint32_t getUInt32Property(io_registry_entry_t service, CFStringRef key) {
    CFTypeRef prop = IORegistryEntryCreateCFProperty(service, key, kCFAllocatorDefault, 0);
    if (!prop) return 0;

    uint32_t value = 0;
    if (CFGetTypeID(prop) == CFDataGetTypeID()) {
        CFDataRef data = (CFDataRef)prop;
        if (CFDataGetLength(data) == sizeof(uint32_t)) {
            CFDataGetBytes(data, CFRangeMake(0, sizeof(uint32_t)), (UInt8*)&value);
        }
    } else if (CFGetTypeID(prop) == CFNumberGetTypeID()) {
        CFNumberGetValue((CFNumberRef)prop, kCFNumberSInt32Type, &value);
    }
    CFRelease(prop);
    return value;
}

// 打印 PCI 信息并返回结构
ZxmlPciInfo printPCIInfo() {
    ZxmlPciInfo info{};
    memset(&info, 0, sizeof(info));

    CFMutableDictionaryRef matchingDict = IOServiceMatching("IOPCIDevice");
    io_iterator_t iter;
    if (IOServiceGetMatchingServices(kIOMainPortDefault, matchingDict, &iter) != KERN_SUCCESS) {
        std::cerr << "No PCI devices found." << std::endl;
        return info;
    }

    io_registry_entry_t service;
    while ((service = IOIteratorNext(iter))) {
        uint32_t vendorId = getUInt32Property(service, CFSTR("vendor-id"));
        uint32_t deviceId = getUInt32Property(service, CFSTR("device-id"));
        uint32_t subsystemId = getUInt32Property(service, CFSTR("subsystem-id"));
        uint32_t revisionId = getUInt32Property(service, CFSTR("revision-id"));

        // 填充结构
        info.segment = 0;   // macOS 一般只有 segment 0
        info.bus = getUInt32Property(service, CFSTR("bus-number"));
        info.device = getUInt32Property(service, CFSTR("device-number"));
        info.pciDeviceId = deviceId;
        info.pciSubsystemId = subsystemId;

        snprintf(info.sbdf, sizeof(info.sbdf), "%04x:%02x:%02x.0",
                 info.segment, info.bus, info.device);

        std::cout << "Device: " << info.sbdf
                  << " Vendor=0x" << std::hex << vendorId
                  << " Device=0x" << deviceId
                  << " Subsystem=0x" << subsystemId
                  << " Revision=0x" << revisionId
                  << std::dec << std::endl;

        IOObjectRelease(service);
    }
    IOObjectRelease(iter);

    return info;
}

int main() {
    auto pciInfo = printPCIInfo();
    std::cout << "First PCI device ID: 0x" << std::hex << pciInfo.pciDeviceId << std::endl;
    return 0;
}
