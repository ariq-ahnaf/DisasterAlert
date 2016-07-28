#ifndef __gpsservice-consumer_H__
#define __gpsservice-consumer_H__

#include <app.h>
#include <Elementary.h>
#include <system_settings.h>
#include <efl_extension.h>
#include <dlog.h>


/* This is a sample application that is responsible for displaying geolocation and satellites information
 * aquired from gps-service via message port. Coordinates (longitude and latitude) are aquired with one second interval,
 * whereas satellites information every 5 seconds.
 *
 * Application window consists of three areas:
 * - information area - displays longitude, latitude, number of visible satellites as well as an indication of sufficient
 * number of satellites for position tracking purpose
 * - message area - displays other information received from gps-service: 1. Boundary area exceeded - if current position
 * goes outside defined boundary, 2. Inside boundary aread - if current position goes outside defined boundary.
 * - map - local area map is displayed and current position is marked. At the current position, the circular boundary
 * is set with radius equal to 30m.  Current position marker is updated according to acquired geolocation data.
 *
 */

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "gpsservice-consumer"

#if !defined(PACKAGE)
#define PACKAGE "org.tizen.gpsservice-consumer"
#endif

#endif /* __gpsservice-consumer__ */
