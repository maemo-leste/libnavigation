/*
 * navigation-provider.c
 *
 * Copyright (C) 2022 Ivaylo Dimitrov <ivo.g.dimitrov.75@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <https://www.gnu.org/licenses/>.
 *
 */

/**
 * SECTION:navigation-provider
 * @short_description: a representation of an navigation provider.
 *
 * An #NavigationProvider is an object which represents a navigation provider.
 */

#include "config.h"

#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>
#include <gconf/gconf-client.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <libxml/xmlreader.h>

#include "navigation-provider-client-glue.h"

#include "navigation-provider.h"

#define ISO_CODES_DIR "/share/xml/iso-codes"
#define ISO_3166_XML_PATH ISO_CODES_PREFIX ISO_CODES_DIR "/iso_3166.xml"

struct _NavigationProviderPrivate
{
  gchar *service;
  DBusGConnection *gdbus;
  DBusGProxy *proxy;
  DBusConnection *dbus;
  GHashTable *requests;
};

typedef struct _NavigationProviderPrivate NavigationProviderPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(
  NavigationProvider,
  navigation_provider,
  G_TYPE_OBJECT
)

#define PRIVATE(provider) \
  ((NavigationProviderPrivate *) \
   navigation_provider_get_instance_private( \
     (NavigationProvider *)(provider)))

struct _NavigationProviderRequest
{
  GCallback cb;
  gboolean verbose;
  gpointer user_data;
};

typedef struct _NavigationProviderRequest NavigationProviderRequest;

static GHashTable *a3_2_country = NULL;

static void
process_iso_3166_node(xmlTextReaderPtr reader, GHashTable *table)
{
  const xmlChar *name = xmlTextReaderConstName(reader);
  gchar *country = NULL;
  gchar *alpha_3_code = NULL;

  if (!name || xmlStrcmp(name, BAD_CAST "iso_3166_entry"))
    return;

  while (xmlTextReaderMoveToNextAttribute(reader) == 1)
  {
    if (!xmlStrcmp(xmlTextReaderConstName(reader), BAD_CAST "alpha_3_code"))
      alpha_3_code = g_strdup((const gchar *)xmlTextReaderConstValue(reader));
    else if (!xmlStrcmp(xmlTextReaderConstName(reader), BAD_CAST "name"))
      country = g_strdup((const gchar *)xmlTextReaderConstValue(reader));

    if (country && alpha_3_code)
    {
      g_hash_table_insert(table, alpha_3_code, country);
      break;
    }
  }
}

static gboolean
parse_iso_3166(GHashTable *table)
{
  xmlTextReaderPtr reader;
  int ret;

  reader = xmlReaderForFile(ISO_3166_XML_PATH, NULL, 0);

  g_return_val_if_fail(reader != NULL, FALSE);

  while ((ret = xmlTextReaderRead(reader)) == 1)
    process_iso_3166_node(reader, table);

  xmlFreeTextReader(reader);

  g_return_val_if_fail(ret == 0, FALSE);

  return TRUE;
}

static void __attribute__((constructor))
create_country_table()
{
  a3_2_country = g_hash_table_new_full((GHashFunc)&g_str_hash,
                                       (GEqualFunc)&g_str_equal,
                                       (GDestroyNotify)&g_free,
                                       (GDestroyNotify)&g_free);

  if (!parse_iso_3166(a3_2_country))
    g_warning("Loading iso3166 failed.");
}

static void __attribute__((destructor))
destroy_country_table()
{
  g_hash_table_destroy(a3_2_country);
}

static gchar *
_navigation_country_code_to_country(const gchar *alpha_3_code)
{
  return g_strdup(g_hash_table_lookup(a3_2_country, alpha_3_code));
}

static void
check_country(NavigationAddress *address)
{
  if (address)
  {
    if (!address->country || !g_strcmp0(address->country, ""))
    {
      if (address->country_code && g_strcmp0(address->country_code, ""))
      {
        address->country = _navigation_country_code_to_country(
            address->country_code);
      }
    }
  }
}

GQuark
navigation_error_quark()
{
  static GQuark quark = 0;

  if (!quark)
    quark = g_quark_from_static_string("navigation-error-quark");

  return quark;
}

static void
array_to_address(NavigationAddress **address, int idx, const gchar *val)
{
  if (!val || *val)
  {
    switch (idx)
    {
      case 0:
      {
        (*address)->house_num = g_strdup(val);
        break;
      }
      case 1:
      {
        (*address)->house_name = g_strdup(val);
        break;
      }
      case 2:
      {
        (*address)->street = g_strdup(val);
        break;
      }
      case 3:
      {
        (*address)->suburb = g_strdup(val);
        break;
      }
      case 4:
      {
        (*address)->town = g_strdup(val);
        break;
      }
      case 5:
      {
        (*address)->municipality = g_strdup(val);
        break;
      }
      case 6:
      {
        (*address)->province = g_strdup(val);
        break;
      }
      case 7:
      {
        (*address)->postal_code = g_strdup(val);
        break;
      }
      case 8:
      {
        (*address)->country = g_strdup(val);
        break;
      }
      case 9:
      {
        (*address)->country_code = g_strdup(val);
        break;
      }
      case 10:
      {
        (*address)->time_zone = g_strdup(val);
        break;
      }
    }
  }
}

static NavigationLocation *
get_location(DBusMessageIter *iter)
{
  NavigationLocation *location = g_new0(NavigationLocation, 1);
  DBusMessageIter sub;

  dbus_message_iter_recurse(iter, &sub);

  if (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_DOUBLE)
    dbus_message_iter_get_basic(&sub, &location->latitude);

  dbus_message_iter_next(&sub);

  if (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_DOUBLE)
    dbus_message_iter_get_basic(&sub, &location->longitude);

  return location;
}

static DBusHandlerResult
navigation_provider_dbus_filter(DBusConnection *connection,
                                DBusMessage *message,
                                gpointer user_data)
{
  NavigationProvider *provider = user_data;
  NavigationProviderPrivate *priv = PRIVATE(provider);

  const char *path;
  DBusMessageIter sub1;
  DBusMessageIter sub2;
  DBusMessageIter iter;

  path = dbus_message_get_path(message);

  if (path)
  {
    NavigationProviderRequest *request;

    request = g_hash_table_lookup(priv->requests, path);

    if (request)
    {
      g_object_ref(provider);

      if (request->cb)
      {
        if (dbus_message_is_signal(message,
                                   "com.nokia.Navigation.MapProvider",
                                   "LocationToAddressReply"))
        {
          NavigationAddress *address = NULL;

          dbus_message_iter_init(message, &iter);

          if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY)
          {
            dbus_message_iter_recurse(&iter, &sub1);

            if (dbus_message_iter_get_arg_type(&sub1) == DBUS_TYPE_ARRAY)
            {
              int i = 0;

              address = g_new0(NavigationAddress, 1);
              dbus_message_iter_recurse(&sub1, &sub2);

              while (dbus_message_iter_get_arg_type(&sub2) != DBUS_TYPE_INVALID)
              {
                const gchar *v;

                dbus_message_iter_get_basic(&sub2, &v);
                array_to_address(&address, i, v);
                dbus_message_iter_next(&sub2);
                i++;
              }
            }
          }

          check_country(address);

          if (request->verbose)
          {
            ((NavigationProviderLocationToAddressCallback)request->cb)(
              provider, address, request->user_data);
          }
          else
          {
            ((NavigationProviderLocationToAddressVerboseCallback)request->cb)(
              provider, address, NULL, request->user_data);
          }
        }
        else if (dbus_message_is_signal(message,
                                        "com.nokia.Navigation.MapProvider",
                                        "LocationToAddressError"))
        {
          ((NavigationProviderLocationToAddressVerboseCallback)request->cb)
            (provider, NULL,
            g_error_new(NAVIGATION_ERROR,
                        NAVIGATION_ERROR_USER_CANCELED_OPERATION,
                        "User canceled operation"),
            request->user_data);
        }
        else if (dbus_message_is_signal(message,
                                        "com.nokia.Navigation.MapProvider",
                                        "AddressToLocationError"))
        {
          ((NavigationProviderAddressToLocationVerboseCallback)request->cb)
            (provider, NULL,
            g_error_new(NAVIGATION_ERROR,
                        NAVIGATION_ERROR_USER_CANCELED_OPERATION,
                        "User canceled operation"),
            request->user_data);
        }
        else if (dbus_message_is_signal(message,
                                        "com.nokia.Navigation.MapProvider",
                                        "AddressToLocationsReply"))
        {
          NavigationLocation *location = NULL;

          dbus_message_iter_init(message, &sub2);

          if (dbus_message_iter_get_arg_type(&sub2))
          {
            dbus_message_iter_recurse(&sub2, &sub1);

            if (dbus_message_iter_get_arg_type(&sub1))
              location = get_location(&sub1);
          }

          if (request->verbose)
          {
            ((NavigationProviderAddressToLocationVerboseCallback)request->cb)(
              provider, location, NULL, request->user_data);
          }
          else
          {
            ((NavigationProviderAddressToLocationCallback)request->cb)(
              provider, location, request->user_data);
          }
        }
        else if (dbus_message_is_signal(message,
                                        "com.nokia.Navigation.MapProvider",
                                        "GetMapTileReply"))
        {
          NavigationArea *area = NULL;
          GdkPixbuf *pixbuf;

          dbus_message_iter_init(message, &sub1);

          if ((dbus_message_iter_get_arg_type(&sub1) == DBUS_TYPE_ARRAY) &&
              (dbus_message_iter_get_element_type(&sub1) == DBUS_TYPE_BYTE))
          {
            int n_elements;
            guint8 *stream;
            GdkPixdata pixdata;

            dbus_message_iter_recurse(&sub1, &sub2);
            dbus_message_iter_get_fixed_array(&sub2, &stream, &n_elements);
            G_GNUC_BEGIN_IGNORE_DEPRECATIONS
            gdk_pixdata_deserialize(&pixdata, n_elements, stream, NULL);

            pixbuf = gdk_pixbuf_from_pixdata(&pixdata, TRUE, NULL);
            G_GNUC_END_IGNORE_DEPRECATIONS

            if (pixbuf)
            {
              NavigationArea *area = g_new(NavigationArea, 1);

              dbus_message_iter_next(&sub1);

              if (dbus_message_iter_get_arg_type(&sub1) == DBUS_TYPE_STRUCT)
              {
                NavigationLocation *location = get_location(&sub1);
                area->nw.longitude = location->longitude;
                area->nw.latitude = location->latitude;
                navigation_location_free(location);
              }

              dbus_message_iter_next(&sub1);

              if (dbus_message_iter_get_arg_type(&sub1) == DBUS_TYPE_STRUCT)
              {
                NavigationLocation *location = get_location(&sub1);

                area->se.longitude = location->longitude;
                area->se.latitude = location->latitude;

                navigation_location_free(location);
              }
            }
          }

          ((NavigationProviderGetPixbufCallback)request->cb)(
            provider, pixbuf, area, request->user_data);
        }

#if 0
        else if (dbus_message_is_signal(message,
                                        "com.nokia.Navigation.MapProvider",
                                        "CoordinateReply"))
        {
          NavigationLocation *location = g_new(NavigationLocation, 1);

          if (!dbus_message_get_args(message, NULL,
                                     DBUS_TYPE_DOUBLE, &location->latitude,
                                     DBUS_TYPE_DOUBLE, &location->longitude,
                                     DBUS_TYPE_INVALID))
          {
            g_warning("Could not parse get location from map response signal");
            navigation_location_free(location);
            location = NULL;
          }

          request->cb(provider, location, request->user_data);
        }
#endif
        else if (dbus_message_is_signal(message,
                                        "com.nokia.Navigation.MapProvider",
                                        "GetPOICategoriesReply"))
        {
          char **categories = NULL;

          dbus_message_iter_init(message, &sub2);

          if (dbus_message_iter_get_arg_type(&sub2) != DBUS_TYPE_INVALID)
          {
            GPtrArray *array = g_ptr_array_new();

            dbus_message_iter_recurse(&sub2, &sub1);

            while (dbus_message_iter_get_arg_type(&sub1) == DBUS_TYPE_STRING)
            {
              const gchar *v;

              dbus_message_iter_get_basic(&sub1, &v);
              g_ptr_array_add(array, g_strdup(v));
              dbus_message_iter_next(&sub1);
            }

            g_ptr_array_add(array, NULL);
            categories = (char **)g_ptr_array_free(array, FALSE);
          }

          ((NavigationProviderGetPOICategoriesCallback)request->cb)(
            provider, categories, request->user_data);
        }
        else
          g_warning("Unknown reply recieved");
      }

      g_hash_table_remove(priv->requests, path);
      g_object_unref(provider);
      return DBUS_HANDLER_RESULT_HANDLED;
    }
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
navigation_provider_dispose(GObject *object)
{
  NavigationProviderPrivate *priv = PRIVATE(object);

  if (priv->requests)
  {
    g_hash_table_destroy(priv->requests);
    priv->requests = NULL;
  }

  if (priv->dbus)
  {
    dbus_bus_remove_match(
      priv->dbus,
      "type='signal',interface='com.nokia.Navigation.MapProvider'", NULL);
    dbus_connection_remove_filter(priv->dbus, navigation_provider_dbus_filter,
                                  object);
  }

  if (priv->gdbus)
  {
    dbus_g_connection_unref(priv->gdbus);
    priv->gdbus = NULL;
  }

  G_OBJECT_CLASS(navigation_provider_parent_class)->dispose(object);
}

static void
navigation_provider_finalize(GObject *object)
{
  g_free(PRIVATE(object)->service);

  G_OBJECT_CLASS(navigation_provider_parent_class)->finalize(object);
}

static void
navigation_provider_class_init(NavigationProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->dispose = navigation_provider_dispose;
  object_class->finalize = navigation_provider_finalize;
}

static void
navigation_provider_init(NavigationProvider *provider)
{
  NavigationProviderPrivate *priv = PRIVATE(provider);

  priv->requests = g_hash_table_new_full((GHashFunc)&g_str_hash,
                                         (GEqualFunc)&g_str_equal,
                                         (GDestroyNotify)&g_free,
                                         (GDestroyNotify)&g_free);
}

NavigationProvider *
navigation_provider_new_default()
{
  return g_object_new(NAVIGATION_TYPE_PROVIDER, NULL);
}

void
navigation_location_free(NavigationLocation *location)
{
  g_free(location);
}

void
navigation_address_free(NavigationAddress *address)
{
  if (address)
  {
    g_free(address->house_num);
    g_free(address->house_name);
    g_free(address->street);
    g_free(address->suburb);
    g_free(address->town);
    g_free(address->municipality);
    g_free(address->province);
    g_free(address->postal_code);
    g_free(address->country);
    g_free(address->country_code);
    g_free(address->time_zone);
    g_free(address);
  }
}

GList *
navigation_provider_list_all()
{
  GDir *dir = g_dir_open("/usr/share/osso-navigation-providers/", 0, NULL);
  const gchar *dir_name;
  GList *list = NULL;

  if (!dir)
    return NULL;

  while ((dir_name = g_dir_read_name(dir)))
  {
    GKeyFile *key_file = g_key_file_new();
    GError *error = NULL;
    gchar *file = g_build_filename("/usr/share/osso-navigation-providers/",
                                   dir_name, NULL);

    if (g_key_file_load_from_file(key_file, file, G_KEY_FILE_NONE, &error))
    {
      NavigationProviderDetails *details = g_new0(NavigationProviderDetails, 1);

      details->name = g_key_file_get_value(
          key_file, "Map service", "Name", NULL);
      details->service = g_key_file_get_value(
          key_file, "Map service", "Service", NULL);
      details->description = g_key_file_get_value(
          key_file, "Map service", "Description", NULL);
      list = g_list_prepend(list, details);
    }
    else
    {
      g_warning("Error loading: %s - %s", file, error->message);
      g_error_free(error);
    }

    g_key_file_free(key_file);
    g_free(file);
  }

  g_dir_close(dir);

  return list;
}

void
navigation_provider_list_free(GList *list)
{
  GList *l;

  for (l = list; l; l = l->next)
  {
    NavigationProviderDetails *details = l->data;

    g_free(details->name);
    g_free(details->service);
    g_free(details->description);
    g_free(details);
  }

  g_list_free(list);
}

static gchar *
get_safe_string(const gchar *s)
{
  if (s)
    return g_strdup(s);
  else
    return g_strdup("");
}

gchar **
address_to_array(const NavigationAddress *address)
{
  gchar **array = g_new(gchar *, 16);

  array[0] = get_safe_string(address->house_num);
  array[1] = get_safe_string(address->house_name);
  array[2] = get_safe_string(address->street);
  array[3] = get_safe_string(address->suburb);
  array[4] = get_safe_string(address->town);
  array[5] = get_safe_string(address->municipality);
  array[6] = get_safe_string(address->province);
  array[7] = get_safe_string(address->postal_code);
  array[8] = get_safe_string(address->country);
  array[9] = get_safe_string(address->country_code);
  array[10] = get_safe_string(address->time_zone);
  array[11] = get_safe_string(NULL);
  array[12] = get_safe_string(NULL);
  array[13] = get_safe_string(NULL);
  array[14] = get_safe_string(NULL);
  array[15] = NULL;

  return array;
}

NavigationAddress *
navigation_address_copy(NavigationAddress *address)
{
  NavigationAddress *copy;

  if (!address)
    return NULL;

  copy = g_memdup(address, sizeof(NavigationAddress));

  copy->house_num = g_strdup(address->house_num);
  copy->house_name = g_strdup(address->house_name);
  copy->street = g_strdup(address->street);
  copy->suburb = g_strdup(address->suburb);
  copy->town = g_strdup(address->town);
  copy->municipality = g_strdup(address->municipality);
  copy->province = g_strdup(address->province);
  copy->postal_code = g_strdup(address->postal_code);
  copy->country = g_strdup(address->country);
  copy->country_code = g_strdup(address->country_code);
  copy->time_zone = g_strdup(address->time_zone);

  return copy;
}

void
navigation_make_resident()
{
  GModule *module = g_module_open(LIBDIR "/libnavigation.so.0", 0);

  if (!module)
  {
    g_critical("%s: g_module_open() failed: %s", __FUNCTION__,
               g_module_error());
  }
  else
    g_module_make_resident(module);
}

gchar *
navigation_provider_get_default_service()
{
  GConfClient *gconf = gconf_client_get_default();
  GError *error = NULL;
  gchar *service;

  service = gconf_client_get_string(gconf, "/apps/osso/navigation/service",
                                    &error);

  if (error)
  {
    g_assert( service == NULL);
    g_warning("service is not set: %s", error->message);
    g_error_free(error);
  }

  g_object_unref(gconf);

  return service;
}

void
navigation_provider_set_default_service(const char *service)
{
  gconf_client_set_string(gconf_client_get_default(),
                          "/apps/osso/navigation/service", service, NULL);
}

static int
navigation_provider_service_init(NavigationProvider *provider, GError **error)
{
  NavigationProviderPrivate *priv = PRIVATE(provider);

  if (priv->proxy)
    return TRUE;

  if (!priv->service)
    priv->service = navigation_provider_get_default_service();

  if (!priv->service)
  {
    g_set_error(error, NAVIGATION_ERROR, NAVIGATION_ERROR_TOO_MANY_REQUESTS,
                "Failed to identify the default service");
    return FALSE;
  }

  priv->gdbus = dbus_g_bus_get(DBUS_BUS_SESSION, error);

  if (!priv->gdbus)
    return FALSE;

  priv->dbus = dbus_g_connection_get_connection(priv->gdbus);

  dbus_connection_add_filter(priv->dbus, navigation_provider_dbus_filter,
                             provider, NULL);
  dbus_bus_add_match(
    priv->dbus,
    "type='signal',interface='com.nokia.Navigation.MapProvider'", NULL);
  priv->proxy = dbus_g_proxy_new_for_name(priv->gdbus, priv->service,
                                          "/Provider",
                                          "com.nokia.Navigation.MapProvider");

  return TRUE;
}

gboolean
navigation_provider_show_route(NavigationProvider *provider,
                               NavigationLocation *from, NavigationLocation *to,
                               unsigned int route_options,
                               unsigned int map_options, GError **error)
{
  NavigationProviderPrivate *priv;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (!navigation_provider_service_init(provider, error))
    return FALSE;

  return com_nokia_Navigation_MapProvider_show_route(
           priv->proxy, from->latitude, from->longitude,
           to->latitude, to->longitude, route_options, map_options, error);
}

gboolean
navigation_provider_show_poi_categories(NavigationProvider *provider,
                                        const char **categories,
                                        unsigned int map_options,
                                        GError **error)
{
  NavigationProviderPrivate *priv;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (!navigation_provider_service_init(provider, error))
    return FALSE;

  return com_nokia_Navigation_MapProvider_show_places_po_icategories(
           priv->proxy, categories, map_options, error);
}

gboolean
navigation_provider_show_location(NavigationProvider *provider,
                                  NavigationLocation *location,
                                  unsigned int map_options, GError **error)
{
  NavigationProviderPrivate *priv;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (!navigation_provider_service_init(provider, error))
    return FALSE;

  return com_nokia_Navigation_MapProvider_show_place_geo(
           priv->proxy, location->latitude, location->longitude, map_options,
           error);
}

gboolean
navigation_provider_show_places(NavigationProvider *provider,
                                const char **places, unsigned int map_options,
                                GError **error)
{
  NavigationProviderPrivate *priv;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (!navigation_provider_service_init(provider, error))
    return FALSE;

  return com_nokia_Navigation_MapProvider_show_places_topos(
           priv->proxy, places, map_options, error);
}

gboolean
navigation_provider_show_region(NavigationProvider *provider,
                                const NavigationArea *area,
                                unsigned int map_options, GError **error)
{
  NavigationProviderPrivate *priv;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (!navigation_provider_service_init(provider, error))
    return FALSE;

  return com_nokia_Navigation_MapProvider_show_region(
           priv->proxy, area->nw.latitude, area->nw.longitude,
           area->se.latitude, area->se.longitude, map_options,
           error);
}

static gboolean
check_too_many_requests(NavigationProviderPrivate *priv, GError **error)
{
  if (g_hash_table_size(priv->requests) > 5)
  {
    g_set_error(error, NAVIGATION_ERROR, NAVIGATION_ERROR_TOO_MANY_REQUESTS,
                "Libnavigation buffer for pending requests is full");
    return TRUE;
  }

  return FALSE;
}

/* *INDENT-OFF* */
gboolean
navigation_provider_request_pixbuf_from_map(
  NavigationProvider *provider, const NavigationLocation *location, int zoom,
  int map_width, int map_height, unsigned int map_options,
  NavigationProviderGetPixbufCallback cb, gpointer userdata, GError **error)
/* *INDENT-ON* */
{
  NavigationProviderPrivate *priv;
  char *object_path = NULL;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (check_too_many_requests(priv, error))
    return FALSE;

  if (navigation_provider_service_init(provider, error) &&
      com_nokia_Navigation_MapProvider_get_map_tile(
        priv->proxy, location->latitude, location->longitude, zoom,
        map_width, map_height, map_options, &object_path, error))
  {
    NavigationProviderRequest *request = g_new0(NavigationProviderRequest, 1);

    request->cb = (GCallback)cb;
    request->user_data = userdata;
    g_hash_table_insert(priv->requests, object_path, request);
    return TRUE;
  }

  return FALSE;
}

/* *INDENT-OFF* */
gboolean
navigation_provider_get_location_from_map(
  NavigationProvider *provider, unsigned int map_options,
  NavigationProviderGetLocationCallback cb, gpointer userdata, GError **error)
/* *INDENT-ON* */
{
  NavigationProviderPrivate *priv;
  char *object_path = NULL;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (check_too_many_requests(priv, error))
    return FALSE;

  if (navigation_provider_service_init(provider, error) &&
      com_nokia_Navigation_MapProvider_get_location_from_map(
        priv->proxy, map_options, &object_path, error))
  {
    NavigationProviderRequest *request = g_new0(NavigationProviderRequest, 1);

    request->cb = (GCallback)cb;
    request->user_data = userdata;
    g_hash_table_insert(priv->requests, object_path, request);
    return TRUE;
  }

  return FALSE;
}

/* *INDENT-OFF* */
gboolean
navigation_provider_get_poi_categories(
  NavigationProvider *provider, NavigationProviderGetPOICategoriesCallback cb,
  gpointer userdata, GError **error)
/* *INDENT-ON* */
{
  NavigationProviderPrivate *priv;
  char *object_path = NULL;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (check_too_many_requests(priv, error))
    return FALSE;

  if (navigation_provider_service_init(provider, error) &&
      com_nokia_Navigation_MapProvider_get_po_icategories(
        priv->proxy, &object_path, error))
  {
    NavigationProviderRequest *request = g_new0(NavigationProviderRequest, 1);

    request->cb = (GCallback)cb;
    request->user_data = userdata;
    g_hash_table_insert(priv->requests, object_path, request);
    return TRUE;
  }

  return FALSE;
}

/* *INDENT-OFF* */
gboolean
navigation_provider_address_to_location(
  NavigationProvider *provider, const NavigationAddress *address,
  NavigationProviderAddressToLocationCallback cb, gpointer userdata,
  GError **error)
/* *INDENT-ON* */

{
  NavigationProviderPrivate *priv;
  gchar **array;
  char *object_path = NULL;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (check_too_many_requests(priv, error))
    return FALSE;

  if (!navigation_provider_service_init(provider, error))
    return FALSE;

  array = address_to_array(address);

  if (com_nokia_Navigation_MapProvider_address_to_locations(
        priv->proxy, (const char **)array, FALSE, &object_path, error))
  {
    NavigationProviderRequest *request = g_new0(NavigationProviderRequest, 1);

    request->cb = (GCallback)cb;
    request->user_data = userdata;
    g_hash_table_insert(priv->requests, object_path, request);
    g_strfreev(array);

    return TRUE;
  }
  else
  {
    g_warning("Address to locations failed in provider");
    g_strfreev(array);
  }

  return FALSE;
}

/* *INDENT-OFF* */
gboolean
navigation_provider_location_to_address(
    NavigationProvider *provider, NavigationLocation *location,
    NavigationProviderLocationToAddressCallback cb, gpointer userdata,
    GError **error)
/* *INDENT-ON* */
{
  NavigationProviderPrivate *priv;
  char *object_path = NULL;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (check_too_many_requests(priv, error))
    return FALSE;

  if (!navigation_provider_service_init(provider, error))
    return FALSE;

  if (!com_nokia_Navigation_MapProvider_location_to_addresses(
        priv->proxy, location->latitude, location->longitude, FALSE,
        &object_path, error))
  {
    NavigationProviderRequest *request = g_new0(NavigationProviderRequest, 1);

    request->cb = (GCallback)cb;
    request->user_data = userdata;
    g_hash_table_insert(priv->requests, object_path, request);

    return TRUE;
  }
  else
    g_warning("Location to address failed in provider");

  return FALSE;
}

/* *INDENT-OFF* */
gboolean
navigation_provider_location_to_address_verbose(
    NavigationProvider *provider, NavigationLocation *location,
    NavigationProviderLocationToAddressVerboseCallback cb, gpointer userdata,
    GError **error)
/* *INDENT-ON* */
{
  NavigationProviderPrivate *priv;
  char *object_path = NULL;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (check_too_many_requests(priv, error))
    return FALSE;

  if (!navigation_provider_service_init(provider, error))
    return FALSE;

  if (com_nokia_Navigation_MapProvider_location_to_addresses(
        priv->proxy, location->latitude, location->longitude, TRUE,
        &object_path, error))
  {
    NavigationProviderRequest *request = g_new0(NavigationProviderRequest, 1);

    request->cb = (GCallback)cb;
    request->verbose = TRUE;
    request->user_data = userdata;
    g_hash_table_insert(priv->requests, object_path, request);

    return TRUE;
  }
  else
    g_warning("Location to address failed in provider");

  return FALSE;
}

/* *INDENT-OFF* */
gboolean
navigation_provider_address_to_location_verbose(
    NavigationProvider *provider, const NavigationAddress *address,
    NavigationProviderAddressToLocationVerboseCallback cb, gpointer userdata,
    GError **error)
/* *INDENT-ON* */
{
  NavigationProviderPrivate *priv;
  gchar **array;
  char *object_path = NULL;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (check_too_many_requests(priv, error))
    return FALSE;

  if (!navigation_provider_service_init(provider, error))
    return FALSE;

  array = address_to_array(address);

  if (com_nokia_Navigation_MapProvider_address_to_locations(
        priv->proxy, (const char **)array, TRUE, &object_path, error))
  {
    NavigationProviderRequest *request = g_new0(NavigationProviderRequest, 1);

    request->cb = (GCallback)cb;
    request->verbose = TRUE;
    request->user_data = userdata;
    g_hash_table_insert(priv->requests, object_path, request);
    g_strfreev(array);
    return TRUE;
  }
  else
  {
    g_warning("Address to locations failed in provider");
    g_strfreev(array);
  }

  return FALSE;
}

gboolean
navigation_provider_location_to_address_cached(NavigationProvider *provider,
                                               NavigationLocation *location,
                                               gdouble tolerance,
                                               NavigationAddress **address,
                                               GError **error)
{
  NavigationProviderPrivate *priv;
  GPtrArray *addresses;
  gchar **p;
  int i;

  g_return_val_if_fail(NAVIGATION_IS_PROVIDER(provider), FALSE);

  priv = PRIVATE(provider);

  if (!navigation_provider_service_init(provider, error))
    return FALSE;

  if (!com_nokia_Navigation_MapProvider_location_to_addresses_cached(
        priv->proxy, location->latitude, location->longitude, tolerance,
        &addresses, error))
  {
    return FALSE;
  }

  p = (gchar **)addresses->pdata;
  *address = g_new0(NavigationAddress, 1);

  while (*p && (i < 15))
  {
    array_to_address(address, i++, *p);
    g_free(*p++);
  }

  check_country(*address);
  g_ptr_array_free(addresses, TRUE);

  return 1;
}
