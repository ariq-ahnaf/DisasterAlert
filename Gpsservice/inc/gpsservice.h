#ifndef __gpsservice_H__
#define __gpsservice_H__

#include <dlog.h>

/* This is a sample application demonstrating proper usage of GPS module. It is a service-like application
 * running in the background, broadcasting information to another application that is supposed to present data to user.
 *
 * Using GPS module, following geolocation data are aquired:
 * - longitude
 * - latitude
 * - satellites information - number of active satellites
 *
 * Data is broadcasted to another application using bundle and message port. Coordinates are broadcasted every one second,
 * whereas satellites information is broadcasted after any change has been observed.
 *
 * 2 types of messages:
 * 1. Position update - it contains current longitude and latitude as well as tag indicating whether current position
 * is within initial circle boundary or not. It is sent every one second.
 * 2. Satellites update - it contains number of satellites in view. It is send every 5 seconds.
 *
 * Note that satellite data is not supported on Tizen Emulator.
 */

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "gpsservice"

#endif /* __gpsservice_H__ */
