/*
 * This file is part of libnavigation
 *
 * Copyright (C) 2009 Nokia Corporation. All rights reserved.
 *
 * Contact: Quanyi Sun <Quanyi.Sun@nokia.com>
 *
 * This software, including documentation, is protected by copyright
 * controlled by Nokia Corporation. All rights are reserved. Copying,
 * including reproducing, storing, adapting or translating, any or all
 * of this material requires the prior written consent of Nokia
 * Corporation. This material also contains confidential information
 * which may not be disclosed to others without the prior written
 * consent of Nokia.
 */

#ifndef __NAVIGATION_MAP_H__
#define __NAVIGATION_MAP_H__

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <navigation/navigation-provider-enums.h>

G_BEGIN_DECLS

#define NAVIGATION_TYPE_PROVIDER (navigation_provider_get_type ())
#define NAVIGATION_PROVIDER(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), NAVIGATION_TYPE_PROVIDER, NavigationProvider))
#define NAVIGATION_IS_PROVIDER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), NAVIGATION_TYPE_PROVIDER))

struct _NavigationProvider
{
  GObject parent;
};

typedef struct _NavigationProvider NavigationProvider;

struct _NavigationProviderClass
{
  GObjectClass parent_class;
};

typedef struct _NavigationProviderClass NavigationProviderClass;

#define NAVIGATION_HOUSE_NUMBER		"house_number"
#define NAVIGATION_HOUSE_NAME		"house_name"
#define NAVIGATION_STREET_NAME		"street_name"
#define NAVIGATION_SUBURB_NAME		"suburb_name"
#define NAVIGATION_TOWN_NAME		"town_name"
#define NAVIGATION_MUNICIPALITY_NAME	"municipality_name"
#define NAVIGATION_PROVINCE_NAME	"province_name"
#define NAVIGATION_POSTAL_CODE		"postal_code"
#define NAVIGATION_COUNTRY_NAME		"country_name"
#define NAVIGATION_COUNTRY_CODE		"country_code"
#define NAVIGATION_TIME_ZONE		"time_zone"

GQuark navigation_error_quark (void);

/**
 * NAVIGATION_ERROR:
 *
 * Get access to the error quark of libnavigation.
 */
#define NAVIGATION_ERROR navigation_error_quark ()

/**
 * NavigationError:
 * @NAVIGATION_ERROR_TOO_MANY_REQUESTS: Too frequent requests
 */
typedef enum {
        NAVIGATION_ERROR_TOO_MANY_REQUESTS,
        NAVIGATION_ERROR_USER_CANCELED_OPERATION,
} NavigationError;


/**
 * NavigationProviderDetails:
 * @name: The name of the navigation provider
 * @service: The D-Bus service name
 * @description: A text description of the provider
 *
 * A struct representing an available navigation provider
 */
typedef struct _NavigationProviderDetails {
	char *name;
	char *service;
	char *description;
} NavigationProviderDetails;

/**
 * NavigationLocation:
 * @latitude: The latitude of the location
 * @longitude: The longitude of the location
 *
 * A struct representing a 2D location
 */
typedef struct _NavigationLocation {
	double latitude;
	double longitude;
} NavigationLocation;

/**
 * NavigationAddress:
 * @house_num: House number
 * @house_name: House name
 * @street: Street
 * @suburb: Suburb
 * @town: Town
 * @municipality: Municipality
 * @province: Province
 * @postal_code: Postal code
 * @country: Country
 * @country_code: Country code
 * @time_zone: Time zone
 *
 * A struct containing address information. Values can be NULL if information
 * is not available.
 */
typedef struct _NavigationAddress {
  char *house_num;
  char *house_name;
  char *street;
  char *suburb;
  char *town;
  char *municipality;
  char *province;
  char *postal_code;
  char *country;
  char *country_code;
  char *time_zone;
  /*< private >*/
  /* Reserved for future use */
  void *reserved1;
  void *reserved2;
  void *reserved3;
  void *reserved4;
} NavigationAddress;

/**
 * NavigationArea:
 * @nw: Coordinates of northwest corner of area
 * @se: Coordinates of southeast corner of area
 *
 * A struct representing a 2D area
 */
typedef struct _NavigationArea {
	NavigationLocation nw;
	NavigationLocation se;
} NavigationArea;

typedef struct _NavigationMap {
	NavigationArea  area;
	gint            px_width;
	gint            px_height;
} NavigationMap;

/**
 * navigation_provider_new_default:
 *
 * Creates a new #NavigationProvider with the default provider.
 *
 * Return value: A #NavigationProvider object
 */
NavigationProvider *navigation_provider_new_default (void);

/**
 * navigation_make_resudent
 *
 * Loads the navigation library into memory and makes it resident so that it
 * cannot be unloaded.
 */
void navigation_make_resident (void);

/**
 * navigation_provider_list_all:
 *
 * Generates a list of all the navigation provider details that are registered
 * with the system.
 *
 * Return value: A list of #NavigationProviderDetails
 */
GList *navigation_provider_list_all (void);

/**
 * navigation_provider_list_free:
 * @providers: A list of #NavigationProviderDetails
 *
 * Frees all resources used in @providers
 */
void navigation_provider_list_free (GList *providers);

/**
 * navigation_provider_get_default_service:
 *
 * Gets the default service string.
 *
 * Return value: Newly allocated string that needs to be freed with g_free
 */
char *navigation_provider_get_default_service (void);

/**
 * navigation_provider_set_default_service:
 * @service: The service name
 *
 * Sets the default service to @service
 */
void navigation_provider_set_default_service (const char *service);

/**
 * navigation_address_free:
 * @address: the #NavigationAddress data type to be freed
 *
 * Frees the allocated #NavigationAddress data
 */
void navigation_address_free (NavigationAddress *address);

/**
 * navigation_location_free:
 * @location: the #NavigationLocation data type to be freed
 *
 * Frees the allocated #NavigationLocation data
 */
void navigation_location_free (NavigationLocation *location);

/**
 * address_to_array:
 * @address: Address that will be copied to string array.
 *
 * Creates string array from address. Used internally in DBus communication.
 * 
 * Return value: Newly allocated string array containing all #NavigationAddress
 * structs fields.
 */
gchar ** address_to_array (const NavigationAddress *address);

/**
 * navigation_address_copy:
 * @address: Address that will be coped
 * 
 * Copy #NavigationAddress struct.
 *
 * Return value: Newly allocated #NavigationAddress struct where address
 * information is copied.
 */
NavigationAddress * navigation_address_copy (NavigationAddress *address);

/**
 * NavigationProviderLocationToAddressCallback:
 * @provider: A #NavigationProvider
 * @address: A #NavigationAddress that should be freed after use
 * @userdata: The userdata passed into #navigation_provider_location_to_address
 *
 * Type of the callback function for #navigation_provider_location_to_address
 * which is called whenever @provider has returned a address.
 * 
 * Note: Make sure you free address after use. You can use navigation_address_free() for that. 
 */
typedef void (* NavigationProviderLocationToAddressCallback) (NavigationProvider *provider, 
                                                              NavigationAddress  *address, 
				                              gpointer           userdata);

/**
 * navigation_provider_location_to_address:
 * @provider: A #NavigationProvider object
 * @location: A #NavigationLocation
 * @cb: A #NavigationProviderLocationToAddressCallback
 * @userdata: The data to be passed to @cb
 *
 * Uses @provider object to convert @location to an address string
 *
 * Return value: TRUE on success, FALSE otherwise.
 */
gboolean
navigation_provider_location_to_address (NavigationProvider *provider, 
                                         NavigationLocation *location,
                                         NavigationProviderLocationToAddressCallback cb,
                                         gpointer            userdata,
                                         GError             **error);

/**
 * NavigationProviderLocationToAddressVerboseCallback:
 * @provider: A #NavigationProvider
 * @address: A #NavigationAddress that should be freed after use
 * @error: A possible error that should be freed after use
 * @userdata: The userdata passed into #navigation_provider_location_to_address
 *
 * Type of the callback function for #navigation_provider_location_to_address_verbose
 * which is called whenever @provider has returned a address or an error.
 * 
 * Note: Make sure you free address and error after use. You can use navigation_address_free() for that.
 */
typedef void (* NavigationProviderLocationToAddressVerboseCallback) (NavigationProvider *provider, 
                                                              NavigationAddress  *address,
                                                              GError             *error,
				                              gpointer           userdata);

/**
 * navigation_provider_location_to_address_verbose:
 * @provider: A #NavigationProvider object
 * @location: A #NavigationLocation
 * @cb: A #NavigationProviderLocationToAddressVerboseCallback
 * @userdata: The data to be passed to @cb
 *
 * Uses @provider object to convert @location to an address string. A verbose version that will
 * give error if user canceled operation (example closed network opening dialog).
 *
 * Return value: TRUE on success, FALSE otherwise.
 */
gboolean
navigation_provider_location_to_address_verbose (NavigationProvider *provider, 
                                         NavigationLocation *location,
                                         NavigationProviderLocationToAddressVerboseCallback cb,
                                         gpointer            userdata,
                                         GError             **error);

/**
 * navigation_provider_location_to_address_cached:
 * @provider: A #NavigationProvider object
 * @location: A #NavigationLocation
 * @tolerance: A tolerance in meters how far cached value can be from requested
 * @address: A #NavigationAddress that should be freed after use
 *
 * Uses @provider object to convert @location to an address string.
 * Only tries to search location from navigation providers cache so
 * that network connection is not needed. Always returns closest address.
 *
 * Return value: TRUE on success, FALSE otherwise.
 */
gboolean
navigation_provider_location_to_address_cached (NavigationProvider *provider, 
                                                NavigationLocation *location,
                                                gdouble            tolerance,
                                                NavigationAddress  **address,
                                                GError             **error);

/**
 * navigation_address_list_free:
 * @addresses: #GSList of #NavigationAddress data types to be freed
 *
 * Frees the allocated #GSList and #NavigationAddresses data
 */
void navigation_address_list_free (GSList *addresses);

/**
 * NavigationProviderAddressToLocationCallback:
 * @provider: A #NavigationProvider
 * @location: A #NavigationLocation that should be freed after use
 * @userdata: The userdata passed into #navigation_provider_location_to_address
 *
 * Type of the callback function for #navigation_provider_address_to_location
 * which is called whenever @provider has returned a location.
 *
 * Note: Make sure you free location after use.
 */
typedef void (* NavigationProviderAddressToLocationCallback) (NavigationProvider *provider, 
                                                              NavigationLocation *location, 
				                              gpointer           userdata);

/**
 * navigation_provider_address_to_location:
 * @provider: A #NavigationProvider object
 * @address: A #NavigationAddress object
 * @cb: A #NavigationProviderAddressToLocationCallback
 * @userdata: The data to be passed to @cb
 *
 * Uses @provider to convert @address into a #NavigationLocation
 *
 * Return value: TRUE on success, FALSE otherwise.
 */
gboolean
navigation_provider_address_to_location (NavigationProvider      *provider,
		                         const NavigationAddress *address,
                                         NavigationProviderAddressToLocationCallback cb,
                                         gpointer                userdata,
			                 GError                  **error);

/**
 * NavigationProviderAddressToLocationVerboseCallback:
 * @provider: A #NavigationProvider
 * @location: A #NavigationLocation that should be freed after use
 * @error: A possible error that should be freed after use
 * @userdata: The userdata passed into #navigation_provider_location_to_address
 *
 * Type of the callback function for #navigation_provider_address_to_location_verbose
 * which is called whenever @provider has returned a location or an error.
 *
 * Note: Make sure you free location after use.
 */
typedef void (* NavigationProviderAddressToLocationVerboseCallback) (NavigationProvider *provider,
                                                                     NavigationLocation *location,
                                                                     GError             *error,
				                                                     gpointer           userdata);

/**
 * navigation_provider_address_to_location_verbose:
 * @provider: A #NavigationProvider object
 * @address: A #NavigationAddress object
 * @cb: A #NavigationProviderAddressToLocationVerboseCallback
 * @userdata: The data to be passed to @cb
 *
 * Uses @provider to convert @address into a #NavigationLocation. A verbose version that will
 * give error if user canceled operation (example closed network opening dialog).
 *
 * Return value: TRUE on success, FALSE otherwise.
 */
gboolean
navigation_provider_address_to_location_verbose (NavigationProvider      *provider,
		                         const NavigationAddress *address,
                                         NavigationProviderAddressToLocationVerboseCallback cb,
                                         gpointer                userdata,
			                 GError                  **error);

/**
 * navigation_provider_show_region:
 * @provider: A #NavigationProvider
 * @area: Area to show
 * @map_options: TODO check possible options
 * @error: A #GError to return errors
 *
 * Requests @provider to display a map of the region bounded by @area.
 *
 * Return value: TRUE on success, FALSE otherwise
 */
gboolean navigation_provider_show_region (NavigationProvider   *provider,
					  const NavigationArea *area,
					  unsigned int          map_options,
					  GError              **error);

/**
 * navigation_provider_show_places:
 * @provider: A #NavigationProvider
 * @places: An array of unique identifiers representing ToposPlace-objects
 * @map_options: TODO check possible options
 * @error: A #GError for reporting errors
 *
 * Requests that @provider should display the #ToposPlace-objects represented by
 * the unique IDs in @places.
 *
 * Return Value: TRUE on success, FALSE otherwise
 */
gboolean navigation_provider_show_places (NavigationProvider *provider, 
				          const char        **places,
				          unsigned int        map_options,
				          GError            **error);

/**
 * navigation_provider_show_location:
 * @provider: A #NavigationProvider
 * @location: A #NavigationLocation
 * @map_options: TODO check possible options
 * @error: A #GError for reporting errors
 *
 * Requests that @provider should display the location defined in @location.
 *
 * Return Value: TRUE on success, FALSE otherwise
 */
gboolean navigation_provider_show_location (NavigationProvider *provider,
					    NavigationLocation *location,
					    unsigned int        map_options,
					    GError            **error);

/**
 * navigation_provider_show_poi_categories:
 * @provider: A #NavigationProvider
 * @categories: An array of unique identifiers representing #ToposCategory-objects
 * @map_options: TODO check possible options
 * @error: A #GError for reporting errors
 *
 * Requests that @provider should display the #ToposPlace-objects, which belong to
 * POI categories in @categories. @categories specifies a set of #ToposCategory-objects.
 *
 * Return Value: TRUE on success, FALSE otherwise
 */
gboolean navigation_provider_show_poi_categories (NavigationProvider *provider,
						  const char        **categories,
						  unsigned int        map_options,
						  GError            **error);

/**
 * NavigationProviderGetPOICategoriesCallback:
 * @provider: A #NavigationProvider
 * @categories: An array of different point of interest categories
 * @userdata: The userdata passed into #navigation_provider_get_poi_categories
 *
 * Type of the callback function for #navigation_provider_get_poi_categories
 * which is called whenever @provider has returned categories.
 *
 * Note: Make sure you free categories after use.
 */
typedef void (* NavigationProviderGetPOICategoriesCallback) (NavigationProvider *provider, 
                                                             char              **categories, 
				                             gpointer            userdata);

/**
 * navigation_provider_get_poi_categories:
 * @provider: A #NavigationProvider
 * @cb: A #NavigationProviderGetPOICategoriesCallback
 * @userdata: The data to be passed to @cb
 * @error: A #GError for reporting errors
 *
 * Requests that @provider should return a list of all the POI categories that
 * @provider is aware of. Typically the map vendor delivers this information.
 * Thus the result not necessarily matches POI categories stored in Topos.
 *
 * Return value: TRUE on success, FALSE otherwise.
 */
gboolean navigation_provider_get_poi_categories (NavigationProvider *provider,
                                                 NavigationProviderGetPOICategoriesCallback cb,
                                                 gpointer                userdata,
			                         GError                  **error);

/**
 * navigation_provider_show_route:
 * @provider: A #NavigationProvider
 * @from: A #NavigationLocation representing the start of the route
 * @to: A #NavigationLocation representing the end of the route
 * @route_options: TODO check possible options
 * @map_options: TODO check possible options
 * @error: A #GError for reporting errors
 *
 * Requests that @provider should display the route starting at @from and ending
 * at @to.
 *
 * Return value: TRUE on success, FALSE otherwise
 */
gboolean navigation_provider_show_route (NavigationProvider *provider, 
					 NavigationLocation *from, 
					 NavigationLocation *to,
					 unsigned int        route_options,
					 unsigned int        map_options,
					 GError            **error);

/**
 * NavigationProviderGetLocationCallback:
 * @provider: A #NavigationProvider
 * @location: A #NavigationLocation that should be freed after use
 * @userdata: The userdata passed into #navigation_provider_get_location_from_map
 *
 * Type of the callback function for #navigation_provider_get_location_from_map
 * which is called whenever @provider has returned a location.
 *
 * Note: Make sure you free location after use.
 */
typedef void (* NavigationProviderGetLocationCallback) (NavigationProvider *provider, 
							NavigationLocation *location, 
							gpointer            userdata);

/**
 * navigation_provider_get_location_from_map:
 * @provider: A #NavigationProvider
 * @map_options: TODO check possible options
 * @cb: A #NavigationGetLocationCallback
 * @userdata: The data to be passed to @cb
 * @error: A #GError for reporting errors
 *
 * Requests that @provider opens the map and reports the location the user
 * selected. @cb will be called when the user has selected somewhere
 *
 * Return value: TRUE on success, FALSE otherwise.
 */
gboolean
navigation_provider_get_location_from_map (NavigationProvider *provider, 
					   unsigned int        map_options,
					   NavigationProviderGetLocationCallback cb, 
					   gpointer            userdata,
					   GError            **error);

/**
 * NavigationProviderGetPixbufCallback:
 * @provider: A #NavigationProvider
 * @pixbuf: A #GdkPixbuf
 * @area: Area of returned pixbuf
 * @userdata: The userdata passed into #navigation_provider_request_pixbuf_from_map
 *
 * Type of the callback function for #navigation_provider_request_pixbuf_from_map
 * which is called whenever @provider has returned a pixbuf.
 *
 * Note: Make sure you remove reference to pixbuf after use.
 */
typedef void (* NavigationProviderGetPixbufCallback) (NavigationProvider  *provider,
						      GdkPixbuf           *pixbuf,
                                                      NavigationArea      *area,
						      gpointer             userdata);

/**
 * navigation_provider_request_pixbuf_from_map:
 * @provider: A #NavigationProvider
 * @location: Location to render
 * @zoom: Zoom level, usually something between 0 and 18
 * @map_width: Requested map bitmap width
 * @map_height: Requested map bitmap height
 * @map_options: TODO check possible options
 * @cb: A #NavigationGetPixbufCallback
 * @userdata: The data to be passed to @cb
 * @error: A #GError for reporting errors
 *
 * Requests an area defined by @location and @zoom to be generated by @provider.
 * @cb will be called when the pixbuf is ready with @userdata.
 *
 * Return value: TRUE on success, FALSE otherwise.
 */
gboolean
navigation_provider_request_pixbuf_from_map (NavigationProvider       *provider,
					     const NavigationLocation *location,
					     int                       zoom,
                                             int                       map_width,
                                             int                       map_height,
					     unsigned int              map_options,
					     NavigationProviderGetPixbufCallback cb,
					     gpointer                  userdata,
					     GError                  **error);

G_END_DECLS

#endif
