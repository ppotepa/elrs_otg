/*
 * libusb_stub.cpp - Stub implementation for libusb-1.0
 * This allows compilation but will not work with real hardware
 * Install the real libusb-1.0 library for actual functionality
 */

#include "libusb-1.0/libusb.h"
#include <iostream>
#include <cstring>

// Stub implementations that show error messages
int libusb_init(libusb_context **context)
{
    std::cerr << "[LIBUSB_STUB] Real libusb-1.0 library not available" << std::endl;
    return LIBUSB_ERROR_NOT_SUPPORTED;
}

void libusb_exit(libusb_context *context)
{
    // No-op
}

ssize_t libusb_get_device_list(libusb_context *context, libusb_device ***list)
{
    std::cerr << "[LIBUSB_STUB] Real libusb-1.0 library required for device enumeration" << std::endl;
    *list = nullptr;
    return 0;
}

void libusb_free_device_list(libusb_device **list, int unref_devices)
{
    // No-op
}

int libusb_get_device_descriptor(libusb_device *device, struct libusb_device_descriptor *desc)
{
    return LIBUSB_ERROR_NOT_SUPPORTED;
}

libusb_device_handle *libusb_open_device_with_vid_pid(libusb_context *context, uint16_t vendor_id, uint16_t product_id)
{
    std::cerr << "[LIBUSB_STUB] Real libusb-1.0 library required for device access" << std::endl;
    return nullptr;
}

int libusb_open(libusb_device *device, libusb_device_handle **handle)
{
    return LIBUSB_ERROR_NOT_SUPPORTED;
}

void libusb_close(libusb_device_handle *handle)
{
    // No-op
}

libusb_device *libusb_get_device(libusb_device_handle *handle)
{
    return nullptr;
}

int libusb_claim_interface(libusb_device_handle *handle, int interface_number)
{
    return LIBUSB_ERROR_NOT_SUPPORTED;
}

int libusb_release_interface(libusb_device_handle *handle, int interface_number)
{
    return LIBUSB_ERROR_NOT_SUPPORTED;
}

int libusb_kernel_driver_active(libusb_device_handle *handle, int interface_number)
{
    return 0; // Assume no kernel driver
}

int libusb_detach_kernel_driver(libusb_device_handle *handle, int interface_number)
{
    return LIBUSB_ERROR_NOT_SUPPORTED;
}

int libusb_bulk_transfer(libusb_device_handle *handle, unsigned char endpoint,
                         unsigned char *data, int length, int *actual_length, unsigned int timeout)
{
    std::cerr << "[LIBUSB_STUB] Real libusb-1.0 library required for data transfer" << std::endl;
    return LIBUSB_ERROR_NOT_SUPPORTED;
}

int libusb_get_string_descriptor_ascii(libusb_device_handle *handle, uint8_t desc_index,
                                       unsigned char *data, int length)
{
    return LIBUSB_ERROR_NOT_SUPPORTED;
}

const char *libusb_error_name(int error_code)
{
    switch (error_code)
    {
    case LIBUSB_SUCCESS:
        return "LIBUSB_SUCCESS";
    case LIBUSB_ERROR_IO:
        return "LIBUSB_ERROR_IO";
    case LIBUSB_ERROR_INVALID_PARAM:
        return "LIBUSB_ERROR_INVALID_PARAM";
    case LIBUSB_ERROR_ACCESS:
        return "LIBUSB_ERROR_ACCESS";
    case LIBUSB_ERROR_NO_DEVICE:
        return "LIBUSB_ERROR_NO_DEVICE";
    case LIBUSB_ERROR_NOT_FOUND:
        return "LIBUSB_ERROR_NOT_FOUND";
    case LIBUSB_ERROR_BUSY:
        return "LIBUSB_ERROR_BUSY";
    case LIBUSB_ERROR_TIMEOUT:
        return "LIBUSB_ERROR_TIMEOUT";
    case LIBUSB_ERROR_OVERFLOW:
        return "LIBUSB_ERROR_OVERFLOW";
    case LIBUSB_ERROR_PIPE:
        return "LIBUSB_ERROR_PIPE";
    case LIBUSB_ERROR_INTERRUPTED:
        return "LIBUSB_ERROR_INTERRUPTED";
    case LIBUSB_ERROR_NO_MEM:
        return "LIBUSB_ERROR_NO_MEM";
    case LIBUSB_ERROR_NOT_SUPPORTED:
        return "LIBUSB_ERROR_NOT_SUPPORTED";
    default:
        return "LIBUSB_ERROR_OTHER";
    }
}

const char *libusb_strerror(enum libusb_error error_code)
{
    switch (error_code)
    {
    case LIBUSB_SUCCESS:
        return "Success";
    case LIBUSB_ERROR_IO:
        return "Input/Output Error";
    case LIBUSB_ERROR_INVALID_PARAM:
        return "Invalid parameter";
    case LIBUSB_ERROR_ACCESS:
        return "Access denied";
    case LIBUSB_ERROR_NO_DEVICE:
        return "No such device";
    case LIBUSB_ERROR_NOT_FOUND:
        return "Entity not found";
    case LIBUSB_ERROR_BUSY:
        return "Resource busy";
    case LIBUSB_ERROR_TIMEOUT:
        return "Operation timed out";
    case LIBUSB_ERROR_OVERFLOW:
        return "Overflow";
    case LIBUSB_ERROR_PIPE:
        return "Pipe error";
    case LIBUSB_ERROR_INTERRUPTED:
        return "System call interrupted";
    case LIBUSB_ERROR_NO_MEM:
        return "Insufficient memory";
    case LIBUSB_ERROR_NOT_SUPPORTED:
        return "Operation not supported - install real libusb-1.0";
    default:
        return "Other error";
    }
}