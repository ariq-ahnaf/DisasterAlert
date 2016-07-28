#ifndef __location_manager_H__
#define __location_manager_H__

#define REMOTE_APP_ID "org.tizen.gpsservice-consumer"
#define REMOTE_PORT "gps-consumer-port"

/*
 * Initialize location manager and set callback for position and satellite data updates
 */
bool geolocation_manager_init(void);

/*
 * Stop geolocation service
 */
void geolocation_manager_stop_service(void);

/*
 * Destroy geolocation service
 */
void geolocation_manager_destroy_service(void);

#endif /* __location_manager_H__ */

