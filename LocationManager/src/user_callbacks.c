/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "user_callbacks.h"
#include "main.h"

static int numofactive = 0;
static int numofinview = 0;
static double user_latitude = 0.0;
static double user_longitude = 0.0;
static double user_speed = 0.0;
static double user_direction = 0.0;
static double user_climb = 0.0;
static int cur_azimuth, cur_elevation, cur_prn, cur_snr;
location_manager_h manager = NULL;
location_bounds_h bounds_poly = NULL;
Evas_Object *start, *stop;

char *_accuracy_level_to_string(location_accuracy_level_e level)
{
    switch (level) {
    case LOCATIONS_ACCURACY_NONE:
        return "LOCATIONS_ACCURACY_NONE";

    case LOCATIONS_ACCURACY_COUNTRY:
        return "LOCATIONS_ACCURACY_COUNTRY";

    case LOCATIONS_ACCURACY_REGION:
        return "LOCATIONS_ACCURACY_REGION";

    case LOCATIONS_ACCURACY_LOCALITY:
        return "LOCATIONS_ACCURACY_LOCALITY";

    case LOCATIONS_ACCURACY_POSTALCODE:
        return "LOCATIONS_ACCURACY_POSTALCODE";

    case LOCATIONS_ACCURACY_STREET:
        return "LOCATIONS_ACCURACY_STREET";

    case LOCATIONS_ACCURACY_DETAILED:
        return "LOCATIONS_ACCURACY_DETAILED";

    default:
        return "Unknown";
    }
}

void _location_bounds_state_changed_cb(location_boundary_state_e state, void *user_data)
{
    PRINT_MSG("New bound state %s", state == LOCATIONS_BOUNDARY_IN ? "IN" : "OUT");
    dlog_print(DLOG_DEBUG, LOG_TAG, "New bound state %d", state);

}

/* When the update is received, updates the variables that store the current position */
static void position_updated(double latitude, double longitude, double altitude, time_t timestamp,
                             void *user_data)
{
    user_latitude = latitude;
    user_longitude = longitude;
    PRINT_MSG("latitude: %f, longitude: %f", latitude, longitude);
    dlog_print(DLOG_DEBUG, LOG_TAG, "latitude: %f, longitude: %f\n", latitude, longitude);
}

/* When the update is received, updates the variables that store the current velocity */
static void velocity_updated(double speed, double direction, double climb, time_t timestamp,
                             void *user_data)
{
    user_speed = speed;
    user_direction = direction;
    user_climb = climb;
    PRINT_MSG("speed: %f, direction: %f, climb: %f", speed, direction, climb);
    dlog_print(DLOG_DEBUG, LOG_TAG, "speed: %f, longitude: %f, climb: %f\n", speed, direction,
               climb);
}

location_service_state_e state_;

/* Called when the service state changes */
static void __state_changed_cb(location_service_state_e state, void *user_data)
{
    PRINT_MSG("__state_changed_cb new state: %s", state == LOCATIONS_SERVICE_ENABLED ?
              "LOCATIONS_SERVICE_ENABLED" : "LOCATIONS_SERVICE_DISABLED");
    dlog_print(DLOG_DEBUG, LOG_TAG, "__state_changed_cb %s\n", state == LOCATIONS_SERVICE_ENABLED ?
               "LOCATIONS_SERVICE_ENABLED" : "LOCATIONS_SERVICE_DISABLED");
    state_ = state;

    /* Get the current location information */
    double altitude, latitude, longitude, climb, direction, speed;
    double horizontal, vertical;
    location_accuracy_level_e level;
    time_t timestamp;
    int ret = 0;

    if (state == LOCATIONS_SERVICE_ENABLED) {
        ret = location_manager_get_location(manager, &altitude, &latitude, &longitude,
                                            &climb, &direction, &speed, &level,
                                            &horizontal, &vertical, &timestamp);
        if (LOCATIONS_ERROR_NONE != ret) {
            PRINT_MSG("Error %s",
                      (LOCATIONS_ERROR_SERVICE_NOT_AVAILABLE ==
                       ret) ? "LOCATIONS_ERROR_SERVICE_NOT_AVAILABLE" : "Other error");
        }

        PRINT_MSG("Current location: A:%f La:%f Lo:%f ", altitude, latitude, longitude);
        dlog_print(DLOG_DEBUG, LOG_TAG, "Current location: A:%f La:%f Lo:%f",
                   altitude, latitude, longitude);
    }
}

static bool poly_coords_cb(location_coords_s coords, void *user_data)
{
    PRINT_MSG("location_bounds_foreach_rect_coords(latitude : %lf, longitude: %lf) \n",
              coords.latitude, coords.longitude);
    dlog_print(DLOG_DEBUG, LOG_TAG,
               "location_bounds_foreach_rect_coords(latitude : %lf, longitude: %lf) \n",
               coords.latitude, coords.longitude);
    return true;
}

static bool gps_get_satellites_cb(unsigned int azimuth, unsigned int elevation, unsigned int prn,
                                  int snr, bool is_in_use, void *user_data)
{
    cur_azimuth = azimuth;
    cur_elevation = elevation;
    cur_prn = prn;
    cur_snr = snr;
    PRINT_MSG("azimuth: %d, elevation: %d, prn: %d, snr: %d", azimuth, elevation, prn, snr);
    dlog_print(DLOG_DEBUG, LOG_TAG, "azimuth: %d, elevation: %d, prn: %d, snr: %d", azimuth,
               elevation, prn, snr);
    return true;
}

static void gps_satellite_updated_cb(int num_of_active, int num_of_inview, time_t timestamp,
                                     void *user_data)
{
    numofactive = num_of_active;
    numofinview = num_of_inview;
    PRINT_MSG("Satellites: active: %d, inview: %d", numofactive, numofinview);
    dlog_print(DLOG_DEBUG, LOG_TAG, "Satellites: active: %d, view: %d", numofactive, numofinview);

    if (num_of_inview > 0) {
        int ret = gps_status_foreach_satellites_in_view(manager, gps_get_satellites_cb, NULL);
        if (LOCATIONS_ERROR_NONE != ret) {
            PRINT_MSG("gps_status_foreach_satellites_in_view failed : %d", ret);
            dlog_print(DLOG_ERROR, LOG_TAG, "gps_status_foreach_satellites_in_view failed : %d", ret);
        }
    }
}

/* Get the Last Known Location */
void _get_last_location_cb(appdata_s *ad, Evas_Object *obj, void *event_info)
{
    if (state_ != LOCATIONS_SERVICE_ENABLED) {
        PRINT_MSG("state is not LOCATIONS_SERVICE_ENABLED");
        dlog_print(DLOG_ERROR, LOG_TAG, "state is not LOCATIONS_SERVICE_ENABLED");
        return;
    }

    int ret = 0;
    PRINT_MSG("_get_last_location_cb");
    dlog_print(DLOG_DEBUG, LOG_TAG, "_get_last_location_cb");

    double altitude = 0, latitude = 0, longitude = 0, climb = 0, direction = 0, speed = 0;
    double horizontal = 0, vertical = 0;
    location_accuracy_level_e level;
    time_t timestamp;
    /* Get the last location information including altitude, latitude, and direction */
    ret = location_manager_get_last_location(manager, &altitude, &latitude, &longitude,
            &climb, &direction, &speed, &level, &horizontal,
            &vertical, &timestamp);
    if (LOCATIONS_ERROR_NONE != ret) {
        dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_get_last_location failed : %d", ret);
        PRINT_MSG("location_manager_get_last_location failed : %d", ret);
    } else {
        PRINT_MSG("Last location: A:%f La:%f Lo:%f ", altitude, latitude, longitude);
        dlog_print(DLOG_DEBUG, LOG_TAG, "Last location: A:%f La:%f Lo:%f",
                   altitude, latitude, longitude);
    }
}

/* Cancel location Updates */
void _cancel_location_updates_cb(appdata_s *ad, Evas_Object *obj, void *event_info)
{
    if (manager == NULL) {
        PRINT_MSG("Error: manager == NULL");
        dlog_print(DLOG_ERROR, LOG_TAG, "Error: manager == NULL");
        return;
    }

    /* Register the position update callback */
    int ret = location_manager_unset_position_updated_cb(manager);
    PRINT_MSG("location_manager_unset_position_updated_cb: %d", ret);
    dlog_print(DLOG_DEBUG, LOG_TAG, "location_manager_unset_position_updated_cb: %d", ret);
}

/* Use Location Bounds */
void _get_location_bounds_cb(appdata_s *ad, Evas_Object *obj, void *event_info)
{
    int ret = 0;
    PRINT_MSG("_get_location_bounds_cb");
    dlog_print(DLOG_DEBUG, LOG_TAG, "_get_location_bounds_cb");

    if (manager == NULL) {
        PRINT_MSG("Error: manager == NULL");
        dlog_print(DLOG_ERROR, LOG_TAG, "Error: manager == NULL");
        return;
    }

    /* Create location bounds with the required type */
    int poly_size = 3;
    location_coords_s coord_list[poly_size];
    coord_list[0].latitude = 37;
    coord_list[0].longitude = 126;
    coord_list[1].latitude = 38;
    coord_list[1].longitude = 128;
    coord_list[2].latitude = 35;
    coord_list[2].longitude = 128;

    ret = location_bounds_create_polygon(coord_list, poly_size, &bounds_poly);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("location_bounds_create_polygon failed : %d", ret);
        dlog_print(DLOG_ERROR, LOG_TAG, "location_bounds_create_polygon failed : %d", ret);
    }

    /* Get the coordinates of the generated polygon bounds */
    ret = location_bounds_foreach_polygon_coords(bounds_poly, poly_coords_cb, NULL);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("location_bounds_foreach_polygon_coords failed : %d", ret);
        dlog_print(DLOG_ERROR, LOG_TAG, "location_bounds_foreach_polygon_coords failed : %d", ret);
    }

    /* Register the callback */
    ret =
        location_bounds_set_state_changed_cb(bounds_poly, _location_bounds_state_changed_cb, NULL);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("location_bounds_set_state_changed_cb failed : %d", ret);
        dlog_print(DLOG_ERROR, LOG_TAG, "location_bounds_set_state_changed_cb failed : %d", ret);
    }

    /* Get the boundary information */
    ret = location_manager_add_boundary(manager, bounds_poly);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("location_manager_add_boundary failed : %d", ret);
        dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_add_boundary failed : %d", ret);
    }
}

/* Get Satellite Information */
void _get_satellite_information_cb(appdata_s *ad, Evas_Object *obj, void *event_info)
{
    if (state_ != LOCATIONS_SERVICE_ENABLED) {
        PRINT_MSG("state is not LOCATIONS_SERVICE_ENABLED");
        dlog_print(DLOG_ERROR, LOG_TAG, "state is not LOCATIONS_SERVICE_ENABLED");
        return;
    }

    PRINT_MSG("_get_satellite_information_cb");
    dlog_print(DLOG_DEBUG, LOG_TAG, "_get_satellite_information_cb");

    if (manager == NULL) {
        PRINT_MSG("Error: manager == NULL");
        dlog_print(DLOG_ERROR, LOG_TAG, "Error: manager == NULL");
        return;
    }

    /* Register the callback */
    int ret = gps_status_set_satellite_updated_cb(manager, gps_satellite_updated_cb, 10, NULL);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("gps_status_set_satellite_updated_cb failed : %d", ret);
        dlog_print(DLOG_DEBUG, LOG_TAG, "gps_status_set_satellite_updated_cb failed : %d", ret);
    }
}

/* Tracking the Route */
void _track_the_route_cb(appdata_s *ad, Evas_Object *obj, void *event_info)
{
    if (state_ != LOCATIONS_SERVICE_ENABLED) {
        PRINT_MSG("state is not LOCATIONS_SERVICE_ENABLED");
        dlog_print(DLOG_ERROR, LOG_TAG, "state is not LOCATIONS_SERVICE_ENABLED");
        return;
    }

    if (manager == NULL) {
        PRINT_MSG("Error: manager == NULL");
        dlog_print(DLOG_ERROR, LOG_TAG, "Error: manager == NULL");
        return;
    }

    /* Register the position update callback */
    int ret = location_manager_set_position_updated_cb(manager, position_updated, 2, NULL);
    PRINT_MSG("location_manager_set_position_updated_cb: %d", ret);
    dlog_print(DLOG_DEBUG, LOG_TAG, "location_manager_set_position_updated_cb: %d", ret);

    /* Register velocity callback */
    ret = location_manager_set_velocity_updated_cb(manager, velocity_updated, 2, NULL);
    PRINT_MSG("location_manager_set_position_updated_cb: %d", ret);
    dlog_print(DLOG_DEBUG, LOG_TAG, "location_manager_set_position_updated_cb: %d", ret);

    /* Get information about the current position */
    time_t timestamp;
    double altitude;
    double latitude;
    double longitude;
    ret = location_manager_get_position(manager, &altitude, &latitude, &longitude, &timestamp);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("location_manager_get_position failed : %d", ret);
        dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_get_position failed : %d", ret);
    } else {
        PRINT_MSG("Current position: A:%f La:%f Lo:%f ", altitude, latitude, longitude);
        dlog_print(DLOG_DEBUG, LOG_TAG, "Current position: A:%f La:%f Lo:%f",
                   altitude, latitude, longitude);
    }

    /* Get information about the current velocity */
    double climb;
    double direction;
    double speed;
    ret = location_manager_get_velocity(manager, &climb, &direction, &speed, &timestamp);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("location_manager_get_velocity failed : %d", ret);
        dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_get_velocity failed : %d", ret);
    } else {
        PRINT_MSG("Current velocity: Climb:%f Direction:%f Speed:%f ", climb, direction, speed);
        dlog_print(DLOG_DEBUG, LOG_TAG, "Current velocity: Climb:%f Direction:%f Speed:%f",
                   climb, direction, speed);
    }

    /* Get information about the current accuracy */
    location_accuracy_level_e level;
    double horizontal;
    double vertical;
    ret = location_manager_get_accuracy(manager, &level, &horizontal, &vertical);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("location_manager_get_accuracy failed : %d", ret);
        dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_get_accuracy failed : %d", ret);
    } else {
        PRINT_MSG("Current accuracy: Level:%d Horizontal:%f Vertical:%f ", level, horizontal,
                  vertical);
        dlog_print(DLOG_DEBUG, LOG_TAG, "Current accuracy: Level:%f Horizontal:%f Vertical:%f",
                   level, horizontal, vertical);
    }

    /*
     * To get all of above values please use
     * location_manager_get_location(manager, &altitude, &latitude, &longitude,
     *      &climb, &direction, &speed, &level, &horizontal, &vertical, &timestamp);
     */

    /* Get the distance */
    double distance;
    ret = location_manager_get_distance(37.28, 127.01, latitude, longitude, &distance);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("location_manager_get_distance failed : %d", ret);
        dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_get_distance failed : %d", ret);
    } else {
        PRINT_MSG("The distance between (37.28, 127.01) and current position: %f ", distance);
        dlog_print(DLOG_DEBUG, LOG_TAG,
                   "The distance between (37.28, 127.01) and current position: %f ", distance);
    }
}

void _location_init(void)
{
    bool is_enabled = false;
    int ret = location_manager_is_enabled_method(LOCATIONS_METHOD_HYBRID, &is_enabled);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("location_manager_is_enabled_method failed : %d", ret);
        dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_is_enabled_method failed : %d", ret);
        return;
    }

    if (true != is_enabled) {
        state_ = LOCATIONS_SERVICE_DISABLED;
        PRINT_MSG
        ("Location service is disabled. Please switch it on and tap \"Initialize location service\" button.");
        return;
    }

    state_ = LOCATIONS_SERVICE_ENABLED;
    /* Create a location manager handle */
    ret = location_manager_create(LOCATIONS_METHOD_HYBRID, &manager);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("location_manager_create failed : %d", ret);
        dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_create failed : %d", ret);
        return;
    }

    PRINT_MSG("location_manager_create success");

    /* To know when the service becomes enabled */
    ret = location_manager_set_service_state_changed_cb(manager, __state_changed_cb, NULL);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("location_manager_set_service_state_changed_cb failed : %d", ret);
        dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_set_service_state_changed_cb failed : %d", ret);
    }

    /* Start the location service */
    ret = location_manager_start(manager);
    if (LOCATIONS_ERROR_NONE != ret) {
        PRINT_MSG("location_manager_start failed : %d", ret);
        dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_start failed : %d", ret);
        return;
    }

    PRINT_MSG("location_manager_start success");
    dlog_print(DLOG_ERROR, LOG_TAG, "location_manager_start success");
    PRINT_MSG("Wait for state LOCATIONS_SERVICE_ENABLED...");
    elm_object_disabled_set(start, EINA_TRUE);
    elm_object_disabled_set(stop, EINA_FALSE);
}

void _location_deinitialize(void)
{
    if (elm_object_disabled_get(start)) {
        /* Destroy the polygon bounds */
        int ret = location_bounds_destroy(bounds_poly);
        bounds_poly = NULL;

        /* Unset all connected callback functions */

        ret = gps_status_unset_satellite_updated_cb(manager);
        dlog_print(DLOG_DEBUG, LOG_TAG, "gps_status_set_satellite_updated_cb: %d", ret);

        ret = location_manager_unset_service_state_changed_cb(manager);
        dlog_print(DLOG_DEBUG, LOG_TAG, "location_manager_unset_service_state_changed_cb: %d", ret);

        /* Stop the Location Manager */
        ret = location_manager_stop(manager);
        dlog_print(DLOG_DEBUG, LOG_TAG, "location_manager_stop: %d", ret);
        PRINT_MSG("location_manager_stop success");

        /* Destroy the Location Manager */
        ret = location_manager_destroy(manager);
        manager = NULL;
        dlog_print(DLOG_DEBUG, LOG_TAG, "location_manager_destroy: %d", ret);
        elm_object_disabled_set(start, EINA_FALSE);
        elm_object_disabled_set(stop, EINA_TRUE);
    }
}

void create_buttons_in_main_window(appdata_s *ad)
{
    Evas_Object *display = _create_new_cd_display(ad, "Location Manager", _pop_cb);

    start = _new_button(ad, display, "Initialize location service", _location_init);
    _new_button(ad, display, "Get last location", _get_last_location_cb);
    _new_button(ad, display, "Location bounds", _get_location_bounds_cb);
    _new_button(ad, display, "Satellite information", _get_satellite_information_cb);
    _new_button(ad, display, "Track the route", _track_the_route_cb);
    _new_button(ad, display, "Cancel route tracking", _cancel_location_updates_cb);
    stop = _new_button(ad, display, "Deinitialize location service", _location_deinitialize);
    elm_object_disabled_set(start, EINA_FALSE);
    elm_object_disabled_set(stop, EINA_TRUE);
}
