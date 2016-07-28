#ifndef __view_manager_H__
#define __view_manager_H__

#include "gpsservice-consumer.h"

/*
 * Create application's window and its content
*/
bool view_manager_create_base_gui(void);

/*
 * Destroy application's window and its content
*/
void view_manager_destroy(void);

/*
 * Update displayed message
*/
void view_manager_update_message(char *message);

/*
 * Update displayed satellites in view count
*/
void view_manager_update_satellites_count(char *count_str);

/*
 * Create map and circle boundary overlay with given coordinates
*/
void view_manager_create_map_with_circle_boundary(double longitude, double latitude);

/*
 * Update displayed current position overlay to given coordinates
 * If position overlay doesn't exist, create it
*/
void view_manager_update_map_position(double longitude, double latitude);

#endif /* __view_manager__ */
