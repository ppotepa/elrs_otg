/*
 * libusb.h - Header file for libusb-1.0
 * Minimal header for ELRS OTG Demo compilation
 *
 * This is a simplified version for compilation purposes.
 * For full functionality, install the complete libusb-1.0 library.
 */

#ifndef LIBUSB_H
#define LIBUSB_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#ifdef _WIN32
    typedef intptr_t ssize_t;
#else
#include <sys/types.h>
#endif

/* libusb version information */
#define LIBUSB_MAJOR_VERSION 1
#define LIBUSB_MINOR_VERSION 0
#define LIBUSB_MICRO_VERSION 27

    /* Forward declarations */
    typedef struct libusb_context libusb_context;
    typedef struct libusb_device libusb_device;
    typedef struct libusb_device_handle libusb_device_handle;

    /* Device descriptor */
    struct libusb_device_descriptor
    {
        uint8_t bLength;
        uint8_t bDescriptorType;
        uint16_t bcdUSB;
        uint8_t bDeviceClass;
        uint8_t bDeviceSubClass;
        uint8_t bDeviceProtocol;
        uint8_t bMaxPacketSize0;
        uint16_t idVendor;
        uint16_t idProduct;
        uint16_t bcdDevice;
        uint8_t iManufacturer;
        uint8_t iProduct;
        uint8_t iSerialNumber;
        uint8_t bNumConfigurations;
    };

    /* Error codes */
    enum libusb_error
    {
        LIBUSB_SUCCESS = 0,
        LIBUSB_ERROR_IO = -1,
        LIBUSB_ERROR_INVALID_PARAM = -2,
        LIBUSB_ERROR_ACCESS = -3,
        LIBUSB_ERROR_NO_DEVICE = -4,
        LIBUSB_ERROR_NOT_FOUND = -5,
        LIBUSB_ERROR_BUSY = -6,
        LIBUSB_ERROR_TIMEOUT = -7,
        LIBUSB_ERROR_OVERFLOW = -8,
        LIBUSB_ERROR_PIPE = -9,
        LIBUSB_ERROR_INTERRUPTED = -10,
        LIBUSB_ERROR_NO_MEM = -11,
        LIBUSB_ERROR_NOT_SUPPORTED = -12,
        LIBUSB_ERROR_OTHER = -99
    };

    /* Function declarations */
    int libusb_init(libusb_context **context);
    void libusb_exit(libusb_context *context);

    ssize_t libusb_get_device_list(libusb_context *context, libusb_device ***list);
    void libusb_free_device_list(libusb_device **list, int unref_devices);

    int libusb_get_device_descriptor(libusb_device *device, struct libusb_device_descriptor *desc);

    libusb_device_handle *libusb_open_device_with_vid_pid(libusb_context *context, uint16_t vendor_id, uint16_t product_id);
    int libusb_open(libusb_device *device, libusb_device_handle **handle);
    void libusb_close(libusb_device_handle *handle);

    libusb_device *libusb_get_device(libusb_device_handle *handle);

    int libusb_claim_interface(libusb_device_handle *handle, int interface_number);
    int libusb_release_interface(libusb_device_handle *handle, int interface_number);

    int libusb_kernel_driver_active(libusb_device_handle *handle, int interface_number);
    int libusb_detach_kernel_driver(libusb_device_handle *handle, int interface_number);

    int libusb_bulk_transfer(libusb_device_handle *handle, unsigned char endpoint,
                             unsigned char *data, int length, int *actual_length, unsigned int timeout);

    int libusb_get_string_descriptor_ascii(libusb_device_handle *handle, uint8_t desc_index,
                                           unsigned char *data, int length);

    const char *libusb_error_name(int error_code);
    const char *libusb_strerror(enum libusb_error error_code);

#ifdef __cplusplus
}
#endif

#endif /* LIBUSB_H */