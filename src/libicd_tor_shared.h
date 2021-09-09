#ifndef __LIBICD_TOR_SHARED_H
#define __LIBICD_TOR_SHARED_H

#define TOR_NETWORK_TYPE "TOR"
#define TOR_PROVIDER_TYPE "TOR"
#define TOR_PROVIDER_NAME "Tor Provider"

#define TOR_DEFAULT_SERVICE_ATTRIBUTES 0
#define TOR_DEFAULT_SERVICE_PRIORITY 0

#define GC_TOR "/system/osso/connectivity/providers/tor"
#define GC_ICD_TOR_AVAILABLE_IDS "/system/osso/connectivity/srv_provider/TOR/available_ids"

#define GC_NETWORK_TYPE "/system/osso/connectivity/network_type/TOR"
#define GC_TOR_ACTIVE  GC_NETWORK_TYPE"/active_config"
#define GC_TOR_SYSTEM  GC_NETWORK_TYPE"/system_wide_enabled"

#define GC_SOCKSPORT       "socks-port"
#define GC_CONTROLPORT     "control-port"
#define GC_TRANSPORT       "trans-port"
#define GC_DNSPORT         "dns-port"
#define GC_DATADIR         "datadir"
#define GC_BRIDGES         "bridges"
#define GC_BRIDGESENABLED  "bridges-enabled"
#define GC_HIDDENSERVICES  "hiddenservices"
#define GC_HSENABLED       "hiddenservices-enabled"

#define ICD_TOR_DBUS_INTERFACE "org.maemo.Tor"
#define ICD_TOR_DBUS_PATH "/org/maemo/Tor"

#define ICD_TOR_SIGNALS_STATUS #ICD_TOR_DBUS_INTERFACE ".Status"

#define ICD_TOR_SIGNAL_STATUSCHANGED      "StatusChanged"
#define ICD_TOR_SIGNALS_STATUS_CONNECTED "Connected"
#define ICD_TOR_SIGNALS_STATUS_STARTED "Started"
#define ICD_TOR_SIGNALS_STATUS_STOPPED "Stopped"

#if 0
// TODO: make this an enum
#define TOR_DBUS_METHOD_START_RESULT_OK 0
#define TOR_DBUS_METHOD_START_RESULT_INVALID_CONFIG 1
#define TOR_DBUS_METHOD_START_RESULT_ALREADY_RUNNING 2
#define TOR_DBUS_METHOD_START_RESULT_FAILED_TO_START 3
#endif

#endif				/* __LIBICD_TOR_SHARED_H */
