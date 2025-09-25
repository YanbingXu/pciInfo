#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <iostream>

int widthTable[16] = {0, 1, 2, 4, 8, 16};
int speedTable[16] = {0, 2500, 5000, 8000, 16000};

int getIntFromCFNumber(CFDictionaryRef props, const char* key) {
    CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(props, CFStringCreateWithCString(nullptr, key, kCFStringEncodingUTF8));
    if (!num) return 0;
    int value = 0;
    CFNumberGetValue(num, kCFNumberIntType, &value);
    return value;
}

int getIntFromCFData(CFDictionaryRef props, const char* key) {
    CFDataRef data = (CFDataRef)CFDictionaryGetValue(props, CFStringCreateWithCString(nullptr, key, kCFStringEncodingUTF8));
    if (!data) return 0;
    const uint8_t* bytes = CFDataGetBytePtr(data);
    if (!bytes) return 0;
    return bytes[0]; // macOS PCIe speed typically stored in first byte
}

void printPCIInfo() {
    CFMutableDictionaryRef matchDict = IOServiceMatching("IOPCIDevice");
    io_iterator_t iter;
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, matchDict, &iter) != KERN_SUCCESS) {
        std::cerr << "Failed to get PCI devices\n";
        return;
    }

    io_object_t device;
    while ((device = IOIteratorNext(iter))) {
        CFMutableDictionaryRef props = nullptr;
        if (IORegistryEntryCreateCFProperties(device, &props, kCFAllocatorDefault, kNilOptions) != KERN_SUCCESS) {
            IOObjectRelease(device);
            continue;
        }

        int vendor = getIntFromCFData(props, "vendor-id");
        int deviceID = getIntFromCFData(props, "device-id");
        int subVendor = getIntFromCFData(props, "subsystem-vendor-id");
        int subDevice = getIntFromCFData(props, "subsystem-id");
        int revision = getIntFromCFData(props, "revision-id");

        int linkStatus = getIntFromCFNumber(props, "IOPCIExpressLinkStatus");
        int curSpeedGen = linkStatus & 0xf;          // 低4位
        int curWidthEnc = (linkStatus >> 4) & 0xf;   // 接下来的4位
        int curWidth = widthTable[curWidthEnc];
        int curSpeed = speedTable[curSpeedGen];

        int maxSpeedGen = getIntFromCFData(props, "maximum-link-speed");
        int maxSpeed = speedTable[maxSpeedGen];

        std::cout << "PCI Device: vendor=0x" << std::hex << vendor
                  << " device=0x" << deviceID
                  << " subsystem-vendor=0x" << subVendor
                  << " subsystem=0x" << subDevice
                  << " revision=0x" << revision << std::dec << "\n";
        std::cout << "  pciCurWidth=" << curWidth << "\n";
        std::cout << "  pciCurGen=" << curSpeed << " (GT/s)\n";
        std::cout << "  pciMaxGen=" << maxSpeed << " (GT/s)\n\n";

        CFRelease(props);
        IOObjectRelease(device);
    }
    IOObjectRelease(iter);
}


// int getIntFromCFData(CFDictionaryRef props, const char* key) {
//     CFStringRef cfKey = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
//     CFTypeRef ref = CFDictionaryGetValue(props, cfKey);
//     CFRelease(cfKey);
//     if (ref && CFGetTypeID(ref) == CFDataGetTypeID()) {
//         const UInt32* val = reinterpret_cast<const UInt32*>(CFDataGetBytePtr((CFDataRef)ref));
//         return *val;
//     }
//     return 0;
// }

// int getIntFromCFNumber(CFDictionaryRef props, const char* key) {
//     CFStringRef cfKey = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
//     CFTypeRef ref = CFDictionaryGetValue(props, cfKey);
//     CFRelease(cfKey);
//     if (ref && CFGetTypeID(ref) == CFNumberGetTypeID()) {
//         int val = 0;
//         CFNumberGetValue((CFNumberRef)ref, kCFNumberIntType, &val);
//         return val;
//     }
//     return 0;
// }

// void printPCIInfo() {
//     CFMutableDictionaryRef matchDict = IOServiceMatching("IOPCIDevice");
//     if (!matchDict) return;
//
//     io_iterator_t iter;
//     if (IOServiceGetMatchingServices(kIOMasterPortDefault, matchDict, &iter) != KERN_SUCCESS) return;
//
//     io_object_t device;
//     while ((device = IOIteratorNext(iter))) {
//         CFMutableDictionaryRef props = nullptr;
//         if (IORegistryEntryCreateCFProperties(device, &props, kCFAllocatorDefault, kNilOptions) == KERN_SUCCESS) {
//             if (props) {
//                 int vid = getIntFromCFData(props, "vendor-id");
//                 int did = getIntFromCFData(props, "device-id");
//                 int subid = getIntFromCFData(props, "subsystem-vendor-id"); // macOS 里通常是 subsystem-vendor-id
//                 int subdev = getIntFromCFData(props, "subsystem-id");
//                 int rev = getIntFromCFData(props, "revision-id");
//
//                 int maxSpeed = getIntFromCFData(props, "maximum-link-speed"); // 最大链路速率
//                 int linkStatus = getIntFromCFNumber(props, "IOPCIExpressLinkStatus"); // 当前链路状态
//
//                 // linkStatus 高 16bit = 宽度，低16bit = 速率（GT/s） macOS 参考 IORegistryExplorer
//                 int curWidth = (linkStatus >> 16) & 0xffff;
//                 int curSpeed = linkStatus & 0xffff;
//
//                 std::cout << "PCI Device: vendor=0x" << std::hex << vid
//                           << " device=0x" << did
//                           << " subsystem-vendor=0x" << subid
//                           << " subsystem=0x" << subdev
//                           << " revision=0x" << rev << std::dec << "\n";
//
//                 std::cout << "  pciCurWidth=" << curWidth << "\n";
//                 std::cout << "  pciCurGen=" << curSpeed << " (GT/s)\n";
//                 std::cout << "  pciMaxGen=" << maxSpeed << " (GT/s)\n";
//
//                 CFRelease(props);
//             }
//         }
//         IOObjectRelease(device);
//     }
//     IOObjectRelease(iter);
// }

void printCFTypeInfo(CFTypeRef value) {
    if (!value) return;

    CFTypeID type = CFGetTypeID(value);
    if (type == CFNumberGetTypeID()) {
        int val = 0;
        CFNumberGetValue((CFNumberRef)value, kCFNumberIntType, &val);
        std::cout << "CFNumber: " << val << "\n";
    } else if (type == CFDataGetTypeID()) {
        CFDataRef data = (CFDataRef)value;
        const UInt8* bytes = CFDataGetBytePtr(data);
        CFIndex len = CFDataGetLength(data);
        std::cout << "CFData (len=" << len << "): ";
        for (CFIndex i = 0; i < std::min(len, CFIndex(8)); ++i)
            std::cout << std::hex << (int)bytes[i] << " ";
        std::cout << std::dec << "\n";
    } else if (type == CFStringGetTypeID()) {
        char buf[128];
        if (CFStringGetCString((CFStringRef)value, buf, sizeof(buf), kCFStringEncodingUTF8))
            std::cout << "CFString: " << buf << "\n";
    } else if (type == CFBooleanGetTypeID()) {
        bool b = CFBooleanGetValue((CFBooleanRef)value);
        std::cout << "CFBoolean: " << b << "\n";
    } else {
        std::cout << "Other CFTypeID: " << type << "\n";
    }
}

void debugPCIInfo() {
    CFMutableDictionaryRef matchDict = IOServiceMatching("IOPCIDevice");
    if (!matchDict) {
        std::cerr << "Failed to create match dictionary\n";
        return;
    }

    io_iterator_t iter;
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, matchDict, &iter) != KERN_SUCCESS) {
        std::cerr << "Failed to get matching services\n";
        return;
    }

    io_object_t device;
    while ((device = IOIteratorNext(iter))) {
        CFMutableDictionaryRef props = nullptr;
        if (IORegistryEntryCreateCFProperties(device, &props, kCFAllocatorDefault, kNilOptions) == KERN_SUCCESS) {
            if (props) {
                CFIndex count = CFDictionaryGetCount(props);
                const void** keys = new const void*[count];
                const void** values = new const void*[count];
                CFDictionaryGetKeysAndValues(props, keys, values);

                std::cout << "---- PCI Device ----\n";
                for (CFIndex i = 0; i < count; ++i) {
                    char keyBuf[128];
                    CFStringRef keyStr = (CFStringRef)keys[i];
                    if (CFStringGetCString(keyStr, keyBuf, sizeof(keyBuf), kCFStringEncodingUTF8)) {
                        std::cout << keyBuf << ": ";
                        printCFTypeInfo((CFTypeRef)values[i]);
                    }
                }

                delete[] keys;
                delete[] values;
                CFRelease(props);
            }
        }
        IOObjectRelease(device);
    }
    IOObjectRelease(iter);
}

int main() {
    // debugPCIInfo();
    printPCIInfo();
    return 0;
}
