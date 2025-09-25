#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <iostream>

int getIntFromCFData(CFDictionaryRef props, const char* key) {
    CFStringRef cfKey = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
    CFTypeRef ref = CFDictionaryGetValue(props, cfKey);
    CFRelease(cfKey);
    if (ref && CFGetTypeID(ref) == CFDataGetTypeID()) {
        const UInt32* val = reinterpret_cast<const UInt32*>(CFDataGetBytePtr((CFDataRef)ref));
        return *val;
    }
    return 0;
}

int getIntFromCFNumber(CFDictionaryRef props, const char* key) {
    CFStringRef cfKey = CFStringCreateWithCString(kCFAllocatorDefault, key, kCFStringEncodingUTF8);
    CFTypeRef ref = CFDictionaryGetValue(props, cfKey);
    CFRelease(cfKey);
    if (ref && CFGetTypeID(ref) == CFNumberGetTypeID()) {
        int val = 0;
        CFNumberGetValue((CFNumberRef)ref, kCFNumberIntType, &val);
        return val;
    }
    return 0;
}

void printPCIInfo() {
    CFMutableDictionaryRef matchDict = IOServiceMatching("IOPCIDevice");
    if (!matchDict) return;

    io_iterator_t iter;
    if (IOServiceGetMatchingServices(kIOMasterPortDefault, matchDict, &iter) != KERN_SUCCESS) return;

    io_object_t device;
    while ((device = IOIteratorNext(iter))) {
        CFMutableDictionaryRef props = nullptr;
        if (IORegistryEntryCreateCFProperties(device, &props, kCFAllocatorDefault, kNilOptions) == KERN_SUCCESS) {
            if (props) {
                int vid = getIntFromCFData(props, "vendor-id");
                int did = getIntFromCFData(props, "device-id");
                int subid = getIntFromCFData(props, "subsystem-vendor-id"); // macOS 里通常是 subsystem-vendor-id
                int subdev = getIntFromCFData(props, "subsystem-id");
                int rev = getIntFromCFData(props, "revision-id");

                int maxSpeed = getIntFromCFData(props, "maximum-link-speed"); // 最大链路速率
                int linkStatus = getIntFromCFNumber(props, "IOPCIExpressLinkStatus"); // 当前链路状态

                // linkStatus 高 16bit = 宽度，低16bit = 速率（GT/s） macOS 参考 IORegistryExplorer
                int curWidth = (linkStatus >> 16) & 0xffff;
                int curSpeed = linkStatus & 0xffff;

                std::cout << "PCI Device: vendor=0x" << std::hex << vid
                          << " device=0x" << did
                          << " subsystem-vendor=0x" << subid
                          << " subsystem=0x" << subdev
                          << " revision=0x" << rev << std::dec << "\n";

                std::cout << "  pciCurWidth=" << curWidth << "\n";
                std::cout << "  pciCurGen=" << curSpeed << " (GT/s)\n";
                std::cout << "  pciMaxGen=" << maxSpeed << " (GT/s)\n";

                CFRelease(props);
            }
        }
        IOObjectRelease(device);
    }
    IOObjectRelease(iter);
}

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
