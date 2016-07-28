#include <tizen.h>
#include <locations.h>
#include "view_manager.h"
#include "../res/edje/edje_def.h"

#define LATITUDE_TEXT "Lat:"
#define LONGITUDE_TEXT "Long:"
#define LABEL_WAITING_TEXT "Waiting for data from GPS service..."

#define MESSAGE_BOUNDARY_OUTSIDE "Boundary area exceeded"
#define MESSAGE_BOUNDARY_INSIDE "Inside boundary area"

#define BOUNDARY_CIRCLE_RADIUS_M 30.0 /* m */
#define BOUNDARY_CIRCLE_RADIUS_REL 0.0002604 /* 30m */
#define ZOOM_LEVEL 18 /*maximum supported zoom level */

#define CHAR_BUF_SIZE 20

static struct
{
	Evas_Object *win;
	Evas_Object *layout;
	Evas_Object *conform;
	Evas_Object *box;
	Evas_Object *map;

	Elm_Map_Overlay *pos_overlay;
	Elm_Map_Overlay *boundary_overlay;

	location_bounds_h bounds;
} s_view_data = {
	.win = NULL,
	.layout = NULL,
	.conform = NULL,
	.box = NULL,
	.map = NULL,

	.pos_overlay = NULL,
	.boundary_overlay = NULL
};

static void __delete_win_request_cb(void *data,
										Evas_Object *obj,
										void *event_info);
static void __layout_back_cb(void *data, Evas_Object *obj, void *event_info);
static void __get_app_resource(const char *edj_file_in,
									char *edj_path_out,
									int edj_path_max);
static Evas_Object *__create_map(Evas_Object *parent);

bool
view_manager_create_base_gui(void)
{
	Evas_Object *label;
	char edj_path[PATH_MAX] = {0, };

	/* Window */
	s_view_data.win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	if (!s_view_data.win) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create window");
		return false;
	}

	elm_win_conformant_set(s_view_data.win, EINA_TRUE);
	elm_win_autodel_set(s_view_data.win, EINA_TRUE);

	evas_object_smart_callback_add(s_view_data.win, "delete,request", __delete_win_request_cb, NULL);

	/* Conformant */
	s_view_data.conform = elm_conformant_add(s_view_data.win);
	if (!s_view_data.conform) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create conform");
		view_manager_destroy();
		return false;
	}

	elm_win_indicator_mode_set(s_view_data.win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(s_view_data.win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(s_view_data.conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(s_view_data.win, s_view_data.conform);
	evas_object_show(s_view_data.conform);

	/* Base Layout */
	__get_app_resource(EDJ_FILE, edj_path, (int)PATH_MAX);
	s_view_data.layout = elm_layout_add(s_view_data.win);
	if (!s_view_data.layout) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create layout");
		view_manager_destroy();
		return false;
	}

	if (!elm_layout_file_set(s_view_data.layout, edj_path, GRP_MAIN)) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to set layout file");
		view_manager_destroy();
		return false;
	}

	elm_object_part_text_set(s_view_data.layout, LAYOUT_PART_SATELLITE_TEXT, "");
	elm_object_part_text_set(s_view_data.layout, LAYOUT_PART_LATITUDE_TEXT, LATITUDE_TEXT);
	elm_object_part_text_set(s_view_data.layout, LAYOUT_PART_LONGITUDE_TEXT, LONGITUDE_TEXT);
	elm_object_part_text_set(s_view_data.layout, LAYOUT_PART_MESSAGE_TEXT, "");

	/* Map box */
	s_view_data.box = elm_box_add(s_view_data.win);
	if (!s_view_data.box) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create box");
		view_manager_destroy();
		return false;
	}

	evas_object_size_hint_weight_set(s_view_data.box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_show(s_view_data.box);

	label = elm_label_add(s_view_data.win);
	if (!label) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create label");
		view_manager_destroy();
		return false;
	}

	elm_object_text_set(label, LABEL_WAITING_TEXT);
	evas_object_show(label);

	elm_box_pack_end(s_view_data.box, label);
	elm_object_part_content_set(s_view_data.layout, LAYOUT_PART_MAP_AREA, s_view_data.box);

	evas_object_size_hint_weight_set(s_view_data.layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	eext_object_event_callback_add(s_view_data.layout, EEXT_CALLBACK_BACK, __layout_back_cb, NULL);
	elm_object_content_set(s_view_data.conform, s_view_data.layout);

	/* Show window after base gui is set up */
	evas_object_show(s_view_data.win);

	return true;
}

void
view_manager_destroy(void)
{
	elm_map_overlay_del(s_view_data.pos_overlay);
	elm_map_overlay_del(s_view_data.boundary_overlay);

	evas_object_del(s_view_data.map);
	evas_object_del(s_view_data.box);
	evas_object_del(s_view_data.conform);
	evas_object_del(s_view_data.layout);
	evas_object_del(s_view_data.win);

	location_bounds_destroy(s_view_data.bounds);

	s_view_data.win = NULL;
	s_view_data.layout = NULL;
	s_view_data.conform = NULL;
	s_view_data.box = NULL;
	s_view_data.map = NULL;
	s_view_data.pos_overlay = NULL;
	s_view_data.boundary_overlay = NULL;
}

void
view_manager_create_map_with_circle_boundary(double circle_longitude, double circle_latitude)
{
	location_coords_s circle_coords;
	if (!s_view_data.box || s_view_data.map) {
		return;
	}

	/* Delete box content - initial label */
	elm_box_clear(s_view_data.box);

	/* Create map and add it to map box */
	s_view_data.map = __create_map(s_view_data.win);
	if (!s_view_data.map) {
		return;
	}

	elm_box_pack_end(s_view_data.box, s_view_data.map);

	/* Create circle boundary */
	dlog_print(DLOG_INFO, LOG_TAG, "Circle boundary overlay at %lf, %lf", circle_longitude, circle_latitude);

	s_view_data.boundary_overlay = elm_map_overlay_circle_add(s_view_data.map, circle_longitude, circle_latitude, BOUNDARY_CIRCLE_RADIUS_REL);
	if (!s_view_data.boundary_overlay) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create boundary overlay");
		return;
	}

	elm_map_overlay_displayed_zoom_min_set(s_view_data.boundary_overlay, ZOOM_LEVEL);

	/* Create location bounds - BOUNDARY_CIRCLE_RADIUS [m] circle with center defined by initial coordinates */
	circle_coords.longitude = circle_longitude;
	circle_coords.latitude = circle_latitude;

	dlog_print(DLOG_INFO, LOG_TAG, "center: %lf %lf, radius %lf", circle_coords.longitude, circle_coords.latitude, BOUNDARY_CIRCLE_RADIUS_M);
	int ret = location_bounds_create_circle(circle_coords, BOUNDARY_CIRCLE_RADIUS_M, &s_view_data.bounds);
	if (ret != LOCATIONS_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create circle bound: error %d", ret);
		return;
	}
	dlog_print(DLOG_INFO, LOG_TAG, "Circle bounds created - %.1fm radius, center at %f, %f", BOUNDARY_CIRCLE_RADIUS_M, circle_coords.longitude, circle_coords.latitude);
}

void
view_manager_update_message(char *message)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Update message to: %s", message);

	elm_object_part_text_set(s_view_data.layout, LAYOUT_PART_MESSAGE_TEXT, message);
}

void
view_manager_update_satellites_count(char *count_str)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Update satellites count to %s", count_str);

	elm_object_part_text_set(s_view_data.layout, LAYOUT_PART_SATELLITE_TEXT, count_str);
}

void
view_manager_update_map_position(double longitude, double latitude)
{
	char lat_info[CHAR_BUF_SIZE];
	char long_info[CHAR_BUF_SIZE];
	bool contains;
	location_coords_s coords;

	if (!s_view_data.map) {
		/* First position update message - create map with boundary circle */
		view_manager_create_map_with_circle_boundary(longitude, latitude);
	}

	/* Set position overlay to new coordinates */
	dlog_print(DLOG_INFO, LOG_TAG, "Update position to %lf %lf", longitude, latitude);

	if (!s_view_data.pos_overlay) {
		/* Create position overlay */
		s_view_data.pos_overlay = elm_map_overlay_add(s_view_data.map, longitude, latitude);
		if (!s_view_data.pos_overlay) {
			dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create position overlay");
			return;
		}

		elm_map_overlay_displayed_zoom_min_set(s_view_data.pos_overlay, ZOOM_LEVEL);
		elm_map_overlay_show(s_view_data.pos_overlay);
	} else {
		/* Move position overlay */
		elm_map_overlay_region_set(s_view_data.pos_overlay, longitude, latitude);
	}

	/* Show current region */
	elm_map_region_bring_in(s_view_data.map, longitude, latitude);

	/* Display current position in information area */
	snprintf(lat_info, CHAR_BUF_SIZE, "%s %lf", LATITUDE_TEXT, latitude);
	snprintf(long_info, CHAR_BUF_SIZE, "%s %lf", LONGITUDE_TEXT, longitude);

	elm_object_part_text_set(s_view_data.layout, LAYOUT_PART_LATITUDE_TEXT, lat_info);
	elm_object_part_text_set(s_view_data.layout, LAYOUT_PART_LONGITUDE_TEXT, long_info);

	/* Set message in message area depending on whether given coordinates are within
	 * boundary circle or not */
	coords.longitude = longitude;
	coords.latitude = latitude;

	contains = location_bounds_contains_coordinates(s_view_data.bounds, coords);

	contains ? view_manager_update_message(MESSAGE_BOUNDARY_INSIDE)
			: view_manager_update_message(MESSAGE_BOUNDARY_OUTSIDE);
}

/* Static functions */
static void
__delete_win_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void
__layout_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	/* Let window go to hide state. */
	elm_win_lower(s_view_data.win);
}

static void
__get_app_resource(const char *edj_file_in, char *edj_path_out, int edj_path_max)
{
	char *res_path = app_get_resource_path();
	if (res_path) {
		snprintf(edj_path_out, edj_path_max, "%s%s", res_path, edj_file_in);
		free(res_path);
	}
}

static Evas_Object*
__create_map(Evas_Object *parent)
{
	Evas_Object *map = elm_map_add(parent);
	if (!map) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Failed to create map");
		return NULL;
	}

	evas_object_size_hint_weight_set(map, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(map, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(map);

	elm_map_zoom_set(map, ZOOM_LEVEL);

	return map;
}
