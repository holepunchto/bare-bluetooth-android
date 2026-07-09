#include <assert.h>
#include <bare.h>
#include <jni.h>
#include <jnitl.h>
#include <js.h>
#include <jstl.h>
#include <utf.h>

#include <stdlib.h>
#include <uv.h>

#include <mutex>
#include <unordered_map>
#include <unordered_set>

// java.lang
using j_object_t = java_object_t<"java/lang/Object">;
using j_string_t = java_object_t<"java/lang/String">;

// java.util
using j_uuid_t = java_object_t<"java/util/UUID">;
using j_list_t = java_object_t<"java/util/List">;
using j_map_t = java_object_t<"java/util/Map">;
using j_map_entry_t = java_object_t<"java/util/Map$Entry">;
using j_set_t = java_object_t<"java/util/Set">;
using j_iterator_t = java_object_t<"java/util/Iterator">;

// java.io
using j_output_stream_t = java_object_t<"java/io/OutputStream">;

// android.app
using j_application_t = java_object_t<"android/app/Application">;

// android.content
using j_context_t = java_object_t<"android/content/Context">;

// android.os
using j_parcel_uuid_t = java_object_t<"android/os/ParcelUuid">;

// android.bluetooth
using j_bluetooth_adapter_t = java_object_t<"android/bluetooth/BluetoothAdapter">;
using j_bluetooth_device_t = java_object_t<"android/bluetooth/BluetoothDevice">;
using j_bluetooth_gatt_t = java_object_t<"android/bluetooth/BluetoothGatt">;
using j_bluetooth_gatt_callback_t = java_object_t<"android/bluetooth/BluetoothGattCallback">;
using j_bluetooth_gatt_characteristic_t = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">;
using j_bluetooth_gatt_descriptor_t = java_object_t<"android/bluetooth/BluetoothGattDescriptor">;
using j_bluetooth_gatt_server_t = java_object_t<"android/bluetooth/BluetoothGattServer">;
using j_bluetooth_gatt_server_callback_t = java_object_t<"android/bluetooth/BluetoothGattServerCallback">;
using j_bluetooth_gatt_service_t = java_object_t<"android/bluetooth/BluetoothGattService">;
using j_bluetooth_manager_t = java_object_t<"android/bluetooth/BluetoothManager">;
using j_bluetooth_server_socket_t = java_object_t<"android/bluetooth/BluetoothServerSocket">;
using j_bluetooth_socket_t = java_object_t<"android/bluetooth/BluetoothSocket">;

// android.bluetooth.le
using j_advertise_callback_t = java_object_t<"android/bluetooth/le/AdvertiseCallback">;
using j_advertise_settings_t = java_object_t<"android/bluetooth/le/AdvertiseSettings">;
using j_bluetooth_le_advertiser_t = java_object_t<"android/bluetooth/le/BluetoothLeAdvertiser">;
using j_bluetooth_le_scanner_t = java_object_t<"android/bluetooth/le/BluetoothLeScanner">;
using j_scan_callback_t = java_object_t<"android/bluetooth/le/ScanCallback">;
using j_scan_record_t = java_object_t<"android/bluetooth/le/ScanRecord">;
using j_scan_result_t = java_object_t<"android/bluetooth/le/ScanResult">;

// to.holepunch.bare.bluetooth
using j_hp_advertise_callback_t = java_object_t<"to/holepunch/bare/bluetooth/AdvertiseCallback">;
using j_hp_advertise_helper_t = java_object_t<"to/holepunch/bare/bluetooth/AdvertiseHelper">;
using j_hp_gatt_callback_t = java_object_t<"to/holepunch/bare/bluetooth/GattCallback">;
using j_hp_gatt_server_callback_t = java_object_t<"to/holepunch/bare/bluetooth/GattServerCallback">;
using j_hp_l2cap_acceptor_t = java_object_t<"to/holepunch/bare/bluetooth/L2capAcceptor">;
using j_hp_l2cap_connector_t = java_object_t<"to/holepunch/bare/bluetooth/L2capConnector">;
using j_hp_l2cap_reader_t = java_object_t<"to/holepunch/bare/bluetooth/L2capReader">;
using j_hp_scan_callback_t = java_object_t<"to/holepunch/bare/bluetooth/ScanCallback">;
using j_hp_scan_helper_t = java_object_t<"to/holepunch/bare/bluetooth/ScanHelper">;

static inline java_vm_t
bare_bluetooth_android_jvm() {
  return java_vm_t::get_created().value();
}

static inline java_class_loader_t
bare_bluetooth_android_get_class_loader(JNIEnv *env) {
  auto thread = java_thread_t::current_thread(env);
  return thread.get_context_class_loader();
}

static inline j_context_t
bare_bluetooth_android_get_context(JNIEnv *env) {
  auto cls = java_class_t<"android/app/ActivityThread">(env);
  auto current_app = cls.get_static_method<j_application_t()>("currentApplication");
  return j_context_t(env, current_app());
}

template <java_class_name_t N>
static inline std::string
bare_bluetooth_android_get_uuid_string(JNIEnv *env, java_object_t<N> obj) {
  auto uuid = obj.get_class().template get_method<j_uuid_t()>("getUuid")(obj);
  return uuid.get_class().template get_method<std::string()>("toString")(uuid);
}

static inline std::string
bare_bluetooth_android_create_characteristic_key(
  const std::string &service_uuid,
  const std::string &characteristic_uuid,
  int32_t instance_id
) {
  assert(!service_uuid.empty());
  assert(!characteristic_uuid.empty());

  return service_uuid + ":" + characteristic_uuid + ":" + std::to_string(instance_id);
}

static inline std::string
bare_bluetooth_android_get_characteristic_service_uuid(
  JNIEnv *env,
  j_bluetooth_gatt_characteristic_t characteristic
) {
  auto service = characteristic.get_class().get_method<j_bluetooth_gatt_service_t()>("getService")(characteristic);
  return bare_bluetooth_android_get_uuid_string(env, service);
}

static inline int32_t
bare_bluetooth_android_get_characteristic_instance_id(
  JNIEnv *env,
  j_bluetooth_gatt_characteristic_t characteristic
) {
  auto get_instance_id = characteristic.get_class().get_method<int()>("getInstanceId");
  return get_instance_id(characteristic);
}

static inline std::string
bare_bluetooth_android_get_characteristic_key(
  JNIEnv *env,
  j_bluetooth_gatt_characteristic_t characteristic
) {
  auto service_uuid = bare_bluetooth_android_get_characteristic_service_uuid(env, characteristic);
  auto characteristic_uuid = bare_bluetooth_android_get_uuid_string(env, characteristic);
  auto instance_id = bare_bluetooth_android_get_characteristic_instance_id(env, characteristic);

  return bare_bluetooth_android_create_characteristic_key(service_uuid, characteristic_uuid, instance_id);
}

static inline std::string
bare_bluetooth_android_get_device_address(JNIEnv *env, j_bluetooth_gatt_t gatt) {
  auto device = gatt.get_class().get_method<j_bluetooth_device_t()>("getDevice")(gatt);
  return device.get_class().get_method<std::string()>("getAddress")(device);
}

static inline java_array_t<unsigned char>
bare_bluetooth_android_make_byte_array(
  JNIEnv *env,
  const void *data,
  size_t length
) {
  auto arr = java_array_t<unsigned char>(env, static_cast<int>(length));
  arr.copy_from(std::span<const unsigned char>(static_cast<const unsigned char *>(data), length));
  return arr;
}

static inline bool
bare_bluetooth_android_check_exception(JNIEnv *env) {
  if (env->ExceptionCheck()) {
    env->ExceptionClear();
    return true;
  }
  return false;
}

struct bare_bluetooth_android_peripheral_t;

static std::unordered_map<uint64_t, bare_bluetooth_android_peripheral_t *> bare_bluetooth_android_peripherals;
static std::mutex bare_bluetooth_android_peripherals_mutex;
static uint64_t bare_bluetooth_android_next_peripheral_id = 1;

static bare_bluetooth_android_peripheral_t *
bare_bluetooth_android_find_peripheral(uint64_t id) {
  auto it = bare_bluetooth_android_peripherals.find(id);
  return it == bare_bluetooth_android_peripherals.end() ? nullptr : it->second;
}

struct bare_bluetooth_android_channel_t {
  js_env_t *env;
  js_ref_t *ctx;

  js_threadsafe_function_t *tsfn_data;
  js_threadsafe_function_t *tsfn_drain;
  js_threadsafe_function_t *tsfn_end;
  js_threadsafe_function_t *tsfn_error;
  js_threadsafe_function_t *tsfn_close;
  js_threadsafe_function_t *tsfn_open;

  java_global_ref_t<j_bluetooth_socket_t> socket;
  java_global_ref_t<j_hp_l2cap_reader_t> reader;

  // Stable id handed to the Java reader, resolved against the registry.
  uint64_t id;
  int psm;
  std::string peer_address;
  std::atomic<bool> opened;
  std::atomic<bool> destroyed;
  std::atomic<bool> finalized;
  bool exiting;

  js_deferred_teardown_t *teardown;
};

typedef struct {
  std::vector<unsigned char> data;
} bare_bluetooth_android_channel_data_t;

typedef struct {
  std::string message;
} bare_bluetooth_android_channel_error_t;

struct bare_bluetooth_android_device_handle_t {
  java_global_ref_t<j_bluetooth_device_t> handle;
};

struct bare_bluetooth_android_gatt_handle_t {
  java_global_ref_t<j_bluetooth_gatt_t> handle;
  java_global_ref_t<j_hp_gatt_callback_t> gatt_callback;
};

struct bare_bluetooth_android_characteristic_handle_t {
  java_global_ref_t<j_bluetooth_gatt_characteristic_t> handle;
};

struct bare_bluetooth_android_service_handle_t {
  java_global_ref_t<j_bluetooth_gatt_service_t> handle;
};

struct bare_bluetooth_android_socket_handle_t {
  java_global_ref_t<j_bluetooth_socket_t> handle;
};

struct bare_bluetooth_android_server_socket_handle_t {
  java_global_ref_t<j_bluetooth_server_socket_t> handle;
};

struct bare_bluetooth_android_uuid_handle_t {
  java_global_ref_t<j_uuid_t> handle;
};

template <typename T>
static void
bare_bluetooth_android__on_release(js_env_t *env, T *data) {
  delete data;
}

struct bare_bluetooth_android_central_t {
  js_env_t *env;
  js_ref_t *ctx;

  js_threadsafe_function_t *tsfn_state_change;
  js_threadsafe_function_t *tsfn_discover;
  js_threadsafe_function_t *tsfn_connect;
  js_threadsafe_function_t *tsfn_disconnect;
  js_threadsafe_function_t *tsfn_connect_fail;
  js_threadsafe_function_t *tsfn_scan_fail;

  java_global_ref_t<j_bluetooth_adapter_t> adapter;
  java_global_ref_t<j_bluetooth_le_scanner_t> scanner;
  java_global_ref_t<j_hp_scan_callback_t> scan_callback;
  java_global_ref_t<j_hp_gatt_callback_t> gatt_callback_ref;

  // Stable id handed to the Java callbacks, resolved against the registry.
  uint64_t id;
  bool exiting;
  std::atomic<bool> destroyed;
  std::mutex connected_addresses_mutex;
  std::unordered_set<std::string> connected_addresses;

  js_deferred_teardown_t *teardown;
};

typedef struct {
  int32_t state;
} bare_bluetooth_android_central_state_change_t;

typedef struct {
  std::string uuid;
  std::vector<unsigned char> data;
} bare_bluetooth_android_service_data_entry_t;

typedef struct {
  bool present;
  std::vector<bare_bluetooth_android_service_data_entry_t> service_data;
} bare_bluetooth_android_scan_record_t;

typedef struct {
  std::string address;
  std::string name;
  int32_t rssi;
  bare_bluetooth_android_scan_record_t scan_record;
} bare_bluetooth_android_central_discover_t;

typedef struct {
  std::string address;
} bare_bluetooth_android_central_connect_t;

typedef struct {
  std::string address;
  std::string error;
} bare_bluetooth_android_central_disconnect_t;

typedef struct {
  std::string address;
  std::string error;
} bare_bluetooth_android_central_connect_fail_t;

typedef struct {
  int32_t error_code;
} bare_bluetooth_android_central_scan_fail_t;

struct bare_bluetooth_android_peripheral_l2cap_open_req_t;

struct bare_bluetooth_android_peripheral_t {
  js_env_t *env;
  js_ref_t *ctx;

  js_threadsafe_function_t *tsfn_services_discover;
  js_threadsafe_function_t *tsfn_characteristics_discover;
  js_threadsafe_function_t *tsfn_read;
  js_threadsafe_function_t *tsfn_write;
  js_threadsafe_function_t *tsfn_notify;
  js_threadsafe_function_t *tsfn_notify_state;
  js_threadsafe_function_t *tsfn_mtu_changed;
  js_persistent_t<js_function_t<void, js_receiver_t, js_handle_t, js_handle_t, uint32_t>> on_channel_open;

  java_global_ref_t<j_bluetooth_gatt_t> gatt;
  java_global_ref_t<j_bluetooth_device_t> device;
  std::vector<java_global_ref_t<j_bluetooth_gatt_service_t>> services;
  std::vector<std::vector<java_global_ref_t<j_bluetooth_gatt_characteristic_t>>> service_characteristics;

  // Stable id handed to the GattCallback, resolved against the registry.
  uint64_t id;
  std::atomic<bool> destroyed;
  bool released;
  bool exiting;
  bool l2cap_connecting;
  bare_bluetooth_android_peripheral_l2cap_open_req_t *l2cap_open;

  js_deferred_teardown_t *teardown;
};

typedef struct {
  uint32_t count;
  std::string error;
} bare_bluetooth_android_peripheral_services_discover_t;

typedef struct {
  int32_t service_index;
  uint32_t count;
  std::string error;
} bare_bluetooth_android_peripheral_characteristics_discover_t;

typedef struct {
  std::string service_uuid;
  std::string uuid;
  int32_t instance_id;
  std::vector<unsigned char> data;
  std::string error;
} bare_bluetooth_android_peripheral_read_t;

typedef struct {
  std::string service_uuid;
  std::string uuid;
  int32_t instance_id;
  std::string error;
} bare_bluetooth_android_peripheral_write_t;

typedef struct {
  std::string service_uuid;
  std::string uuid;
  int32_t instance_id;
  std::vector<unsigned char> data;
  std::string error;
} bare_bluetooth_android_peripheral_notify_t;

typedef struct {
  std::string service_uuid;
  std::string uuid;
  int32_t instance_id;
  bool is_notifying;
  std::string error;
} bare_bluetooth_android_peripheral_notify_state_t;

typedef struct {
  int32_t mtu;
  std::string error;
} bare_bluetooth_android_peripheral_mtu_changed_t;

struct bare_bluetooth_android_server_t {
  js_env_t *env;
  js_ref_t *ctx;

  // Stable identity handed to the Java callbacks instead of a raw pointer.
  // Inbound callbacks resolve this id against the registry, so the native side
  // owns every threadsafe-function ref outright and frees deterministically.
  uint64_t id;
  bool exiting;
  std::atomic<bool> destroyed;

  js_threadsafe_function_t *tsfn_state_change;
  js_threadsafe_function_t *tsfn_add_service;
  js_threadsafe_function_t *tsfn_read_request;
  js_threadsafe_function_t *tsfn_write_request;
  js_threadsafe_function_t *tsfn_subscribe;
  js_threadsafe_function_t *tsfn_unsubscribe;
  js_threadsafe_function_t *tsfn_advertise_error;
  js_threadsafe_function_t *tsfn_channel_publish;
  js_threadsafe_function_t *tsfn_channel_open;
  js_threadsafe_function_t *tsfn_server_connection_state;
  js_threadsafe_function_t *tsfn_descriptor_response;
  js_threadsafe_function_t *tsfn_notify_sent;

  java_global_ref_t<j_bluetooth_adapter_t> adapter;
  java_global_ref_t<j_bluetooth_gatt_server_t> gatt_server;
  java_global_ref_t<j_bluetooth_le_advertiser_t> advertiser;
  java_global_ref_t<j_advertise_callback_t> advertise_callback;

  struct published_channel_t {
    java_global_ref_t<j_bluetooth_server_socket_t> server_socket;
    java_global_ref_t<j_hp_l2cap_acceptor_t> acceptor;
    uint16_t psm;
  };

  std::vector<published_channel_t *> published_channels;
  std::unordered_map<std::string, std::unordered_set<std::string>> subscriptions;
  std::unordered_set<std::string> connected_devices;
  std::unordered_map<std::string, java_global_ref_t<j_bluetooth_gatt_characteristic_t>> characteristics;

  js_deferred_teardown_t *teardown;
};

typedef struct {
  int32_t state;
} bare_bluetooth_android_server_state_change_t;

typedef struct {
  std::string uuid;
  std::string error;
} bare_bluetooth_android_server_add_service_t;

typedef struct {
  std::string device_address;
  int32_t request_id;
  std::string characteristic_uuid;
  int32_t characteristic_instance_id;
  int32_t offset;
} bare_bluetooth_android_server_read_request_t;

typedef struct {
  std::string device_address;
  int32_t request_id;
  std::string characteristic_uuid;
  int32_t characteristic_instance_id;
  int32_t offset;
  std::vector<unsigned char> data;
  bool response_needed;
} bare_bluetooth_android_server_write_request_t;

typedef struct {
  std::string device_address;
  std::string characteristic_uuid;
} bare_bluetooth_android_server_subscribe_t;

typedef struct {
  std::string device_address;
  std::string characteristic_uuid;
} bare_bluetooth_android_server_unsubscribe_t;

typedef struct {
  int32_t error_code;
  std::string error;
} bare_bluetooth_android_server_advertise_error_t;

typedef struct {
  std::string device_address;
  int32_t status;
} bare_bluetooth_android_server_notify_sent_t;

typedef struct {
  std::string device_address;
  int32_t request_id;
  int32_t offset;
  std::vector<unsigned char> data;
} bare_bluetooth_android_server_descriptor_response_t;

typedef struct {
  uint16_t psm;
  std::string error;
} bare_bluetooth_android_server_channel_publish_t;

struct bare_bluetooth_android_server_channel_open_t {
  int32_t socket_id;
  std::string error;
  uint16_t psm;
};

struct bare_bluetooth_android_server_connection_state_t {
  std::string address;
  int32_t status;
  int32_t new_state;
};

static void
bare_bluetooth_android_channel__on_data(
  js_env_t *env,
  js_function_t<void, js_receiver_t, js_typedarray_span_t<uint8_t>> function,
  bare_bluetooth_android_channel_t *context,
  bare_bluetooth_android_channel_data_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_channel_data_t *>(data);
  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(context);

  if (channel->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, channel->ctx, &receiver);
  assert(err == 0);

  err = js_call_function(env, function, js_receiver_t(receiver), js_typedarray_span_t<uint8_t>(event->data.data(), event->data.size()));
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_channel__on_drain(
  js_env_t *env,
  js_function_t<void, js_receiver_t> function,
  bare_bluetooth_android_channel_t *context,
  void *data
) {
  int err;

  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(context);

  if (channel->exiting) return;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, channel->ctx, &receiver);
  assert(err == 0);

  err = js_call_function(env, function, js_receiver_t(receiver));
  assert(err == 0);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_channel__on_end(
  js_env_t *env,
  js_function_t<void, js_receiver_t> function,
  bare_bluetooth_android_channel_t *context,
  void *data
) {
  int err;

  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(context);

  if (channel->exiting) return;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, channel->ctx, &receiver);
  assert(err == 0);

  err = js_call_function(env, function, js_receiver_t(receiver));
  assert(err == 0);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_channel__on_error(
  js_env_t *env,
  js_function_t<void, js_receiver_t, std::string> function,
  bare_bluetooth_android_channel_t *context,
  bare_bluetooth_android_channel_error_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_channel_error_t *>(data);
  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(context);

  if (channel->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, channel->ctx, &receiver);
  assert(err == 0);

  err = js_call_function(env, function, js_receiver_t(receiver), event->message);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

// Registry of live channels. Reader callbacks hold the mutex while dispatching
// and the channel is erased under it when freed, so it is never freed mid-use.
static std::mutex bare_bluetooth_android_channels_mutex;
static std::unordered_map<uint64_t, bare_bluetooth_android_channel_t *> bare_bluetooth_android_channels;
static uint64_t bare_bluetooth_android_next_channel_id = 1;

static bare_bluetooth_android_channel_t *
bare_bluetooth_android_find_channel(uint64_t id) {
  auto it = bare_bluetooth_android_channels.find(id);
  return it == bare_bluetooth_android_channels.end() ? nullptr : it->second;
}

static void
bare_bluetooth_android_channel__on_close(
  js_env_t *env,
  js_function_t<void, js_receiver_t> function,
  bare_bluetooth_android_channel_t *context,
  void *data
) {
  int err;

  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(context);

  if (!channel->exiting) {
    js_handle_scope_t *scope;
    err = js_open_handle_scope(env, &scope);
    assert(err == 0);

    js_value_t *receiver;
    err = js_get_reference_value(env, channel->ctx, &receiver);
    assert(err == 0);

    err = js_call_function(env, function, js_receiver_t(receiver));
    assert(err == 0);

    err = js_close_handle_scope(env, scope);
    assert(err == 0);
  }

  bool expected = false;
  if (channel->finalized.compare_exchange_strong(expected, true)) {
    {
      std::lock_guard<std::mutex> lock(bare_bluetooth_android_channels_mutex);
      bare_bluetooth_android_channels.erase(channel->id);
    }

    js_release_threadsafe_function(channel->tsfn_open, js_threadsafe_function_abort);
    js_release_threadsafe_function(channel->tsfn_close, js_threadsafe_function_abort);
    js_release_threadsafe_function(channel->tsfn_error, js_threadsafe_function_abort);
    js_release_threadsafe_function(channel->tsfn_end, js_threadsafe_function_abort);
    js_release_threadsafe_function(channel->tsfn_drain, js_threadsafe_function_abort);
    js_release_threadsafe_function(channel->tsfn_data, js_threadsafe_function_abort);

    err = js_delete_reference(env, channel->ctx);
    assert(err == 0);

    err = js_finish_deferred_teardown_callback(channel->teardown);
    assert(err == 0);

    delete channel;
  }
}

static void
bare_bluetooth_android_channel__on_open(
  js_env_t *env,
  js_function_t<void, js_receiver_t> function,
  bare_bluetooth_android_channel_t *context,
  void *data
) {
  int err;

  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(context);

  if (channel->exiting) return;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, channel->ctx, &receiver);
  assert(err == 0);

  err = js_call_function(env, function, js_receiver_t(receiver));
  assert(err == 0);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_on_l2cap_reader_open(
  java_env_t,
  j_hp_l2cap_reader_t,
  long native_ptr
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_channels_mutex);
  auto *channel = bare_bluetooth_android_find_channel(static_cast<uint64_t>(native_ptr));
  if (channel == nullptr || channel->destroyed) return;

  js_call_threadsafe_function(channel->tsfn_open);
}

static void
bare_bluetooth_android_on_l2cap_reader_data(
  java_env_t env,
  j_hp_l2cap_reader_t,
  long native_ptr,
  java_array_t<unsigned char> data
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_channels_mutex);
  auto *channel = bare_bluetooth_android_find_channel(static_cast<uint64_t>(native_ptr));
  if (channel == nullptr || channel->destroyed) return;

  auto *event = new bare_bluetooth_android_channel_data_t();
  event->data = data.slice();

  js_call_threadsafe_function(channel->tsfn_data, event);
}

static void
bare_bluetooth_android_on_l2cap_reader_end(
  java_env_t,
  j_hp_l2cap_reader_t,
  long native_ptr
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_channels_mutex);
  auto *channel = bare_bluetooth_android_find_channel(static_cast<uint64_t>(native_ptr));
  if (channel == nullptr || channel->destroyed) return;

  js_call_threadsafe_function(channel->tsfn_end);
}

static void
bare_bluetooth_android_on_l2cap_reader_error(
  java_env_t,
  j_hp_l2cap_reader_t,
  long native_ptr,
  std::string message
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_channels_mutex);
  auto *channel = bare_bluetooth_android_find_channel(static_cast<uint64_t>(native_ptr));
  if (channel == nullptr || channel->destroyed) return;

  auto *event = new bare_bluetooth_android_channel_error_t();
  event->message = message;

  js_call_threadsafe_function(channel->tsfn_error, event);
}

static void
bare_bluetooth_android_on_l2cap_reader_close(
  java_env_t,
  j_hp_l2cap_reader_t,
  long native_ptr
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_channels_mutex);
  auto *channel = bare_bluetooth_android_find_channel(static_cast<uint64_t>(native_ptr));
  if (channel == nullptr) return;

  js_call_threadsafe_function(channel->tsfn_close);
}

static void
bare_bluetooth_android_channel__on_teardown(js_deferred_teardown_t *handle, void *data);

static js_external_t<bare_bluetooth_android_channel_t>
bare_bluetooth_android_l2cap_init(
  js_env_t *env,
  bare_bluetooth_android_socket_handle_t *socket_handle,
  js_object_t ctx,
  js_function_t<void, js_receiver_t, js_typedarray_span_t<uint8_t>> on_data,
  js_function_t<void, js_receiver_t> on_drain,
  js_function_t<void, js_receiver_t> on_end,
  js_function_t<void, js_receiver_t, std::string> on_error,
  js_function_t<void, js_receiver_t> on_close,
  js_function_t<void, js_receiver_t> on_open
) {
  int err;

  auto *channel = new bare_bluetooth_android_channel_t();
  channel->env = env;
  channel->socket = java_global_ref_t<j_bluetooth_socket_t>(std::move(socket_handle->handle));
  channel->opened = false;
  channel->destroyed = false;
  channel->finalized = false;
  channel->exiting = false;

  {
    std::lock_guard<std::mutex> lock(bare_bluetooth_android_channels_mutex);
    channel->id = bare_bluetooth_android_next_channel_id++;
    bare_bluetooth_android_channels.emplace(channel->id, channel);
  }

  delete socket_handle;

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto socket = j_bluetooth_socket_t(jenv, channel->socket);
  auto get_device = socket.get_class().get_method<j_bluetooth_device_t()>("getRemoteDevice");
  auto device = get_device(socket);
  auto get_address = device.get_class().get_method<std::string()>("getAddress");

  channel->peer_address = get_address(device);
  channel->psm = 0;

  err = js_create_reference(env, static_cast<js_value_t *>(ctx), 1, &channel->ctx);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_channel__on_data, bare_bluetooth_android_channel_t, bare_bluetooth_android_channel_data_t>(env, on_data, 0, 1, channel, channel->tsfn_data);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_channel__on_drain, bare_bluetooth_android_channel_t, void>(env, on_drain, 0, 1, channel, channel->tsfn_drain);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_channel__on_end, bare_bluetooth_android_channel_t, void>(env, on_end, 0, 1, channel, channel->tsfn_end);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_channel__on_error, bare_bluetooth_android_channel_t, bare_bluetooth_android_channel_error_t>(env, on_error, 0, 1, channel, channel->tsfn_error);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_channel__on_close, bare_bluetooth_android_channel_t, void>(env, on_close, 0, 1, channel, channel->tsfn_close);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_channel__on_open, bare_bluetooth_android_channel_t, void>(env, on_open, 0, 1, channel, channel->tsfn_open);
  assert(err == 0);

  js_external_t<bare_bluetooth_android_channel_t> handle;
  err = js_create_external(env, channel, handle);
  assert(err == 0);

  err = js_add_deferred_teardown_callback(env, bare_bluetooth_android_channel__on_teardown, (void *) channel, &channel->teardown);
  assert(err == 0);

  return handle;
}

static void
bare_bluetooth_android_l2cap_open(js_env_t *env, bare_bluetooth_android_channel_t *channel) {
  bool expected = false;
  if (!channel->opened.compare_exchange_strong(expected, true)) return;

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto socket = j_bluetooth_socket_t(jenv, channel->socket);
  auto reader_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/L2capReader">();
  auto reader = reader_class(socket, static_cast<long>(channel->id));
  channel->reader = java_global_ref_t<j_hp_l2cap_reader_t>(jenv, reader);

  auto start = reader.get_class().get_method<void()>("start");
  start(reader);
}

static int32_t
bare_bluetooth_android_l2cap_write(
  js_env_t *env,
  bare_bluetooth_android_channel_t *channel,
  js_typedarray_span_t<uint8_t> data
) {
  if (channel->destroyed || !channel->opened) return 0;

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto socket = j_bluetooth_socket_t(jenv, channel->socket);
  auto get_output = socket.get_class().get_method<j_output_stream_t()>("getOutputStream");
  auto output = get_output(socket);

  auto byte_array = bare_bluetooth_android_make_byte_array(jenv, data.data(), data.size());

  auto write_method = output.get_class().get_method<void(java_array_t<unsigned char>)>("write");
  write_method(output, byte_array);

  bool write_ok = !bare_bluetooth_android_check_exception(jenv);

  if (write_ok) {
    js_call_threadsafe_function(channel->tsfn_drain);
  } else {
    auto *event = new bare_bluetooth_android_channel_error_t();
    event->message = std::string("Write error");
    js_call_threadsafe_function(channel->tsfn_error, event);
  }

  return write_ok ? static_cast<int32_t>(data.size()) : 0;
}

static void
bare_bluetooth_android_l2cap_end(js_env_t *env, bare_bluetooth_android_channel_t *channel) {
  bool expected = false;
  if (!channel->destroyed.compare_exchange_strong(expected, true)) return;

  if (!channel->opened) {
    js_call_threadsafe_function(channel->tsfn_close);
    return;
  }

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  if (static_cast<jobject>(channel->reader) != nullptr) {
    auto reader = j_hp_l2cap_reader_t(jenv, channel->reader);
    auto stop = reader.get_class().get_method<void()>("stop");
    stop(reader);
    static_cast<JNIEnv *>(jenv)->ExceptionClear();
  }

  auto socket = j_bluetooth_socket_t(jenv, channel->socket);
  auto close = socket.get_class().get_method<void()>("close");
  close(socket);
  static_cast<JNIEnv *>(jenv)->ExceptionClear();
}

static void
bare_bluetooth_android_channel__on_teardown(js_deferred_teardown_t *handle, void *data) {
  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(data);
  channel->exiting = true;

  bare_bluetooth_android_l2cap_end(channel->env, channel);
}

static uint32_t
bare_bluetooth_android_l2cap_psm(js_env_t *env, bare_bluetooth_android_channel_t *channel) {
  return static_cast<uint32_t>(channel->psm);
}

static std::optional<std::string>
bare_bluetooth_android_l2cap_peer(js_env_t *env, bare_bluetooth_android_channel_t *channel) {
  if (channel->peer_address.empty()) return std::nullopt;

  return channel->peer_address;
}

static void
bare_bluetooth_android_central__on_state_change(
  js_env_t *env,
  js_function_t<void, js_receiver_t, int32_t> function,
  bare_bluetooth_android_central_t *context,
  bare_bluetooth_android_central_state_change_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_central_state_change_t *>(data);
  auto *central = static_cast<bare_bluetooth_android_central_t *>(context);

  if (central->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  err = js_call_function(env, function, js_receiver_t(receiver), event->state);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_discover(
  js_env_t *env,
  js_function_t<void, js_receiver_t, std::string, js_handle_t, int32_t, js_handle_t> function,
  bare_bluetooth_android_central_t *context,
  bare_bluetooth_android_central_discover_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_central_discover_t *>(data);
  auto *central = static_cast<bare_bluetooth_android_central_t *>(context);

  if (central->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_handle_t name;

  if (!event->name.empty()) {
    name = js_handle_t(js_marshall_untyped_value(env, event->name));
  } else {
    js_value_t *null;
    err = js_get_null(env, &null);
    assert(err == 0);
    name = js_handle_t(null);
  }

  js_handle_t scan_record;

  if (!event->scan_record.present) {
    js_value_t *null;
    err = js_get_null(env, &null);
    assert(err == 0);
    scan_record = js_handle_t(null);
  } else {
    js_handle_t service_data;

    if (event->scan_record.service_data.empty()) {
      js_object_t null;
      err = js_get_null(env, null);
      assert(err == 0);
      service_data = js_handle_t(null);
    } else {
      js_object_t object;
      err = js_create_object(env, object);
      assert(err == 0);

      for (const auto &entry : event->scan_record.service_data) {
        js_typedarray_t<> view;
        err = js_create_typedarray(env, entry.data, view);
        assert(err == 0);

        err = js_set_property(env, object, entry.uuid, view);
        assert(err == 0);
      }

      service_data = js_handle_t(object);
    }

    js_object_t record;
    err = js_create_object(env, record);
    assert(err == 0);

    err = js_set_property(env, record, "serviceData", service_data);
    assert(err == 0);

    scan_record = js_handle_t(record);
  }

  js_function_t<void, js_receiver_t, std::string, js_handle_t, int32_t, js_handle_t> callback(function);

  err = js_call_function(
    env,
    callback,
    js_receiver_t(receiver),
    event->address,
    name,
    event->rssi,
    scan_record
  );
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_connect(
  js_env_t *env,
  js_function_t<void, js_receiver_t, js_handle_t, std::string> function,
  bare_bluetooth_android_central_t *context,
  bare_bluetooth_android_central_connect_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_central_connect_t *>(data);
  auto *central = static_cast<bare_bluetooth_android_central_t *>(context);

  if (central->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto callback = j_hp_gatt_callback_t(jenv, central->gatt_callback_ref);
  auto take_connected_gatt = callback.get_class().get_method<j_bluetooth_gatt_t(std::string)>("takeConnectedGatt");
  auto gatt = take_connected_gatt(callback, event->address);

  assert(static_cast<jobject>(gatt) != nullptr);

  auto *gatt_handle = new bare_bluetooth_android_gatt_handle_t{
    java_global_ref_t<j_bluetooth_gatt_t>(jenv, gatt),
    java_global_ref_t<j_hp_gatt_callback_t>(jenv, callback)
  };

  js_external_t<bare_bluetooth_android_gatt_handle_t> ext;
  err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_gatt_handle_t>>(env, gatt_handle, ext);
  assert(err == 0);

  js_function_t<void, js_receiver_t, js_handle_t, std::string> callback_fn(function);

  err = js_call_function(env, callback_fn, js_receiver_t(receiver), js_handle_t(static_cast<js_value_t *>(ext)), event->address);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_disconnect(
  js_env_t *env,
  js_function_t<void, js_receiver_t, std::string, js_handle_t> function,
  bare_bluetooth_android_central_t *context,
  bare_bluetooth_android_central_disconnect_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_central_disconnect_t *>(data);
  auto *central = static_cast<bare_bluetooth_android_central_t *>(context);

  if (central->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *error;

  if (!event->error.empty()) {
    error = js_marshall_untyped_value(env, event->error);
  } else {
    err = js_get_null(env, &error);
    assert(err == 0);
  }

  err = js_call_function(env, function, js_receiver_t(receiver), event->address, js_handle_t(error));
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_connect_fail(
  js_env_t *env,
  js_function_t<void, js_receiver_t, std::string, std::string> function,
  bare_bluetooth_android_central_t *context,
  bare_bluetooth_android_central_connect_fail_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_central_connect_fail_t *>(data);
  auto *central = static_cast<bare_bluetooth_android_central_t *>(context);

  if (central->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  err = js_call_function(env, function, js_receiver_t(receiver), event->address, event->error);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_scan_fail(
  js_env_t *env,
  js_function_t<void, js_receiver_t, int32_t> function,
  bare_bluetooth_android_central_t *context,
  bare_bluetooth_android_central_scan_fail_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_central_scan_fail_t *>(data);
  auto *central = static_cast<bare_bluetooth_android_central_t *>(context);

  if (central->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  err = js_call_function(env, function, js_receiver_t(receiver), event->error_code);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_teardown(js_deferred_teardown_t *handle, void *data);

// Registry of live centrals. Callbacks hold the mutex while dispatching and
// teardown erases under it, so a central is never freed mid-use.
static std::mutex bare_bluetooth_android_centrals_mutex;
static std::unordered_map<uint64_t, bare_bluetooth_android_central_t *> bare_bluetooth_android_centrals;
static uint64_t bare_bluetooth_android_next_central_id = 1;

static bare_bluetooth_android_central_t *
bare_bluetooth_android_find_central(uint64_t id) {
  auto it = bare_bluetooth_android_centrals.find(id);
  return it == bare_bluetooth_android_centrals.end() ? nullptr : it->second;
}

static js_external_t<bare_bluetooth_android_central_t>
bare_bluetooth_android_central_init(
  js_env_t *env,
  js_object_t ctx,
  js_function_t<void, js_receiver_t, int32_t> on_state_change,
  js_function_t<void, js_receiver_t, std::string, js_handle_t, int32_t, js_handle_t> on_discover,
  js_function_t<void, js_receiver_t, js_handle_t, std::string> on_connect,
  js_function_t<void, js_receiver_t, std::string, js_handle_t> on_disconnect,
  js_function_t<void, js_receiver_t, std::string, std::string> on_connect_fail,
  js_function_t<void, js_receiver_t, int32_t> on_scan_fail
) {
  int err;

  auto *central = new bare_bluetooth_android_central_t();
  central->env = env;
  central->destroyed = false;
  central->exiting = false;

  {
    std::lock_guard<std::mutex> lock(bare_bluetooth_android_centrals_mutex);
    central->id = bare_bluetooth_android_next_central_id++;
    bare_bluetooth_android_centrals.emplace(central->id, central);
  }

  err = js_create_reference(env, static_cast<js_value_t *>(ctx), 1, &central->ctx);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_central__on_state_change, bare_bluetooth_android_central_t, bare_bluetooth_android_central_state_change_t>(env, on_state_change, 0, 1, central, central->tsfn_state_change);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_central__on_discover, bare_bluetooth_android_central_t, bare_bluetooth_android_central_discover_t>(env, on_discover, 0, 1, central, central->tsfn_discover);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_central__on_connect, bare_bluetooth_android_central_t, bare_bluetooth_android_central_connect_t>(env, on_connect, 0, 1, central, central->tsfn_connect);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_central__on_disconnect, bare_bluetooth_android_central_t, bare_bluetooth_android_central_disconnect_t>(env, on_disconnect, 0, 1, central, central->tsfn_disconnect);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_central__on_connect_fail, bare_bluetooth_android_central_t, bare_bluetooth_android_central_connect_fail_t>(env, on_connect_fail, 0, 1, central, central->tsfn_connect_fail);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_central__on_scan_fail, bare_bluetooth_android_central_t, bare_bluetooth_android_central_scan_fail_t>(env, on_scan_fail, 0, 1, central, central->tsfn_scan_fail);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto activity = bare_bluetooth_android_get_context(jenv);
  auto get_system_service = activity.get_class().get_method<j_object_t(std::string)>("getSystemService");
  auto manager_obj = get_system_service(activity, std::string("bluetooth"));

  auto bt_manager = j_bluetooth_manager_t(jenv, manager_obj);
  auto get_adapter = bt_manager.get_class().get_method<j_bluetooth_adapter_t()>("getAdapter");
  auto adapter_local = get_adapter(bt_manager);

  central->adapter = java_global_ref_t<j_bluetooth_adapter_t>(jenv, adapter_local);

  auto get_scanner = central->adapter.get_class().get_method<j_bluetooth_le_scanner_t()>("getBluetoothLeScanner");
  auto scanner_local = get_scanner(central->adapter);

  central->scanner = java_global_ref_t<j_bluetooth_le_scanner_t>(jenv, scanner_local);

  auto get_state = central->adapter.get_class().get_method<int()>("getState");
  int android_state = get_state(central->adapter);

  auto *state_event = new bare_bluetooth_android_central_state_change_t();
  state_event->state = android_state;

  js_call_threadsafe_function(central->tsfn_state_change, state_event);

  js_external_t<bare_bluetooth_android_central_t> handle;
  err = js_create_external(env, central, handle);
  assert(err == 0);

  err = js_add_deferred_teardown_callback(env, bare_bluetooth_android_central__on_teardown, (void *) central, &central->teardown);
  assert(err == 0);

  return handle;
}

static void
bare_bluetooth_android_central_start_scan(
  js_env_t *env,
  bare_bluetooth_android_central_t *central,
  std::optional<std::vector<bare_bluetooth_android_uuid_handle_t *>> uuids,
  std::optional<int32_t> scan_mode
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto helper_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/ScanHelper">();
  auto helper = helper_class(static_cast<int>(scan_mode.value_or(2)));

  if (uuids) {
    auto add_uuid = helper.get_class().get_method<void(j_uuid_t)>("addServiceUuid");

    for (auto *uuid_handle : *uuids) {
      add_uuid(helper, j_uuid_t(jenv, uuid_handle->handle));
    }
  }

  auto callback_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/ScanCallback">();
  auto callback_local = callback_class(static_cast<long>(central->id));

  central->scan_callback = java_global_ref_t<j_hp_scan_callback_t>(jenv, callback_local);

  auto start = helper.get_class().get_method<void(j_bluetooth_le_scanner_t, j_scan_callback_t)>("startScan");
  start(helper, j_bluetooth_le_scanner_t(jenv, central->scanner), j_scan_callback_t(jenv, central->scan_callback));
}

static void
bare_bluetooth_android_central_stop_scan(js_env_t *env, bare_bluetooth_android_central_t *central) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto stop_scan = central->scanner.get_class().get_method<void(j_scan_callback_t)>("stopScan");
  stop_scan(central->scanner, j_scan_callback_t(jenv, central->scan_callback));
}

static void
bare_bluetooth_android_central_connect(
  js_env_t *env,
  bare_bluetooth_android_central_t *central,
  std::string address
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto adapter = j_bluetooth_adapter_t(jenv, central->adapter);
  auto get_remote_device = adapter.get_class().get_method<j_bluetooth_device_t(std::string)>("getRemoteDevice");
  auto device = get_remote_device(adapter, address);

  auto gatt_callback_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/GattCallback">();
  auto gatt_callback = gatt_callback_class(static_cast<long>(central->id));

  central->gatt_callback_ref = java_global_ref_t<j_hp_gatt_callback_t>(jenv, gatt_callback);

  auto context = bare_bluetooth_android_get_context(jenv);

  auto connect_gatt = device.get_class().get_method<j_bluetooth_gatt_t(j_context_t, bool, j_bluetooth_gatt_callback_t, int)>("connectGatt");
  connect_gatt(device, context, false, j_bluetooth_gatt_callback_t(jenv, gatt_callback), 2);
}

static void
bare_bluetooth_android_central_disconnect(
  js_env_t *env,
  bare_bluetooth_android_central_t *central,
  bare_bluetooth_android_gatt_handle_t *gatt_handle
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto gatt = j_bluetooth_gatt_t(jenv, gatt_handle->handle);

  auto disconnect = gatt.get_class().get_method<void()>("disconnect");
  disconnect(gatt);

  auto close = gatt.get_class().get_method<void()>("close");
  close(gatt);
}

static void
bare_bluetooth_android_central_release(bare_bluetooth_android_central_t *central) {
  int err = js_delete_reference(central->env, central->ctx);
  assert(err == 0);

  js_release_threadsafe_function(central->tsfn_scan_fail, js_threadsafe_function_abort);
  js_release_threadsafe_function(central->tsfn_connect_fail, js_threadsafe_function_abort);
  js_release_threadsafe_function(central->tsfn_disconnect, js_threadsafe_function_abort);
  js_release_threadsafe_function(central->tsfn_connect, js_threadsafe_function_abort);
  js_release_threadsafe_function(central->tsfn_discover, js_threadsafe_function_abort);
  js_release_threadsafe_function(central->tsfn_state_change, js_threadsafe_function_abort);
}

static void
bare_bluetooth_android_central__on_teardown(js_deferred_teardown_t *handle, void *data) {
  auto *central = static_cast<bare_bluetooth_android_central_t *>(data);

  bool expected = false;
  if (!central->destroyed.compare_exchange_strong(expected, true)) return;

  {
    std::lock_guard<std::mutex> lock(bare_bluetooth_android_centrals_mutex);
    central->exiting = true;
    bare_bluetooth_android_centrals.erase(central->id);
  }

  bare_bluetooth_android_central_release(central);

  int err = js_finish_deferred_teardown_callback(central->teardown);
  assert(err == 0);

  delete central;
}

static void
bare_bluetooth_android_central_destroy(js_env_t *env, bare_bluetooth_android_central_t *central) {
  bool expected = false;
  if (!central->destroyed.compare_exchange_strong(expected, true)) return;

  {
    std::lock_guard<std::mutex> lock(bare_bluetooth_android_centrals_mutex);
    bare_bluetooth_android_centrals.erase(central->id);
  }

  bare_bluetooth_android_central_release(central);

  int err = js_finish_deferred_teardown_callback(central->teardown);
  assert(err == 0);

  delete central;
}

static js_external_t<bare_bluetooth_android_uuid_handle_t>
bare_bluetooth_android_create_uuid(js_env_t *env, std::string str) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto uuid_class = java_class_t<"java/util/UUID">(jenv);
  auto from_string = uuid_class.get_static_method<j_uuid_t(std::string)>("fromString");
  auto uuid_local = from_string(str);

  auto *uuid_handle = new bare_bluetooth_android_uuid_handle_t{java_global_ref_t<j_uuid_t>(jenv, uuid_local)};

  js_external_t<bare_bluetooth_android_uuid_handle_t> handle;
  int err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_uuid_handle_t>>(env, uuid_handle, handle);
  assert(err == 0);

  return handle;
}

static void
bare_bluetooth_android_on_scan_result(
  java_env_t env,
  j_hp_scan_callback_t self,
  long native_ptr,
  int callback_type,
  j_scan_result_t scan_result
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_centrals_mutex);
  auto *central = bare_bluetooth_android_find_central(static_cast<uint64_t>(native_ptr));
  if (central == nullptr || central->exiting) return;

  auto device = scan_result.get_class().get_method<j_bluetooth_device_t()>("getDevice")(scan_result);
  auto address = device.get_class().get_method<std::string()>("getAddress")(device);
  auto rssi = scan_result.get_class().get_method<int()>("getRssi")(scan_result);

  auto name_obj = device.get_class().get_method<j_string_t()>("getName")(device);

  auto *event = new bare_bluetooth_android_central_discover_t();
  event->address = address;
  event->rssi = rssi;

  auto scan_record = scan_result.get_class().get_method<j_scan_record_t()>("getScanRecord")(scan_result);
  event->scan_record.present = scan_record != nullptr;

  if (event->scan_record.present) {
    auto service_data_map = scan_record.get_class().get_method<j_map_t()>("getServiceData")(scan_record);

    if (service_data_map != nullptr) {
      auto entry_set = service_data_map.get_class().get_method<j_set_t()>("entrySet")(service_data_map);
      auto iterator = entry_set.get_class().get_method<j_iterator_t()>("iterator")(entry_set);

      auto has_next = iterator.get_class().get_method<bool()>("hasNext");
      auto next = iterator.get_class().get_method<j_object_t()>("next");

      while (has_next(iterator)) {
        auto entry_obj = next(iterator);
        auto entry = j_map_entry_t(env, static_cast<jobject>(entry_obj));

        auto key_obj = entry.get_class().get_method<j_object_t()>("getKey")(entry);
        auto value_obj = entry.get_class().get_method<j_object_t()>("getValue")(entry);

        auto parcel_uuid = j_parcel_uuid_t(env, key_obj);
        auto uuid = bare_bluetooth_android_get_uuid_string(env, parcel_uuid);

        auto data = java_array_t<unsigned char>(env, value_obj);

        if (data.size() > 0) {
          event->scan_record.service_data.push_back({std::move(uuid), data.slice()});
        }
      }
    }
  }

  if (static_cast<jobject>(name_obj) != nullptr) {
    auto name = java_string_t(env, name_obj);
    event->name = std::string(name);
  } else {
    event->name = {};
  }

  js_call_threadsafe_function(central->tsfn_discover, event);
}

static void
bare_bluetooth_android_on_scan_failed(
  java_env_t env,
  j_hp_scan_callback_t self,
  long native_ptr,
  int error_code
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_centrals_mutex);
  auto *central = bare_bluetooth_android_find_central(static_cast<uint64_t>(native_ptr));
  if (central == nullptr || central->exiting) return;

  auto *event = new bare_bluetooth_android_central_scan_fail_t();
  event->error_code = error_code;

  js_call_threadsafe_function(central->tsfn_scan_fail, event);
}

static void
bare_bluetooth_android_on_connection_state_change(
  java_env_t env,
  j_hp_gatt_callback_t self,
  long native_ptr,
  j_bluetooth_gatt_t gatt,
  int status,
  int new_state
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_centrals_mutex);
  auto *central = bare_bluetooth_android_find_central(static_cast<uint64_t>(native_ptr));
  if (central == nullptr || central->exiting) return;

  auto address = bare_bluetooth_android_get_device_address(env, gatt);

  if (new_state == 2 && status == 0) {
    {
      std::lock_guard<std::mutex> lock(central->connected_addresses_mutex);
      central->connected_addresses.insert(address);
    }

    auto *event = new bare_bluetooth_android_central_connect_t();
    event->address = address;

    js_call_threadsafe_function(central->tsfn_connect, event);
  } else if (new_state == 0) {
    bool was_connected;
    {
      std::lock_guard<std::mutex> lock(central->connected_addresses_mutex);
      was_connected = central->connected_addresses.erase(address) > 0;
    }

    if (was_connected) {
      auto *event = new bare_bluetooth_android_central_disconnect_t();
      event->address = address;

      if (status != 0) {
        char error_buf[64];
        snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
        event->error = error_buf;
      } else {
        event->error = {};
      }

      js_call_threadsafe_function(central->tsfn_disconnect, event);
    } else {
      auto *event = new bare_bluetooth_android_central_connect_fail_t();
      event->address = address;

      char error_buf[64];
      snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
      event->error = error_buf;

      js_call_threadsafe_function(central->tsfn_connect_fail, event);
    }
  }
}

static void
bare_bluetooth_android_peripheral__on_services_discover(
  js_env_t *env,
  js_function_t<void, js_receiver_t, uint32_t, js_handle_t> function,
  bare_bluetooth_android_peripheral_t *context,
  bare_bluetooth_android_peripheral_services_discover_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_services_discover_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  if (peripheral->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  if (event->error.empty()) {
    auto jenv = bare_bluetooth_android_jvm().get_env().value();

    peripheral->service_characteristics.clear();
    peripheral->services.clear();

    auto gatt = j_bluetooth_gatt_t(jenv, peripheral->gatt);
    auto get_services = gatt.get_class().get_method<j_list_t()>("getServices");
    auto services_list = get_services(gatt);
    auto list_size = services_list.get_class().get_method<int()>("size");
    auto list_get = services_list.get_class().get_method<j_object_t(int)>("get");
    int service_count = list_size(services_list);

    for (int i = 0; i < service_count; i++) {
      auto service_obj = list_get(services_list, i);
      peripheral->services.emplace_back(jenv, service_obj);

      auto service_typed = j_bluetooth_gatt_service_t(jenv, service_obj);
      auto get_chars = service_typed.get_class().get_method<j_list_t()>("getCharacteristics");
      auto chars_list = get_chars(service_typed);
      auto chars_size = chars_list.get_class().get_method<int()>("size");
      auto chars_get = chars_list.get_class().get_method<j_object_t(int)>("get");
      int char_count = chars_size(chars_list);

      std::vector<java_global_ref_t<j_bluetooth_gatt_characteristic_t>> chars;
      for (int j = 0; j < char_count; j++) {
        auto char_obj = chars_get(chars_list, j);
        chars.emplace_back(jenv, char_obj);
      }
      peripheral->service_characteristics.push_back(std::move(chars));
    }

    event->count = static_cast<uint32_t>(service_count);
  }

  js_value_t *error;

  if (!event->error.empty()) {
    error = js_marshall_untyped_value(env, event->error);
  } else {
    err = js_get_null(env, &error);
    assert(err == 0);
  }

  err = js_call_function(env, function, js_receiver_t(receiver), event->count, js_handle_t(error));
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_characteristics_discover(
  js_env_t *env,
  js_function_t<void, js_receiver_t, js_handle_t, uint32_t, js_handle_t> function,
  bare_bluetooth_android_peripheral_t *context,
  bare_bluetooth_android_peripheral_characteristics_discover_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_characteristics_discover_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  if (peripheral->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *service_value;

  if (event->service_index >= 0 && static_cast<size_t>(event->service_index) < peripheral->services.size()) {
    auto jenv = bare_bluetooth_android_jvm().get_env().value();
    auto *service = new bare_bluetooth_android_service_handle_t{
      java_global_ref_t<j_bluetooth_gatt_service_t>(jenv, peripheral->services[static_cast<size_t>(event->service_index)])
    };

    js_external_t<bare_bluetooth_android_service_handle_t> ext;
    err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_service_handle_t>>(env, service, ext);
    assert(err == 0);
    service_value = static_cast<js_value_t *>(ext);
  } else {
    err = js_get_null(env, &service_value);
    assert(err == 0);
  }

  js_value_t *error;

  if (!event->error.empty()) {
    error = js_marshall_untyped_value(env, event->error);
  } else {
    err = js_get_null(env, &error);
    assert(err == 0);
  }

  err = js_call_function(env, function, js_receiver_t(receiver), js_handle_t(service_value), event->count, js_handle_t(error));
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static bare_bluetooth_android_characteristic_handle_t *
bare_bluetooth_android_peripheral_find_characteristic_handle(
  JNIEnv *env,
  bare_bluetooth_android_peripheral_t *peripheral,
  const std::string &service_uuid,
  const std::string &uuid,
  int32_t instance_id
) {
  for (auto &chars : peripheral->service_characteristics) {
    for (auto &char_ref : chars) {
      auto characteristic = j_bluetooth_gatt_characteristic_t(env, char_ref);
      auto current_service_uuid = bare_bluetooth_android_get_characteristic_service_uuid(env, characteristic);
      auto current_uuid = bare_bluetooth_android_get_uuid_string(env, characteristic);
      auto current_instance_id = bare_bluetooth_android_get_characteristic_instance_id(env, characteristic);

      if (current_service_uuid == service_uuid && current_uuid == uuid && current_instance_id == instance_id) {
        return new bare_bluetooth_android_characteristic_handle_t{
          java_global_ref_t<j_bluetooth_gatt_characteristic_t>(env, char_ref)
        };
      }
    }
  }

  return nullptr;
}

static void
bare_bluetooth_android_peripheral__on_read(
  js_env_t *env,
  js_function_t<void, js_receiver_t, js_handle_t, std::string, js_handle_t, js_handle_t> function,
  bare_bluetooth_android_peripheral_t *context,
  bare_bluetooth_android_peripheral_read_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_read_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  if (peripheral->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto *characteristic = bare_bluetooth_android_peripheral_find_characteristic_handle(jenv, peripheral, event->service_uuid, event->uuid, event->instance_id);

  js_value_t *characteristic_value;

  if (characteristic) {
    js_external_t<bare_bluetooth_android_characteristic_handle_t> ext;
    err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_characteristic_handle_t>>(env, characteristic, ext);
    assert(err == 0);
    characteristic_value = static_cast<js_value_t *>(ext);
  } else {
    err = js_get_null(env, &characteristic_value);
    assert(err == 0);
    if (event->error.empty()) event->error = "Characteristic not found in cache";
  }

  js_value_t *data_value;

  if (!event->data.empty()) {
    js_typedarray_t<> view;
    err = js_create_typedarray(env, event->data, view);
    assert(err == 0);

    data_value = static_cast<js_value_t *>(view);
  } else {
    err = js_get_null(env, &data_value);
    assert(err == 0);
  }

  js_value_t *error;

  if (!event->error.empty()) {
    error = js_marshall_untyped_value(env, event->error);
  } else {
    err = js_get_null(env, &error);
    assert(err == 0);
  }

  err = js_call_function(env, function, js_receiver_t(receiver), js_handle_t(characteristic_value), event->uuid, js_handle_t(data_value), js_handle_t(error));
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_write(
  js_env_t *env,
  js_function_t<void, js_receiver_t, js_handle_t, std::string, js_handle_t> function,
  bare_bluetooth_android_peripheral_t *context,
  bare_bluetooth_android_peripheral_write_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_write_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  if (peripheral->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto *characteristic = bare_bluetooth_android_peripheral_find_characteristic_handle(jenv, peripheral, event->service_uuid, event->uuid, event->instance_id);

  js_value_t *characteristic_value;

  if (characteristic) {
    js_external_t<bare_bluetooth_android_characteristic_handle_t> ext;
    err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_characteristic_handle_t>>(env, characteristic, ext);
    assert(err == 0);
    characteristic_value = static_cast<js_value_t *>(ext);
  } else {
    err = js_get_null(env, &characteristic_value);
    assert(err == 0);
    if (event->error.empty()) event->error = "Characteristic not found in cache";
  }

  js_value_t *error;

  if (!event->error.empty()) {
    error = js_marshall_untyped_value(env, event->error);
  } else {
    err = js_get_null(env, &error);
    assert(err == 0);
  }

  err = js_call_function(env, function, js_receiver_t(receiver), js_handle_t(characteristic_value), event->uuid, js_handle_t(error));
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_notify(
  js_env_t *env,
  js_function_t<void, js_receiver_t, std::string, js_handle_t, js_handle_t> function,
  bare_bluetooth_android_peripheral_t *context,
  bare_bluetooth_android_peripheral_notify_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_notify_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  if (peripheral->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  auto key = bare_bluetooth_android_create_characteristic_key(event->service_uuid, event->uuid, event->instance_id);

  js_value_t *data_value;

  if (!event->data.empty()) {
    js_typedarray_t<> view;
    err = js_create_typedarray(env, event->data, view);
    assert(err == 0);

    data_value = static_cast<js_value_t *>(view);
  } else {
    err = js_get_null(env, &data_value);
    assert(err == 0);
  }

  js_value_t *error;

  if (!event->error.empty()) {
    error = js_marshall_untyped_value(env, event->error);
  } else {
    err = js_get_null(env, &error);
    assert(err == 0);
  }

  err = js_call_function(env, function, js_receiver_t(receiver), key, js_handle_t(data_value), js_handle_t(error));
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_notify_state(
  js_env_t *env,
  js_function_t<void, js_receiver_t, js_handle_t, std::string, bool, js_handle_t> function,
  bare_bluetooth_android_peripheral_t *context,
  bare_bluetooth_android_peripheral_notify_state_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_notify_state_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  if (peripheral->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto *characteristic = bare_bluetooth_android_peripheral_find_characteristic_handle(jenv, peripheral, event->service_uuid, event->uuid, event->instance_id);

  js_value_t *characteristic_value;

  if (characteristic) {
    js_external_t<bare_bluetooth_android_characteristic_handle_t> ext;
    err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_characteristic_handle_t>>(env, characteristic, ext);
    assert(err == 0);
    characteristic_value = static_cast<js_value_t *>(ext);
  } else {
    err = js_get_null(env, &characteristic_value);
    assert(err == 0);
    if (event->error.empty()) event->error = "Characteristic not found in cache";
  }

  js_value_t *error;

  if (!event->error.empty()) {
    error = js_marshall_untyped_value(env, event->error);
  } else {
    err = js_get_null(env, &error);
    assert(err == 0);
  }

  err = js_call_function(env, function, js_receiver_t(receiver), js_handle_t(characteristic_value), event->uuid, event->is_notifying, js_handle_t(error));
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral_emit_channel_open(
  js_env_t *env,
  bare_bluetooth_android_peripheral_t *peripheral,
  bare_bluetooth_android_socket_handle_t *channel,
  const std::string &error,
  uint32_t psm
) {
  int err;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver_value;
  err = js_get_reference_value(env, peripheral->ctx, &receiver_value);
  assert(err == 0);

  js_receiver_t receiver(receiver_value);

  js_function_t<void, js_receiver_t, js_handle_t, js_handle_t, uint32_t> function;
  err = js_get_reference_value(env, peripheral->on_channel_open, function);
  assert(err == 0);

  js_handle_t channel_arg;

  if (channel) {
    js_external_t<bare_bluetooth_android_socket_handle_t> ext;
    err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_socket_handle_t>>(env, channel, ext);
    assert(err == 0);
    channel_arg = js_handle_t(static_cast<js_value_t *>(ext));
  } else {
    js_value_t *null;
    err = js_get_null(env, &null);
    assert(err == 0);
    channel_arg = js_handle_t(null);
  }

  js_handle_t error_arg;

  if (!error.empty()) {
    error_arg = js_handle_t(js_marshall_untyped_value(env, error));
  } else {
    js_value_t *null;
    err = js_get_null(env, &null);
    assert(err == 0);
    error_arg = js_handle_t(null);
  }

  err = js_call_function(env, function, receiver, channel_arg, error_arg, psm);
  assert(err == 0);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_mtu_changed(
  js_env_t *env,
  js_function_t<void, js_receiver_t, int32_t, js_handle_t> function,
  bare_bluetooth_android_peripheral_t *context,
  bare_bluetooth_android_peripheral_mtu_changed_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_mtu_changed_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  if (peripheral->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *error;

  if (!event->error.empty()) {
    error = js_marshall_untyped_value(env, event->error);
  } else {
    err = js_get_null(env, &error);
    assert(err == 0);
  }

  err = js_call_function(env, function, js_receiver_t(receiver), event->mtu, js_handle_t(error));
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_teardown(js_deferred_teardown_t *handle, void *data);

static js_external_t<bare_bluetooth_android_peripheral_t>
bare_bluetooth_android_peripheral_init(
  js_env_t *env,
  bare_bluetooth_android_gatt_handle_t *gatt_handle,
  js_object_t ctx,
  js_function_t<void, js_receiver_t, uint32_t, js_handle_t> on_services_discover,
  js_function_t<void, js_receiver_t, js_handle_t, uint32_t, js_handle_t> on_characteristics_discover,
  js_function_t<void, js_receiver_t, js_handle_t, std::string, js_handle_t, js_handle_t> on_read,
  js_function_t<void, js_receiver_t, js_handle_t, std::string, js_handle_t> on_write,
  js_function_t<void, js_receiver_t, std::string, js_handle_t, js_handle_t> on_notify,
  js_function_t<void, js_receiver_t, js_handle_t, std::string, bool, js_handle_t> on_notify_state,
  js_function_t<void, js_receiver_t, js_handle_t, js_handle_t, uint32_t> on_channel_open,
  js_function_t<void, js_receiver_t, int32_t, js_handle_t> on_mtu_changed
) {
  int err;

  auto *peripheral = new bare_bluetooth_android_peripheral_t();
  peripheral->env = env;
  peripheral->destroyed = false;
  peripheral->exiting = false;
  peripheral->released = false;

  peripheral->l2cap_connecting = false;
  peripheral->l2cap_open = nullptr;

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  peripheral->gatt = java_global_ref_t<j_bluetooth_gatt_t>(jenv, gatt_handle->handle);

  auto gatt_obj = j_bluetooth_gatt_t(jenv, peripheral->gatt);
  auto get_device = gatt_obj.get_class().get_method<j_bluetooth_device_t()>("getDevice");
  auto device_local = get_device(gatt_obj);
  peripheral->device = java_global_ref_t<j_bluetooth_device_t>(jenv, device_local);

  {
    std::lock_guard<std::mutex> lock(bare_bluetooth_android_peripherals_mutex);
    peripheral->id = bare_bluetooth_android_next_peripheral_id++;
    bare_bluetooth_android_peripherals.emplace(peripheral->id, peripheral);
  }

  {
    auto gatt_callback = j_hp_gatt_callback_t(jenv, gatt_handle->gatt_callback);
    auto set_peripheral_id = gatt_callback.get_class().get_method<void(long)>("setPeripheralId");
    set_peripheral_id(gatt_callback, static_cast<long>(peripheral->id));
  }

  err = js_create_reference(env, static_cast<js_value_t *>(ctx), 1, &peripheral->ctx);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_peripheral__on_services_discover, bare_bluetooth_android_peripheral_t, bare_bluetooth_android_peripheral_services_discover_t>(env, on_services_discover, 0, 1, peripheral, peripheral->tsfn_services_discover);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_peripheral__on_characteristics_discover, bare_bluetooth_android_peripheral_t, bare_bluetooth_android_peripheral_characteristics_discover_t>(env, on_characteristics_discover, 0, 1, peripheral, peripheral->tsfn_characteristics_discover);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_peripheral__on_read, bare_bluetooth_android_peripheral_t, bare_bluetooth_android_peripheral_read_t>(env, on_read, 0, 1, peripheral, peripheral->tsfn_read);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_peripheral__on_write, bare_bluetooth_android_peripheral_t, bare_bluetooth_android_peripheral_write_t>(env, on_write, 0, 1, peripheral, peripheral->tsfn_write);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_peripheral__on_notify, bare_bluetooth_android_peripheral_t, bare_bluetooth_android_peripheral_notify_t>(env, on_notify, 0, 1, peripheral, peripheral->tsfn_notify);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_peripheral__on_notify_state, bare_bluetooth_android_peripheral_t, bare_bluetooth_android_peripheral_notify_state_t>(env, on_notify_state, 0, 1, peripheral, peripheral->tsfn_notify_state);
  assert(err == 0);

  err = js_create_reference(env, on_channel_open, peripheral->on_channel_open);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_peripheral__on_mtu_changed, bare_bluetooth_android_peripheral_t, bare_bluetooth_android_peripheral_mtu_changed_t>(env, on_mtu_changed, 0, 1, peripheral, peripheral->tsfn_mtu_changed);
  assert(err == 0);

  err = js_add_deferred_teardown_callback(env, bare_bluetooth_android_peripheral__on_teardown, (void *) peripheral, &peripheral->teardown);
  assert(err == 0);

  js_external_t<bare_bluetooth_android_peripheral_t> handle;
  err = js_create_external(env, peripheral, handle);
  assert(err == 0);

  return handle;
}

static std::string
bare_bluetooth_android_peripheral_id(js_env_t *env, bare_bluetooth_android_peripheral_t *peripheral) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto device = j_bluetooth_device_t(jenv, peripheral->device);
  auto get_address = device.get_class().get_method<std::string()>("getAddress");
  return get_address(device);
}

static std::optional<std::string>
bare_bluetooth_android_peripheral_name(js_env_t *env, bare_bluetooth_android_peripheral_t *peripheral) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto device = j_bluetooth_device_t(jenv, peripheral->device);
  auto get_name = device.get_class().get_method<j_string_t()>("getName");
  auto name_obj = get_name(device);

  if (static_cast<jobject>(name_obj) == nullptr) return std::nullopt;

  auto name = java_string_t(jenv, name_obj);
  return std::string(name);
}

static bool
bare_bluetooth_android_peripheral_discover_services(js_env_t *env, bare_bluetooth_android_peripheral_t *peripheral) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt = j_bluetooth_gatt_t(jenv, peripheral->gatt);
  auto discover = gatt.get_class().get_method<bool()>("discoverServices");
  return discover(gatt);
}

static void
bare_bluetooth_android_peripheral_discover_characteristics(
  js_env_t *env,
  bare_bluetooth_android_peripheral_t *peripheral,
  bare_bluetooth_android_service_handle_t *service_handle
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  int service_index = -1;
  for (size_t i = 0; i < peripheral->services.size(); i++) {
    if (static_cast<JNIEnv *>(jenv)->IsSameObject(peripheral->services[i], service_handle->handle)) {
      service_index = static_cast<int>(i);
      break;
    }
  }

  auto *event = new bare_bluetooth_android_peripheral_characteristics_discover_t();
  event->service_index = service_index;

  if (service_index >= 0 && static_cast<size_t>(service_index) < peripheral->service_characteristics.size()) {
    event->count = static_cast<uint32_t>(peripheral->service_characteristics[service_index].size());
    event->error = {};
  } else {
    event->count = 0;
    event->error = "Service not found in cache";
  }

  js_call_threadsafe_function(peripheral->tsfn_characteristics_discover, event);
}

static bool
bare_bluetooth_android_peripheral_read(
  js_env_t *env,
  bare_bluetooth_android_peripheral_t *peripheral,
  bare_bluetooth_android_characteristic_handle_t *char_handle
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt = j_bluetooth_gatt_t(jenv, peripheral->gatt);
  auto characteristic = j_bluetooth_gatt_characteristic_t(jenv, char_handle->handle);
  auto read_characteristic = gatt.get_class().get_method<bool(j_bluetooth_gatt_characteristic_t)>("readCharacteristic");
  return read_characteristic(gatt, characteristic);
}

static bool
bare_bluetooth_android_peripheral_write(
  js_env_t *env,
  bare_bluetooth_android_peripheral_t *peripheral,
  bare_bluetooth_android_characteristic_handle_t *char_handle,
  js_typedarray_span_t<uint8_t> data,
  bool with_response
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt = j_bluetooth_gatt_t(jenv, peripheral->gatt);
  auto characteristic = j_bluetooth_gatt_characteristic_t(jenv, char_handle->handle);

  auto set_write_type = characteristic.get_class().get_method<void(int)>("setWriteType");
  set_write_type(characteristic, with_response ? 2 : 1);

  auto byte_array = bare_bluetooth_android_make_byte_array(jenv, data.data(), data.size());
  auto set_value = characteristic.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
  set_value(characteristic, byte_array);

  auto write_characteristic = gatt.get_class().get_method<bool(j_bluetooth_gatt_characteristic_t)>("writeCharacteristic");
  return write_characteristic(gatt, characteristic);
}

static bool
bare_bluetooth_android_peripheral_subscribe(
  js_env_t *env,
  bare_bluetooth_android_peripheral_t *peripheral,
  bare_bluetooth_android_characteristic_handle_t *char_handle
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt = j_bluetooth_gatt_t(jenv, peripheral->gatt);
  auto characteristic = j_bluetooth_gatt_characteristic_t(jenv, char_handle->handle);

  auto set_notify = gatt.get_class().get_method<bool(j_bluetooth_gatt_characteristic_t, bool)>("setCharacteristicNotification");
  bool ok = set_notify(gatt, characteristic, true);

  auto uuid_class = java_class_t<"java/util/UUID">(jenv);
  auto from_string = uuid_class.get_static_method<j_uuid_t(std::string)>("fromString");
  auto cccd_uuid = from_string(std::string("00002902-0000-1000-8000-00805f9b34fb"));

  auto get_descriptor = characteristic.get_class().get_method<j_bluetooth_gatt_descriptor_t(j_uuid_t)>("getDescriptor");
  auto descriptor = get_descriptor(characteristic, cccd_uuid);

  if (static_cast<jobject>(descriptor) != nullptr) {
    jbyte enable_bytes[] = {0x01, 0x00};
    auto enable_value = bare_bluetooth_android_make_byte_array(jenv, enable_bytes, 2);

    auto set_descriptor_value = descriptor.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
    set_descriptor_value(descriptor, enable_value);

    auto write_descriptor = gatt.get_class().get_method<bool(j_bluetooth_gatt_descriptor_t)>("writeDescriptor");
    ok = write_descriptor(gatt, descriptor) && ok;
  } else {
    ok = false;
  }

  return ok;
}

static bool
bare_bluetooth_android_peripheral_unsubscribe(
  js_env_t *env,
  bare_bluetooth_android_peripheral_t *peripheral,
  bare_bluetooth_android_characteristic_handle_t *char_handle
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt = j_bluetooth_gatt_t(jenv, peripheral->gatt);
  auto characteristic = j_bluetooth_gatt_characteristic_t(jenv, char_handle->handle);

  auto set_notify = gatt.get_class().get_method<bool(j_bluetooth_gatt_characteristic_t, bool)>("setCharacteristicNotification");
  bool ok = set_notify(gatt, characteristic, false);

  auto uuid_class = java_class_t<"java/util/UUID">(jenv);
  auto from_string = uuid_class.get_static_method<j_uuid_t(std::string)>("fromString");
  auto cccd_uuid = from_string(std::string("00002902-0000-1000-8000-00805f9b34fb"));

  auto get_descriptor = characteristic.get_class().get_method<j_bluetooth_gatt_descriptor_t(j_uuid_t)>("getDescriptor");
  auto descriptor = get_descriptor(characteristic, cccd_uuid);

  if (static_cast<jobject>(descriptor) != nullptr) {
    jbyte disable_bytes[] = {0x00, 0x00};
    auto disable_value = bare_bluetooth_android_make_byte_array(jenv, disable_bytes, 2);

    auto set_descriptor_value = descriptor.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
    set_descriptor_value(descriptor, disable_value);

    auto write_descriptor = gatt.get_class().get_method<bool(j_bluetooth_gatt_descriptor_t)>("writeDescriptor");
    ok = write_descriptor(gatt, descriptor) && ok;
  } else {
    ok = false;
  }

  return ok;
}

static bool
bare_bluetooth_android_peripheral_request_mtu(
  js_env_t *env,
  bare_bluetooth_android_peripheral_t *peripheral,
  int32_t mtu
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt = j_bluetooth_gatt_t(jenv, peripheral->gatt);
  auto request_mtu = gatt.get_class().get_method<bool(int)>("requestMtu");
  return request_mtu(gatt, mtu);
}

struct bare_bluetooth_android_peripheral_l2cap_open_req_t {
  // Stable id handed to the Java connector, resolved against the registry.
  uint64_t id;
  bare_bluetooth_android_peripheral_t *peripheral;
  bare_bluetooth_android_socket_handle_t *channel;
  java_global_ref_t<j_hp_l2cap_connector_t> connector;
  js_threadsafe_function_t *tsfn_complete;
  std::string error;
  uint32_t psm;
  bool completed;
};

typedef struct {
  bare_bluetooth_android_peripheral_l2cap_open_req_t *req;
  bool success;
  uint32_t psm;
  std::string error;
} bare_bluetooth_android_peripheral_l2cap_open_complete_t;

// Registry of in-flight l2cap open requests. The connector callback holds the
// mutex while dispatching and the request is erased under it when completed, so
// it is never freed mid-use.
static std::mutex bare_bluetooth_android_l2cap_reqs_mutex;
static std::unordered_map<uint64_t, bare_bluetooth_android_peripheral_l2cap_open_req_t *> bare_bluetooth_android_l2cap_reqs;
static uint64_t bare_bluetooth_android_next_l2cap_req_id = 1;

static bare_bluetooth_android_peripheral_l2cap_open_req_t *
bare_bluetooth_android_find_l2cap_req(uint64_t id) {
  auto it = bare_bluetooth_android_l2cap_reqs.find(id);
  return it == bare_bluetooth_android_l2cap_reqs.end() ? nullptr : it->second;
}

static void
bare_bluetooth_android_peripheral_close_socket_handle(
  JNIEnv *env,
  bare_bluetooth_android_socket_handle_t *channel
) {
  auto socket = j_bluetooth_socket_t(env, channel->handle);
  auto close = socket.get_class().get_method<void()>("close");

  close(socket);
  bare_bluetooth_android_check_exception(env);
}

static void
bare_bluetooth_android_peripheral__on_l2cap_open_complete(
  js_env_t *env,
  js_function_t<void>,
  bare_bluetooth_android_peripheral_l2cap_open_req_t *,
  bare_bluetooth_android_peripheral_l2cap_open_complete_t *data
) {
  auto *event = static_cast<bare_bluetooth_android_peripheral_l2cap_open_complete_t *>(data);
  auto *req = event->req;
  auto *peripheral = req->peripheral;
  auto *channel = req->channel;

  if (req->completed) {
    delete event;
    return;
  }

  req->completed = true;

  {
    std::lock_guard<std::mutex> lock(bare_bluetooth_android_l2cap_reqs_mutex);
    bare_bluetooth_android_l2cap_reqs.erase(req->id);
  }

  peripheral->l2cap_connecting = false;
  peripheral->l2cap_open = nullptr;

  if (!event->success && event->error.empty()) {
    event->error = "L2CAP connect failed";
  }

  if (peripheral->destroyed) {
    if (channel) {
      auto jenv = bare_bluetooth_android_jvm().get_env().value();
      bare_bluetooth_android_peripheral_close_socket_handle(jenv, channel);
      delete channel;
    }

    js_release_threadsafe_function(req->tsfn_complete, js_threadsafe_function_abort);
    delete req;
    delete event;

    int err = js_finish_deferred_teardown_callback(peripheral->teardown);
    assert(err == 0);

    delete peripheral;

    return;
  }

  if (event->success && channel) {
    bare_bluetooth_android_peripheral_emit_channel_open(env, peripheral, channel, {}, event->psm);
    req->channel = nullptr;
  } else {
    bare_bluetooth_android_peripheral_emit_channel_open(env, peripheral, nullptr, event->error, event->psm);

    if (channel) {
      auto jenv = bare_bluetooth_android_jvm().get_env().value();
      bare_bluetooth_android_peripheral_close_socket_handle(jenv, channel);
      delete channel;
    }
  }

  js_release_threadsafe_function(req->tsfn_complete, js_threadsafe_function_abort);
  delete req;
  delete event;
}

static void
bare_bluetooth_android_on_l2cap_connector_complete(
  java_env_t,
  j_hp_l2cap_connector_t,
  long native_ptr,
  int psm,
  bool success,
  std::string error
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_l2cap_reqs_mutex);
  auto *req = bare_bluetooth_android_find_l2cap_req(static_cast<uint64_t>(native_ptr));
  if (req == nullptr) return;

  auto *event = new bare_bluetooth_android_peripheral_l2cap_open_complete_t();
  event->req = req;
  event->success = success;
  event->psm = static_cast<uint32_t>(psm);
  event->error = error;

  js_call_threadsafe_function(req->tsfn_complete, event);
}

static void
bare_bluetooth_android_peripheral_release(bare_bluetooth_android_peripheral_t *peripheral);

static void
bare_bluetooth_android_peripheral_open_l2cap_channel(
  js_env_t *env,
  bare_bluetooth_android_peripheral_t *peripheral,
  uint32_t psm
) {
  int err;

  if (peripheral->destroyed) return;

  if (peripheral->l2cap_connecting) {
    bare_bluetooth_android_peripheral_emit_channel_open(env, peripheral, nullptr, "L2CAP open already pending", psm);
    return;
  }

  auto *req = new bare_bluetooth_android_peripheral_l2cap_open_req_t();
  req->peripheral = peripheral;
  req->channel = nullptr;
  req->psm = psm;
  req->completed = false;

  {
    std::lock_guard<std::mutex> lock(bare_bluetooth_android_l2cap_reqs_mutex);
    req->id = bare_bluetooth_android_next_l2cap_req_id++;
    bare_bluetooth_android_l2cap_reqs.emplace(req->id, req);
  }

  js_value_t *noop;
  err = js_create_function(env, "noop", -1, [](js_env_t *, js_callback_info_t *) -> js_value_t * { return NULL; }, NULL, &noop);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_peripheral__on_l2cap_open_complete, bare_bluetooth_android_peripheral_l2cap_open_req_t, bare_bluetooth_android_peripheral_l2cap_open_complete_t>(env, js_function_t<void>(noop), 0, 1, req, req->tsfn_complete);
  assert(err == 0);

  peripheral->l2cap_connecting = true;
  peripheral->l2cap_open = req;

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto device = j_bluetooth_device_t(jenv, peripheral->device);
  auto create_channel = device.get_class().get_method<j_bluetooth_socket_t(int)>("createInsecureL2capChannel");
  auto socket = create_channel(device, static_cast<int>(psm));

  if (bare_bluetooth_android_check_exception(jenv) || static_cast<jobject>(socket) == nullptr) {
    req->error = "Failed to create L2CAP channel";

    auto *event = new bare_bluetooth_android_peripheral_l2cap_open_complete_t();
    event->req = req;
    event->success = false;
    event->psm = psm;
    event->error = req->error;
    js_call_threadsafe_function(req->tsfn_complete, event);
    return;
  }

  req->channel = new bare_bluetooth_android_socket_handle_t{
    java_global_ref_t<j_bluetooth_socket_t>(jenv, socket)
  };

  auto connector_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/L2capConnector">();
  auto connector = connector_class(socket, static_cast<long>(req->id), static_cast<int>(psm));
  req->connector = java_global_ref_t<j_hp_l2cap_connector_t>(jenv, connector);

  auto start = connector.get_class().get_method<void()>("start");
  start(connector);
}

static void
bare_bluetooth_android_peripheral_release(bare_bluetooth_android_peripheral_t *peripheral) {
  int err;

  if (peripheral->released) return;
  peripheral->released = true;

  {
    std::lock_guard<std::mutex> lock(bare_bluetooth_android_peripherals_mutex);
    bare_bluetooth_android_peripherals.erase(peripheral->id);
  }

  if (peripheral->l2cap_open) {
    auto jenv = bare_bluetooth_android_jvm().get_env().value();
    if (static_cast<jobject>(peripheral->l2cap_open->connector) != nullptr) {
      auto connector = j_hp_l2cap_connector_t(jenv, peripheral->l2cap_open->connector);
      auto cancel = connector.get_class().get_method<void()>("cancel");
      cancel(connector);
      static_cast<JNIEnv *>(jenv)->ExceptionClear();
    }
  }

  err = js_delete_reference(peripheral->env, peripheral->ctx);
  assert(err == 0);
  peripheral->ctx = nullptr;

  peripheral->on_channel_open.reset();

  js_release_threadsafe_function(peripheral->tsfn_mtu_changed, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_notify_state, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_notify, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_write, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_read, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_characteristics_discover, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_services_discover, js_threadsafe_function_abort);
}

static void
bare_bluetooth_android_peripheral_destroy(js_env_t *env, bare_bluetooth_android_peripheral_t *peripheral) {
  bool expected = false;
  if (!peripheral->destroyed.compare_exchange_strong(expected, true)) return;

  bare_bluetooth_android_peripheral_release(peripheral);

  if (peripheral->l2cap_connecting) return;

  int err = js_finish_deferred_teardown_callback(peripheral->teardown);
  assert(err == 0);

  delete peripheral;
}

static void
bare_bluetooth_android_peripheral__on_teardown(js_deferred_teardown_t *handle, void *data) {
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(data);

  bool expected = false;
  if (!peripheral->destroyed.compare_exchange_strong(expected, true)) return;

  {
    std::lock_guard<std::mutex> lock(bare_bluetooth_android_peripherals_mutex);
    peripheral->exiting = true;
  }

  bare_bluetooth_android_peripheral_release(peripheral);

  if (peripheral->l2cap_connecting) return;

  int err = js_finish_deferred_teardown_callback(peripheral->teardown);
  assert(err == 0);

  delete peripheral;
}

static uint32_t
bare_bluetooth_android_peripheral_service_count(js_env_t *env, bare_bluetooth_android_peripheral_t *peripheral) {
  return static_cast<uint32_t>(peripheral->services.size());
}

static js_external_t<bare_bluetooth_android_service_handle_t>
bare_bluetooth_android_peripheral_service_at_index(
  js_env_t *env,
  bare_bluetooth_android_peripheral_t *peripheral,
  uint32_t index
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto *service_handle = new bare_bluetooth_android_service_handle_t{java_global_ref_t<j_bluetooth_gatt_service_t>(jenv, peripheral->services[index])};

  js_external_t<bare_bluetooth_android_service_handle_t> result;
  int err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_service_handle_t>>(env, service_handle, result);
  assert(err == 0);

  return result;
}

static std::string
bare_bluetooth_android_service_key(js_env_t *env, bare_bluetooth_android_service_handle_t *service_handle) {
  char key[32];
  snprintf(key, sizeof(key), "%p", static_cast<void *>(service_handle));

  return std::string(key);
}

static std::string
bare_bluetooth_android_service_uuid(js_env_t *env, bare_bluetooth_android_service_handle_t *service_handle) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto service = j_bluetooth_gatt_service_t(jenv, service_handle->handle);
  auto get_uuid = service.get_class().get_method<j_uuid_t()>("getUuid");
  auto uuid_obj = get_uuid(service);
  auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
  return to_string(uuid_obj);
}

static uint32_t
bare_bluetooth_android_service_characteristic_count(js_env_t *env, bare_bluetooth_android_service_handle_t *service_handle) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto service = j_bluetooth_gatt_service_t(jenv, service_handle->handle);
  auto get_characteristics = service.get_class().get_method<j_list_t()>("getCharacteristics");
  auto list = get_characteristics(service);
  auto list_size = list.get_class().get_method<int()>("size");
  return static_cast<uint32_t>(list_size(list));
}

static js_external_t<bare_bluetooth_android_characteristic_handle_t>
bare_bluetooth_android_service_characteristic_at_index(
  js_env_t *env,
  bare_bluetooth_android_service_handle_t *service_handle,
  uint32_t index
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto service = j_bluetooth_gatt_service_t(jenv, service_handle->handle);
  auto get_characteristics = service.get_class().get_method<j_list_t()>("getCharacteristics");
  auto list = get_characteristics(service);
  auto list_get = list.get_class().get_method<j_object_t(int)>("get");
  auto char_obj = list_get(list, static_cast<int>(index));

  auto *char_handle = new bare_bluetooth_android_characteristic_handle_t{java_global_ref_t<j_bluetooth_gatt_characteristic_t>(jenv, char_obj)};

  js_external_t<bare_bluetooth_android_characteristic_handle_t> result;
  int err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_characteristic_handle_t>>(env, char_handle, result);
  assert(err == 0);

  return result;
}

static std::string
bare_bluetooth_android_characteristic_key(js_env_t *env, bare_bluetooth_android_characteristic_handle_t *char_handle) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto characteristic = j_bluetooth_gatt_characteristic_t(jenv, char_handle->handle);
  return bare_bluetooth_android_get_characteristic_key(jenv, characteristic);
}

static std::string
bare_bluetooth_android_characteristic_uuid(js_env_t *env, bare_bluetooth_android_characteristic_handle_t *char_handle) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto characteristic = j_bluetooth_gatt_characteristic_t(jenv, char_handle->handle);
  auto get_uuid = characteristic.get_class().get_method<j_uuid_t()>("getUuid");
  auto uuid_obj = get_uuid(characteristic);
  auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
  return to_string(uuid_obj);
}

static int32_t
bare_bluetooth_android_characteristic_properties(js_env_t *env, bare_bluetooth_android_characteristic_handle_t *char_handle) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto characteristic = j_bluetooth_gatt_characteristic_t(jenv, char_handle->handle);
  auto get_properties = characteristic.get_class().get_method<int()>("getProperties");
  return get_properties(characteristic);
}

static void
bare_bluetooth_android_on_services_discovered(
  java_env_t env,
  j_hp_gatt_callback_t self,
  long native_ptr,
  j_bluetooth_gatt_t gatt,
  int status
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_peripherals_mutex);
  auto *peripheral = bare_bluetooth_android_find_peripheral(static_cast<uint64_t>(native_ptr));
  if (!peripheral || peripheral->exiting) return;

  if (status != 0) {
    auto *event = new bare_bluetooth_android_peripheral_services_discover_t();
    event->count = 0;
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = error_buf;

    js_call_threadsafe_function(peripheral->tsfn_services_discover, event);
    return;
  }

  auto *event = new bare_bluetooth_android_peripheral_services_discover_t();
  event->count = 0;
  event->error = {};

  js_call_threadsafe_function(peripheral->tsfn_services_discover, event);
}

static void
bare_bluetooth_android_on_characteristic_read(
  java_env_t env,
  j_hp_gatt_callback_t self,
  long native_ptr,
  j_bluetooth_gatt_t gatt,
  j_bluetooth_gatt_characteristic_t characteristic,
  java_array_t<unsigned char> value,
  int status
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_peripherals_mutex);
  auto *peripheral = bare_bluetooth_android_find_peripheral(static_cast<uint64_t>(native_ptr));
  if (!peripheral || peripheral->exiting) return;

  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);
  auto service_uuid = bare_bluetooth_android_get_characteristic_service_uuid(env, characteristic);
  auto instance_id = bare_bluetooth_android_get_characteristic_instance_id(env, characteristic);

  auto *event = new bare_bluetooth_android_peripheral_read_t();
  event->service_uuid = service_uuid;
  event->uuid = uuid_str;
  event->instance_id = instance_id;

  if (status == 0 && static_cast<jobject>(value) != nullptr) {
    event->data = value.slice();
  }

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = error_buf;
  } else {
    event->error = {};
  }

  js_call_threadsafe_function(peripheral->tsfn_read, event);
}

static void
bare_bluetooth_android_on_characteristic_write(
  java_env_t env,
  j_hp_gatt_callback_t self,
  long native_ptr,
  j_bluetooth_gatt_t gatt,
  j_bluetooth_gatt_characteristic_t characteristic,
  int status
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_peripherals_mutex);
  auto *peripheral = bare_bluetooth_android_find_peripheral(static_cast<uint64_t>(native_ptr));
  if (!peripheral || peripheral->exiting) return;

  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);
  auto service_uuid = bare_bluetooth_android_get_characteristic_service_uuid(env, characteristic);
  auto instance_id = bare_bluetooth_android_get_characteristic_instance_id(env, characteristic);

  auto *event = new bare_bluetooth_android_peripheral_write_t();
  event->service_uuid = service_uuid;
  event->uuid = uuid_str;
  event->instance_id = instance_id;

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = error_buf;
  } else {
    event->error = {};
  }

  js_call_threadsafe_function(peripheral->tsfn_write, event);
}

static void
bare_bluetooth_android_on_characteristic_changed(
  java_env_t env,
  j_hp_gatt_callback_t self,
  long native_ptr,
  j_bluetooth_gatt_t gatt,
  j_bluetooth_gatt_characteristic_t characteristic,
  java_array_t<unsigned char> value
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_peripherals_mutex);
  auto *peripheral = bare_bluetooth_android_find_peripheral(static_cast<uint64_t>(native_ptr));
  if (!peripheral || peripheral->exiting) return;

  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);
  auto service_uuid = bare_bluetooth_android_get_characteristic_service_uuid(env, characteristic);
  auto instance_id = bare_bluetooth_android_get_characteristic_instance_id(env, characteristic);

  auto *event = new bare_bluetooth_android_peripheral_notify_t();
  event->service_uuid = service_uuid;
  event->uuid = uuid_str;
  event->instance_id = instance_id;
  event->error = {};

  if (static_cast<jobject>(value) != nullptr) {
    event->data = value.slice();
  }

  js_call_threadsafe_function(peripheral->tsfn_notify, event);
}

static void
bare_bluetooth_android_on_descriptor_write(
  java_env_t env,
  j_hp_gatt_callback_t self,
  long native_ptr,
  j_bluetooth_gatt_t gatt,
  j_bluetooth_gatt_descriptor_t descriptor,
  int status
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_peripherals_mutex);
  auto *peripheral = bare_bluetooth_android_find_peripheral(static_cast<uint64_t>(native_ptr));
  if (!peripheral || peripheral->exiting) return;

  auto desc_uuid = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattDescriptor">(env, descriptor);

  bool is_cccd = desc_uuid == "00002902-0000-1000-8000-00805f9b34fb";
  if (!is_cccd) return;

  auto desc_obj = j_bluetooth_gatt_descriptor_t(env, descriptor);
  auto characteristic = desc_obj.get_class().get_method<j_bluetooth_gatt_characteristic_t()>("getCharacteristic")(desc_obj);
  auto char_uuid = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);
  auto service_uuid = bare_bluetooth_android_get_characteristic_service_uuid(env, characteristic);
  auto instance_id = bare_bluetooth_android_get_characteristic_instance_id(env, characteristic);

  auto value = desc_obj.get_class().get_method<java_array_t<unsigned char>()>("getValue")(desc_obj);
  bool is_notifying = false;

  if (static_cast<jarray>(value) != nullptr) {
    if (value.size() >= 1) {
      is_notifying = (value[static_cast<size_t>(0)] != 0);
    }
  }

  auto *event = new bare_bluetooth_android_peripheral_notify_state_t();
  event->service_uuid = service_uuid;
  event->uuid = char_uuid;
  event->instance_id = instance_id;
  event->is_notifying = is_notifying;

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = error_buf;
  } else {
    event->error = {};
  }

  js_call_threadsafe_function(peripheral->tsfn_notify_state, event);
}

static void
bare_bluetooth_android_on_mtu_changed(
  java_env_t env,
  j_hp_gatt_callback_t self,
  long native_ptr,
  j_bluetooth_gatt_t gatt,
  int mtu,
  int status
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_peripherals_mutex);
  auto *peripheral = bare_bluetooth_android_find_peripheral(static_cast<uint64_t>(native_ptr));
  if (!peripheral || peripheral->exiting) return;

  auto *event = new bare_bluetooth_android_peripheral_mtu_changed_t();
  event->mtu = mtu;

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = error_buf;
  } else {
    event->error = {};
  }

  js_call_threadsafe_function(peripheral->tsfn_mtu_changed, event);
}

static void
bare_bluetooth_android_server__on_state_change(
  js_env_t *env,
  js_function_t<void, js_receiver_t, int32_t> function,
  bare_bluetooth_android_server_t *context,
  bare_bluetooth_android_server_state_change_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_state_change_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  if (server->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  err = js_call_function(env, function, js_receiver_t(receiver), event->state);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_add_service(
  js_env_t *env,
  js_function_t<void, js_receiver_t, std::string, js_handle_t> function,
  bare_bluetooth_android_server_t *context,
  bare_bluetooth_android_server_add_service_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_add_service_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  if (server->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *error;

  if (!event->error.empty()) {
    error = js_marshall_untyped_value(env, event->error);
  } else {
    err = js_get_null(env, &error);
    assert(err == 0);
  }

  err = js_call_function(env, function, js_receiver_t(receiver), event->uuid, js_handle_t(error));
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static bare_bluetooth_android_device_handle_t *
bare_bluetooth_android_server_create_device_handle(
  JNIEnv *env,
  bare_bluetooth_android_server_t *server,
  const std::string &address
) {
  auto adapter = j_bluetooth_adapter_t(env, server->adapter);
  auto get_remote_device = adapter.get_class().get_method<j_bluetooth_device_t(std::string)>("getRemoteDevice");
  auto device = get_remote_device(adapter, address);

  return new bare_bluetooth_android_device_handle_t{
    java_global_ref_t<j_bluetooth_device_t>(env, device)
  };
}

static void
bare_bluetooth_android_server__on_read_request(
  js_env_t *env,
  js_function_t<void, js_receiver_t, js_handle_t, int32_t, std::string, int32_t> function,
  bare_bluetooth_android_server_t *context,
  bare_bluetooth_android_server_read_request_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_read_request_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  if (server->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto *device = bare_bluetooth_android_server_create_device_handle(jenv, server, event->device_address);

  js_external_t<bare_bluetooth_android_device_handle_t> ext;
  err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_device_handle_t>>(env, device, ext);
  assert(err == 0);

  err = js_call_function(env, function, js_receiver_t(receiver), js_handle_t(static_cast<js_value_t *>(ext)), event->request_id, event->characteristic_uuid, event->offset);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_write_request(
  js_env_t *env,
  js_function_t<void, js_receiver_t, js_handle_t, int32_t, std::string, int32_t, js_handle_t, bool> function,
  bare_bluetooth_android_server_t *context,
  bare_bluetooth_android_server_write_request_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_write_request_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  if (server->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto *device = bare_bluetooth_android_server_create_device_handle(jenv, server, event->device_address);

  js_external_t<bare_bluetooth_android_device_handle_t> ext;
  err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_device_handle_t>>(env, device, ext);
  assert(err == 0);

  js_value_t *data_value;

  if (!event->data.empty()) {
    js_typedarray_t<> view;
    err = js_create_typedarray(env, event->data, view);
    assert(err == 0);

    data_value = static_cast<js_value_t *>(view);
  } else {
    err = js_get_null(env, &data_value);
    assert(err == 0);
  }

  err = js_call_function(env, function, js_receiver_t(receiver), js_handle_t(static_cast<js_value_t *>(ext)), event->request_id, event->characteristic_uuid, event->offset, js_handle_t(data_value), event->response_needed);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_subscribe(
  js_env_t *env,
  js_function_t<void, js_receiver_t, std::string, std::string> function,
  bare_bluetooth_android_server_t *context,
  bare_bluetooth_android_server_subscribe_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_subscribe_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  if (server->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  server->subscriptions[event->characteristic_uuid].insert(event->device_address);

  err = js_call_function(env, function, js_receiver_t(receiver), event->device_address, event->characteristic_uuid);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_unsubscribe(
  js_env_t *env,
  js_function_t<void, js_receiver_t, std::string, std::string> function,
  bare_bluetooth_android_server_t *context,
  bare_bluetooth_android_server_unsubscribe_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_unsubscribe_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  if (server->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  server->subscriptions[event->characteristic_uuid].erase(event->device_address);

  err = js_call_function(env, function, js_receiver_t(receiver), event->device_address, event->characteristic_uuid);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_advertise_error(
  js_env_t *env,
  js_function_t<void, js_receiver_t, int32_t, std::string> function,
  bare_bluetooth_android_server_t *context,
  bare_bluetooth_android_server_advertise_error_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_advertise_error_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  if (server->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  err = js_call_function(env, function, js_receiver_t(receiver), event->error_code, event->error);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_notify_sent(
  js_env_t *env,
  js_function_t<void, js_receiver_t, std::string, int32_t> function,
  bare_bluetooth_android_server_t *context,
  bare_bluetooth_android_server_notify_sent_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_notify_sent_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  if (server->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  err = js_call_function(env, function, js_receiver_t(receiver), event->device_address, event->status);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_channel_publish(
  js_env_t *env,
  js_function_t<void, js_receiver_t, uint32_t, js_handle_t> function,
  bare_bluetooth_android_server_t *context,
  bare_bluetooth_android_server_channel_publish_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_channel_publish_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  if (server->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *error;

  if (!event->error.empty()) {
    error = js_marshall_untyped_value(env, event->error);
  } else {
    err = js_get_null(env, &error);
    assert(err == 0);
  }

  err = js_call_function(env, function, js_receiver_t(receiver), static_cast<uint32_t>(event->psm), js_handle_t(error));
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_channel_open(
  js_env_t *env,
  js_function_t<void, js_receiver_t, js_handle_t, js_handle_t, uint32_t> function,
  bare_bluetooth_android_server_t *context,
  bare_bluetooth_android_server_channel_open_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_channel_open_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  if (server->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  bare_bluetooth_android_socket_handle_t *channel = nullptr;

  if (event->error.empty() && event->socket_id != 0) {
    auto jenv = bare_bluetooth_android_jvm().get_env().value();
    bare_bluetooth_android_server_t::published_channel_t *published = nullptr;

    for (auto *ch : server->published_channels) {
      if (ch->psm == event->psm) {
        published = ch;
        break;
      }
    }

    if (published) {
      auto acceptor = j_hp_l2cap_acceptor_t(jenv, published->acceptor);
      auto take_socket = acceptor.get_class().get_method<j_bluetooth_socket_t(int)>("takeSocket");
      auto socket = take_socket(acceptor, event->socket_id);

      if (static_cast<jobject>(socket) != nullptr) {
        channel = new bare_bluetooth_android_socket_handle_t{
          java_global_ref_t<j_bluetooth_socket_t>(jenv, socket)
        };
      } else {
        event->error = "Accepted L2CAP socket was not found";
      }
    } else {
      event->error = "L2CAP acceptor was not found";
    }
  }

  js_value_t *channel_value;

  if (channel) {
    js_external_t<bare_bluetooth_android_socket_handle_t> ext;
    err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_socket_handle_t>>(env, channel, ext);
    assert(err == 0);
    channel_value = static_cast<js_value_t *>(ext);
  } else {
    err = js_get_null(env, &channel_value);
    assert(err == 0);
  }

  js_value_t *error;

  if (!event->error.empty()) {
    error = js_marshall_untyped_value(env, event->error);
  } else {
    err = js_get_null(env, &error);
    assert(err == 0);
  }

  err = js_call_function(env, function, js_receiver_t(receiver), js_handle_t(channel_value), js_handle_t(error), static_cast<uint32_t>(event->psm));
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_connection_state(
  js_env_t *env,
  js_function_t<void, js_receiver_t, std::string, int32_t> function,
  bare_bluetooth_android_server_t *context,
  bare_bluetooth_android_server_connection_state_t *data
) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_connection_state_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  if (server->exiting) {
    delete event;
    return;
  }

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  if (event->new_state == 2) {
    server->connected_devices.insert(event->address);
  } else if (event->new_state == 0) {
    server->connected_devices.erase(event->address);

    for (auto &pair : server->subscriptions) {
      pair.second.erase(std::string(event->address));
    }
  }

  err = js_call_function(env, function, js_receiver_t(receiver), event->address, event->new_state);
  assert(err == 0);

  delete event;

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_descriptor_response(
  js_env_t *env,
  js_function_t<void> function,
  bare_bluetooth_android_server_t *context,
  bare_bluetooth_android_server_descriptor_response_t *data
) {
  auto *event = static_cast<bare_bluetooth_android_server_descriptor_response_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  if (server->exiting) {
    delete event;
    return;
  }

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt_server = j_bluetooth_gatt_server_t(jenv, server->gatt_server);
  auto adapter = j_bluetooth_adapter_t(jenv, server->adapter);
  auto get_remote_device = adapter.get_class().get_method<j_bluetooth_device_t(std::string)>("getRemoteDevice");
  auto device = get_remote_device(adapter, event->device_address);

  java_array_t<unsigned char> response_data;

  if (!event->data.empty()) {
    response_data = bare_bluetooth_android_make_byte_array(jenv, event->data.data(), event->data.size());
  }

  auto send_response = gatt_server.get_class().get_method<bool(j_bluetooth_device_t, int, int, int, java_array_t<unsigned char>)>("sendResponse");
  send_response(gatt_server, device, event->request_id, 0, event->offset, response_data);

  delete event;
}

static void
bare_bluetooth_android_server__on_teardown(js_deferred_teardown_t *handle, void *data);

static void
bare_bluetooth_android_server_release(bare_bluetooth_android_server_t *server);

// Registry mapping server ids to live servers. Guarded by its mutex: inbound
// callbacks hold the lock while dispatching, and teardown erases the id under
// the same lock, so a server is never freed while a callback is using it. A
// callback that arrives after teardown is a safe lookup miss.
static std::mutex bare_bluetooth_android_servers_mutex;
static std::unordered_map<uint64_t, bare_bluetooth_android_server_t *> bare_bluetooth_android_servers;
static uint64_t bare_bluetooth_android_next_server_id = 1;

static bare_bluetooth_android_server_t *
bare_bluetooth_android_find_server(uint64_t id) {
  auto it = bare_bluetooth_android_servers.find(id);
  return it == bare_bluetooth_android_servers.end() ? nullptr : it->second;
}

static js_external_t<bare_bluetooth_android_server_t>
bare_bluetooth_android_server_init(
  js_env_t *env,
  js_object_t ctx,
  js_function_t<void, js_receiver_t, int32_t> on_state_change,
  js_function_t<void, js_receiver_t, std::string, js_handle_t> on_add_service,
  js_function_t<void, js_receiver_t, js_handle_t, int32_t, std::string, int32_t> on_read_request,
  js_function_t<void, js_receiver_t, js_handle_t, int32_t, std::string, int32_t, js_handle_t, bool> on_write_request,
  js_function_t<void, js_receiver_t, std::string, std::string> on_subscribe,
  js_function_t<void, js_receiver_t, std::string, std::string> on_unsubscribe,
  js_function_t<void, js_receiver_t, int32_t, std::string> on_advertise_error,
  js_function_t<void, js_receiver_t, uint32_t, js_handle_t> on_channel_publish,
  js_function_t<void, js_receiver_t, js_handle_t, js_handle_t, uint32_t> on_channel_open,
  js_function_t<void, js_receiver_t, std::string, int32_t> on_notify_sent,
  js_function_t<void, js_receiver_t, std::string, int32_t> on_connection_state
) {
  int err;

  auto *server = new bare_bluetooth_android_server_t();
  server->env = env;
  server->destroyed = false;
  server->exiting = false;

  {
    std::lock_guard<std::mutex> lock(bare_bluetooth_android_servers_mutex);
    server->id = bare_bluetooth_android_next_server_id++;
    bare_bluetooth_android_servers.emplace(server->id, server);
  }

  err = js_create_reference(env, static_cast<js_value_t *>(ctx), 1, &server->ctx);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_server__on_state_change, bare_bluetooth_android_server_t, bare_bluetooth_android_server_state_change_t>(env, on_state_change, 0, 1, server, server->tsfn_state_change);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_server__on_add_service, bare_bluetooth_android_server_t, bare_bluetooth_android_server_add_service_t>(env, on_add_service, 0, 1, server, server->tsfn_add_service);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_server__on_read_request, bare_bluetooth_android_server_t, bare_bluetooth_android_server_read_request_t>(env, on_read_request, 0, 1, server, server->tsfn_read_request);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_server__on_write_request, bare_bluetooth_android_server_t, bare_bluetooth_android_server_write_request_t>(env, on_write_request, 0, 1, server, server->tsfn_write_request);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_server__on_subscribe, bare_bluetooth_android_server_t, bare_bluetooth_android_server_subscribe_t>(env, on_subscribe, 0, 1, server, server->tsfn_subscribe);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_server__on_unsubscribe, bare_bluetooth_android_server_t, bare_bluetooth_android_server_unsubscribe_t>(env, on_unsubscribe, 0, 1, server, server->tsfn_unsubscribe);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_server__on_advertise_error, bare_bluetooth_android_server_t, bare_bluetooth_android_server_advertise_error_t>(env, on_advertise_error, 0, 1, server, server->tsfn_advertise_error);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_server__on_channel_publish, bare_bluetooth_android_server_t, bare_bluetooth_android_server_channel_publish_t>(env, on_channel_publish, 0, 1, server, server->tsfn_channel_publish);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_server__on_channel_open, bare_bluetooth_android_server_t, bare_bluetooth_android_server_channel_open_t>(env, on_channel_open, 0, 1, server, server->tsfn_channel_open);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_server__on_notify_sent, bare_bluetooth_android_server_t, bare_bluetooth_android_server_notify_sent_t>(env, on_notify_sent, 0, 1, server, server->tsfn_notify_sent);
  assert(err == 0);

  err = js_create_threadsafe_function<bare_bluetooth_android_server__on_connection_state, bare_bluetooth_android_server_t, bare_bluetooth_android_server_connection_state_t>(env, on_connection_state, 0, 1, server, server->tsfn_server_connection_state);
  assert(err == 0);

  {
    js_value_t *noop;
    err = js_create_function(env, "noop", -1, [](js_env_t *, js_callback_info_t *) -> js_value_t * { return NULL; }, NULL, &noop);
    assert(err == 0);

    err = js_create_threadsafe_function<bare_bluetooth_android_server__on_descriptor_response, bare_bluetooth_android_server_t, bare_bluetooth_android_server_descriptor_response_t>(env, js_function_t<void>(noop), 0, 1, server, server->tsfn_descriptor_response);
    assert(err == 0);
  }

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto activity = bare_bluetooth_android_get_context(jenv);
  auto get_system_service = activity.get_class().get_method<j_object_t(std::string)>("getSystemService");
  auto manager_obj = get_system_service(activity, std::string("bluetooth"));
  auto bt_manager = j_bluetooth_manager_t(jenv, manager_obj);

  auto callback_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/GattServerCallback">();
  auto callback_local = callback_class(static_cast<long>(server->id));

  auto open_gatt_server = bt_manager.get_class().get_method<j_bluetooth_gatt_server_t(j_context_t, j_bluetooth_gatt_server_callback_t)>("openGattServer");
  auto gatt_server_local = open_gatt_server(bt_manager, activity, j_bluetooth_gatt_server_callback_t(jenv, callback_local));
  server->gatt_server = java_global_ref_t<j_bluetooth_gatt_server_t>(jenv, gatt_server_local);

  auto get_adapter = bt_manager.get_class().get_method<j_bluetooth_adapter_t()>("getAdapter");
  auto adapter = get_adapter(bt_manager);
  server->adapter = java_global_ref_t<j_bluetooth_adapter_t>(jenv, adapter);

  auto get_advertiser = adapter.get_class().get_method<j_bluetooth_le_advertiser_t()>("getBluetoothLeAdvertiser");
  auto advertiser_local = get_advertiser(adapter);
  server->advertiser = java_global_ref_t<j_bluetooth_le_advertiser_t>(jenv, advertiser_local);

  auto get_state = adapter.get_class().get_method<int()>("getState");
  int android_state = get_state(adapter);

  auto *state_event = new bare_bluetooth_android_server_state_change_t();
  state_event->state = android_state;
  js_call_threadsafe_function(server->tsfn_state_change, state_event);

  err = js_add_deferred_teardown_callback(env, bare_bluetooth_android_server__on_teardown, (void *) server, &server->teardown);
  assert(err == 0);

  js_external_t<bare_bluetooth_android_server_t> handle;
  err = js_create_external(env, server, handle);
  assert(err == 0);

  return handle;
}

static void
bare_bluetooth_android_server_add_service(
  js_env_t *env,
  bare_bluetooth_android_server_t *server,
  bare_bluetooth_android_service_handle_t *service_handle
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt_server = j_bluetooth_gatt_server_t(jenv, server->gatt_server);
  auto service = j_bluetooth_gatt_service_t(jenv, service_handle->handle);

  auto add_service = gatt_server.get_class().get_method<bool(j_bluetooth_gatt_service_t)>("addService");
  bool result = add_service(gatt_server, service);

  auto get_chars = service.get_class().get_method<j_list_t()>("getCharacteristics");
  auto chars_list = get_chars(service);
  auto list_size = chars_list.get_class().get_method<int()>("size");
  auto list_get = chars_list.get_class().get_method<j_object_t(int)>("get");
  int count = list_size(chars_list);

  for (int i = 0; i < count; i++) {
    auto char_obj = list_get(chars_list, i);
    auto characteristic = j_bluetooth_gatt_characteristic_t(jenv, char_obj);
    auto get_uuid = characteristic.get_class().get_method<j_uuid_t()>("getUuid");
    auto uuid_obj = get_uuid(characteristic);
    auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
    std::string uuid_str = to_string(uuid_obj);

    server->characteristics.emplace(uuid_str, java_global_ref_t<j_bluetooth_gatt_characteristic_t>(jenv, char_obj));
  }

  if (!result) {
    auto get_uuid = service.get_class().get_method<j_uuid_t()>("getUuid");
    auto uuid_obj = get_uuid(service);
    auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
    std::string uuid_str = to_string(uuid_obj);

    auto *event = new bare_bluetooth_android_server_add_service_t();
    event->uuid = uuid_str;
    event->error = "Failed to add service";

    js_call_threadsafe_function(server->tsfn_add_service, event);
  }
}

static void
bare_bluetooth_android_server_start_advertising(
  js_env_t *env,
  bare_bluetooth_android_server_t *server,
  std::optional<std::string> name,
  std::optional<std::vector<bare_bluetooth_android_uuid_handle_t *>> uuids
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto helper_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/AdvertiseHelper">();
  auto helper = helper_class(1, true, name.has_value());

  if (uuids) {
    auto add_uuid = helper.get_class().get_method<void(j_uuid_t)>("addServiceUuid");

    for (auto *uuid_handle : *uuids) {
      add_uuid(helper, j_uuid_t(jenv, uuid_handle->handle));
    }
  }

  auto adv_callback_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/AdvertiseCallback">();
  auto adv_callback = adv_callback_class(static_cast<long>(server->id));
  server->advertise_callback = java_global_ref_t<j_advertise_callback_t>(jenv, adv_callback);

  auto start = helper.get_class().get_method<void(j_bluetooth_le_advertiser_t, j_advertise_callback_t)>("startAdvertising");
  start(helper, j_bluetooth_le_advertiser_t(jenv, server->advertiser), j_advertise_callback_t(jenv, server->advertise_callback));
}

static void
bare_bluetooth_android_server_stop_advertising(js_env_t *env, bare_bluetooth_android_server_t *server) {
  if (static_cast<jobject>(server->advertise_callback)) {
    auto jenv = bare_bluetooth_android_jvm().get_env().value();
    auto advertiser = j_bluetooth_le_advertiser_t(jenv, server->advertiser);
    auto stop = advertiser.get_class().get_method<void(j_advertise_callback_t)>("stopAdvertising");
    stop(advertiser, j_advertise_callback_t(jenv, static_cast<jobject>(server->advertise_callback)));

    server->advertise_callback = {};
  }
}

static void
bare_bluetooth_android_server_respond_to_request(
  js_env_t *env,
  bare_bluetooth_android_server_t *server,
  bare_bluetooth_android_device_handle_t *device_handle,
  int32_t request_id,
  int32_t result_code,
  int32_t offset,
  std::optional<js_typedarray_span_t<uint8_t>> data
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt_server = j_bluetooth_gatt_server_t(jenv, server->gatt_server);
  auto device = j_bluetooth_device_t(jenv, device_handle->handle);

  java_array_t<unsigned char> response_data;

  if (data) {
    response_data = bare_bluetooth_android_make_byte_array(jenv, data->data(), data->size());
  }

  auto send_response = gatt_server.get_class().get_method<bool(j_bluetooth_device_t, int, int, int, java_array_t<unsigned char>)>("sendResponse");
  send_response(gatt_server, device, request_id, result_code, offset, response_data);
}

static bool
bare_bluetooth_android_server_update_value(
  js_env_t *env,
  bare_bluetooth_android_server_t *server,
  bare_bluetooth_android_characteristic_handle_t *char_handle,
  js_typedarray_span_t<uint8_t> data
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto characteristic = j_bluetooth_gatt_characteristic_t(jenv, char_handle->handle);
  auto gatt_server = j_bluetooth_gatt_server_t(jenv, server->gatt_server);

  auto byte_array = bare_bluetooth_android_make_byte_array(jenv, data.data(), data.size());
  auto set_value = characteristic.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
  set_value(characteristic, byte_array);

  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(jenv, char_handle->handle);

  bool all_ok = true;

  auto it = server->subscriptions.find(uuid_str);
  if (it != server->subscriptions.end()) {
    auto notify = gatt_server.get_class().get_method<bool(j_bluetooth_device_t, j_bluetooth_gatt_characteristic_t, bool)>("notifyCharacteristicChanged");

    for (const auto &addr : it->second) {
      if (server->connected_devices.find(addr) != server->connected_devices.end()) {
        auto adapter = j_bluetooth_adapter_t(jenv, server->adapter);
        auto get_remote_device = adapter.get_class().get_method<j_bluetooth_device_t(std::string)>("getRemoteDevice");
        auto device = get_remote_device(adapter, addr);
        bool ok = notify(gatt_server, device, characteristic, false);
        if (!ok) all_ok = false;
      }
    }
  }

  return all_ok;
}

static void
bare_bluetooth_android_on_l2cap_acceptor_accepted(
  java_env_t,
  j_hp_l2cap_acceptor_t,
  long native_ptr,
  int psm,
  int socket_id
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_servers_mutex);
  auto *server = bare_bluetooth_android_find_server(static_cast<uint64_t>(native_ptr));
  if (server == nullptr || server->exiting) return;

  auto *event = new bare_bluetooth_android_server_channel_open_t();
  event->socket_id = socket_id;
  event->error = {};
  event->psm = static_cast<uint16_t>(psm);

  js_call_threadsafe_function(server->tsfn_channel_open, event);
}

static void
bare_bluetooth_android_on_l2cap_acceptor_error(
  java_env_t,
  j_hp_l2cap_acceptor_t,
  long native_ptr,
  int psm,
  std::string error
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_servers_mutex);
  auto *server = bare_bluetooth_android_find_server(static_cast<uint64_t>(native_ptr));
  if (server == nullptr || server->exiting) return;

  auto *event = new bare_bluetooth_android_server_channel_open_t();
  event->socket_id = 0;
  event->error = error;
  event->psm = static_cast<uint16_t>(psm);

  js_call_threadsafe_function(server->tsfn_channel_open, event);
}

static void
bare_bluetooth_android_server_publish_channel(
  js_env_t *env,
  bare_bluetooth_android_server_t *server,
  bool encrypted
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto activity = bare_bluetooth_android_get_context(jenv);
  auto get_system_service = activity.get_class().get_method<j_object_t(std::string)>("getSystemService");
  auto manager_obj = get_system_service(activity, std::string("bluetooth"));
  auto bt_manager = j_bluetooth_manager_t(jenv, manager_obj);
  auto get_adapter = bt_manager.get_class().get_method<j_bluetooth_adapter_t()>("getAdapter");
  auto adapter = get_adapter(bt_manager);

  j_bluetooth_server_socket_t server_socket_local;
  if (encrypted) {
    auto listen = adapter.get_class().get_method<j_bluetooth_server_socket_t()>("listenUsingL2capChannel");
    server_socket_local = listen(adapter);
  } else {
    auto listen = adapter.get_class().get_method<j_bluetooth_server_socket_t()>("listenUsingInsecureL2capChannel");
    server_socket_local = listen(adapter);
  }

  if (bare_bluetooth_android_check_exception(jenv) || static_cast<jobject>(server_socket_local) == nullptr) {
    auto *event = new bare_bluetooth_android_server_channel_publish_t();
    event->psm = 0;
    event->error = "Failed to create L2CAP server socket";
    js_call_threadsafe_function(server->tsfn_channel_publish, event);
    return;
  }

  auto *ch = new bare_bluetooth_android_server_t::published_channel_t();
  ch->server_socket = java_global_ref_t<j_bluetooth_server_socket_t>(jenv, server_socket_local);

  auto get_psm = ch->server_socket.get_class().get_method<int()>("getPsm");
  int psm = get_psm(ch->server_socket);
  ch->psm = static_cast<uint16_t>(psm);

  auto acceptor_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/L2capAcceptor">();
  auto acceptor = acceptor_class(server_socket_local, static_cast<long>(server->id), psm);
  ch->acceptor = java_global_ref_t<j_hp_l2cap_acceptor_t>(jenv, acceptor);

  server->published_channels.push_back(ch);

  auto *event = new bare_bluetooth_android_server_channel_publish_t();
  event->psm = static_cast<uint16_t>(psm);
  event->error = {};
  js_call_threadsafe_function(server->tsfn_channel_publish, event);

  auto start = acceptor.get_class().get_method<void()>("start");
  start(acceptor);
}

static void
bare_bluetooth_android_server_unpublish_channel(
  js_env_t *env,
  bare_bluetooth_android_server_t *server,
  uint32_t psm
) {
  for (auto it = server->published_channels.begin(); it != server->published_channels.end(); ++it) {
    auto *ch = *it;
    if (ch->psm == static_cast<uint16_t>(psm)) {
      auto jenv = bare_bluetooth_android_jvm().get_env().value();
      auto acceptor = j_hp_l2cap_acceptor_t(jenv, ch->acceptor);
      auto stop = acceptor.get_class().get_method<void()>("stop");
      stop(acceptor);
      static_cast<JNIEnv *>(jenv)->ExceptionClear();

      auto close = ch->server_socket.get_class().get_method<void()>("close");
      close(ch->server_socket);
      static_cast<JNIEnv *>(jenv)->ExceptionClear();

      delete ch;
      server->published_channels.erase(it);
      break;
    }
  }
}

static void
bare_bluetooth_android_server_release(bare_bluetooth_android_server_t *server) {
  int err;

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto gatt_server = j_bluetooth_gatt_server_t(jenv, server->gatt_server);
  auto close = gatt_server.get_class().get_method<void()>("close");
  close(gatt_server);

  server->characteristics.clear();
  server->connected_devices.clear();
  server->subscriptions.clear();

  err = js_delete_reference(server->env, server->ctx);
  assert(err == 0);

  for (auto *ch : server->published_channels) {
    delete ch;
  }
  server->published_channels.clear();

  js_release_threadsafe_function(server->tsfn_notify_sent, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_server_connection_state, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_descriptor_response, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_channel_open, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_channel_publish, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_advertise_error, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_unsubscribe, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_subscribe, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_write_request, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_read_request, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_add_service, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_state_change, js_threadsafe_function_abort);
}

static void
bare_bluetooth_android_server_destroy(js_env_t *env, bare_bluetooth_android_server_t *server) {
  bool expected = false;
  if (!server->destroyed.compare_exchange_strong(expected, true)) return;

  // Unregister first: any in-flight callback holds the lock and completes before
  // erase returns; afterwards no callback can resolve the id, so releasing tsfns
  // and freeing is safe and deterministic.
  {
    std::lock_guard<std::mutex> lock(bare_bluetooth_android_servers_mutex);
    bare_bluetooth_android_servers.erase(server->id);
  }

  bare_bluetooth_android_server_release(server);

  int err = js_finish_deferred_teardown_callback(server->teardown);
  assert(err == 0);

  delete server;
}

static void
bare_bluetooth_android_server__on_teardown(js_deferred_teardown_t *handle, void *data) {
  auto *server = static_cast<bare_bluetooth_android_server_t *>(data);

  bool expected = false;
  if (!server->destroyed.compare_exchange_strong(expected, true)) return;

  // Both the set and the erase happen under the lock the callbacks
  // read exiting under, so a callback either sees exiting=true (and bails)
  // or fails the id lookup: never a torn read across threads.
  {
    std::lock_guard<std::mutex> lock(bare_bluetooth_android_servers_mutex);
    server->exiting = true;
    bare_bluetooth_android_servers.erase(server->id);
  }

  bare_bluetooth_android_server_release(server);

  int err = js_finish_deferred_teardown_callback(server->teardown);
  assert(err == 0);

  delete server;
}

static js_external_t<bare_bluetooth_android_service_handle_t>
bare_bluetooth_android_create_mutable_service(
  js_env_t *env,
  bare_bluetooth_android_uuid_handle_t *uuid_handle,
  bool is_primary
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto service_class = java_class_t<"android/bluetooth/BluetoothGattService">(jenv);
  auto service = service_class(j_uuid_t(jenv, uuid_handle->handle), is_primary ? 0 : 1);

  auto *service_handle = new bare_bluetooth_android_service_handle_t{java_global_ref_t<j_bluetooth_gatt_service_t>(jenv, service)};

  js_external_t<bare_bluetooth_android_service_handle_t> handle;
  int err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_service_handle_t>>(env, service_handle, handle);
  assert(err == 0);

  return handle;
}

static js_external_t<bare_bluetooth_android_characteristic_handle_t>
bare_bluetooth_android_create_mutable_characteristic(
  js_env_t *env,
  bare_bluetooth_android_uuid_handle_t *uuid_handle,
  int32_t properties,
  int32_t js_permissions,
  std::optional<js_typedarray_span_t<uint8_t>> value
) {
  int err;

  int32_t android_permissions = 0;
  if (js_permissions & 0x01) android_permissions |= 0x01;
  if (js_permissions & 0x02) android_permissions |= 0x10;
  if (js_permissions & 0x04) android_permissions |= 0x02;
  if (js_permissions & 0x08) android_permissions |= 0x20;

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto char_class = java_class_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv);
  auto characteristic = char_class(j_uuid_t(jenv, uuid_handle->handle), properties, android_permissions);

  if (properties & 0x30) {
    auto uuid_class = java_class_t<"java/util/UUID">(jenv);
    auto from_string = uuid_class.get_static_method<j_uuid_t(std::string)>("fromString");
    auto cccd_uuid = from_string(std::string("00002902-0000-1000-8000-00805f9b34fb"));

    auto desc_class = java_class_t<"android/bluetooth/BluetoothGattDescriptor">(jenv);
    auto cccd = desc_class(cccd_uuid, 0x11);

    auto add_descriptor = characteristic.get_class().get_method<bool(j_bluetooth_gatt_descriptor_t)>("addDescriptor");
    add_descriptor(characteristic, cccd);
  }

  if (value) {
    auto byte_array = bare_bluetooth_android_make_byte_array(jenv, value->data(), value->size());
    auto set_value = characteristic.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
    set_value(characteristic, byte_array);
  }

  auto *char_handle = new bare_bluetooth_android_characteristic_handle_t{java_global_ref_t<j_bluetooth_gatt_characteristic_t>(jenv, characteristic)};

  js_external_t<bare_bluetooth_android_characteristic_handle_t> handle;
  err = js_create_external<bare_bluetooth_android__on_release<bare_bluetooth_android_characteristic_handle_t>>(env, char_handle, handle);
  assert(err == 0);

  return handle;
}

static void
bare_bluetooth_android_service_set_characteristics(
  js_env_t *env,
  bare_bluetooth_android_service_handle_t *service_handle,
  std::vector<bare_bluetooth_android_characteristic_handle_t *> characteristics
) {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto service = j_bluetooth_gatt_service_t(jenv, service_handle->handle);
  auto add_characteristic = service.get_class().get_method<bool(j_bluetooth_gatt_characteristic_t)>("addCharacteristic");

  for (auto *char_handle : characteristics) {
    add_characteristic(service, j_bluetooth_gatt_characteristic_t(jenv, char_handle->handle));
  }
}

static void
bare_bluetooth_android_on_server_connection_state_change(
  java_env_t env,
  j_hp_gatt_server_callback_t self,
  long native_ptr,
  j_bluetooth_device_t device,
  int status,
  int new_state
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_servers_mutex);
  auto *server = bare_bluetooth_android_find_server(static_cast<uint64_t>(native_ptr));
  if (server == nullptr || server->exiting) return;

  auto device_obj = j_bluetooth_device_t(env, device);
  auto address = device_obj.get_class().get_method<std::string()>("getAddress")(device_obj);

  auto *event = new bare_bluetooth_android_server_connection_state_t();
  event->address = address;
  event->status = status;
  event->new_state = new_state;

  js_call_threadsafe_function(server->tsfn_server_connection_state, event);
}

static void
bare_bluetooth_android_on_service_added(
  java_env_t env,
  j_hp_gatt_server_callback_t self,
  long native_ptr,
  int status,
  j_bluetooth_gatt_service_t service
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_servers_mutex);
  auto *server = bare_bluetooth_android_find_server(static_cast<uint64_t>(native_ptr));
  if (server == nullptr || server->exiting) return;

  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattService">(env, service);

  auto *event = new bare_bluetooth_android_server_add_service_t();
  event->uuid = uuid_str;

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = error_buf;
  } else {
    event->error = {};
  }

  js_call_threadsafe_function(server->tsfn_add_service, event);
}

static void
bare_bluetooth_android_on_read_request(
  java_env_t env,
  j_hp_gatt_server_callback_t self,
  long native_ptr,
  j_bluetooth_device_t device,
  int request_id,
  int offset,
  j_bluetooth_gatt_characteristic_t characteristic
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_servers_mutex);
  auto *server = bare_bluetooth_android_find_server(static_cast<uint64_t>(native_ptr));
  if (server == nullptr || server->exiting) return;

  auto device_obj = j_bluetooth_device_t(env, device);
  auto address = device_obj.get_class().get_method<std::string()>("getAddress")(device_obj);
  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);
  auto instance_id = bare_bluetooth_android_get_characteristic_instance_id(env, characteristic);

  auto *event = new bare_bluetooth_android_server_read_request_t();
  event->device_address = address;
  event->request_id = request_id;
  event->characteristic_uuid = uuid_str;
  event->characteristic_instance_id = instance_id;
  event->offset = offset;

  js_call_threadsafe_function(server->tsfn_read_request, event);
}

static void
bare_bluetooth_android_on_write_request(
  java_env_t env,
  j_hp_gatt_server_callback_t self,
  long native_ptr,
  j_bluetooth_device_t device,
  int request_id,
  j_bluetooth_gatt_characteristic_t characteristic,
  bool prepared_write,
  bool response_needed,
  int offset,
  java_array_t<unsigned char> value
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_servers_mutex);
  auto *server = bare_bluetooth_android_find_server(static_cast<uint64_t>(native_ptr));
  if (server == nullptr || server->exiting) return;

  auto device_obj = j_bluetooth_device_t(env, device);
  auto address = device_obj.get_class().get_method<std::string()>("getAddress")(device_obj);
  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);
  auto instance_id = bare_bluetooth_android_get_characteristic_instance_id(env, characteristic);

  auto *event = new bare_bluetooth_android_server_write_request_t();
  event->device_address = address;
  event->request_id = request_id;
  event->characteristic_uuid = uuid_str;
  event->characteristic_instance_id = instance_id;
  event->offset = offset;
  event->response_needed = response_needed;

  if (static_cast<jobject>(value) != nullptr) {
    event->data = value.slice();
  }

  js_call_threadsafe_function(server->tsfn_write_request, event);
}

static void
bare_bluetooth_android_on_descriptor_write_request(
  java_env_t env,
  j_hp_gatt_server_callback_t self,
  long native_ptr,
  j_bluetooth_device_t device,
  int request_id,
  j_bluetooth_gatt_descriptor_t descriptor,
  bool prepared_write,
  bool response_needed,
  int offset,
  java_array_t<unsigned char> value
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_servers_mutex);
  auto *server = bare_bluetooth_android_find_server(static_cast<uint64_t>(native_ptr));
  if (server == nullptr || server->exiting) return;

  auto desc_uuid = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattDescriptor">(env, descriptor);

  auto device_obj = j_bluetooth_device_t(env, device);
  auto device_address = device_obj.get_class().get_method<std::string()>("getAddress")(device_obj);

  if (response_needed) {
    auto *event = new bare_bluetooth_android_server_descriptor_response_t();
    event->device_address = device_address;
    event->request_id = request_id;
    event->offset = offset;

    if (static_cast<jobject>(value) != nullptr) {
      event->data = value.slice();
    }

    js_call_threadsafe_function(server->tsfn_descriptor_response, event);
  }

  bool is_cccd = desc_uuid == "00002902-0000-1000-8000-00805f9b34fb";
  if (!is_cccd) return;

  auto desc_obj = j_bluetooth_gatt_descriptor_t(env, descriptor);
  auto characteristic = desc_obj.get_class().get_method<j_bluetooth_gatt_characteristic_t()>("getCharacteristic")(desc_obj);
  auto char_uuid = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);

  bool subscribing = false;
  if (static_cast<jobject>(value) != nullptr) {
    auto arr = java_array_t<unsigned char>(env, value);
    if (arr.size() >= 1) {
      subscribing = (arr[static_cast<size_t>(0)] != 0);
    }
  }

  if (subscribing) {
    auto *event = new bare_bluetooth_android_server_subscribe_t();
    event->device_address = device_address;
    event->characteristic_uuid = char_uuid;
    js_call_threadsafe_function(server->tsfn_subscribe, event);
  } else {
    auto *event = new bare_bluetooth_android_server_unsubscribe_t();
    event->device_address = device_address;
    event->characteristic_uuid = char_uuid;
    js_call_threadsafe_function(server->tsfn_unsubscribe, event);
  }
}

static void
bare_bluetooth_android_on_advertise_success(
  java_env_t,
  j_hp_advertise_callback_t,
  long,
  j_advertise_settings_t
) {
}

static void
bare_bluetooth_android_on_advertise_failure(
  java_env_t,
  j_hp_advertise_callback_t,
  long native_ptr,
  int error_code
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_servers_mutex);
  auto *server = bare_bluetooth_android_find_server(static_cast<uint64_t>(native_ptr));
  if (server == nullptr || server->exiting) return;

  const char *message;
  switch (error_code) {
  case 1:
    message = "Data too large";
    break;
  case 2:
    message = "Too many advertisers";
    break;
  case 3:
    message = "Already started";
    break;
  case 4:
    message = "Internal error";
    break;
  case 5:
    message = "Feature unsupported";
    break;
  default:
    message = "Unknown advertise error";
    break;
  }

  auto *event = new bare_bluetooth_android_server_advertise_error_t();
  event->error_code = error_code;
  event->error = message;

  js_call_threadsafe_function(server->tsfn_advertise_error, event);
}

static void
bare_bluetooth_android_on_notification_sent(
  java_env_t env,
  j_hp_gatt_server_callback_t self,
  long native_ptr,
  j_bluetooth_device_t device,
  int status
) {
  std::lock_guard<std::mutex> lock(bare_bluetooth_android_servers_mutex);
  auto *server = bare_bluetooth_android_find_server(static_cast<uint64_t>(native_ptr));
  if (server == nullptr || server->exiting) return;

  auto device_obj = j_bluetooth_device_t(env, device);
  auto address = device_obj.get_class().get_method<std::string()>("getAddress")(device_obj);

  auto *event = new bare_bluetooth_android_server_notify_sent_t();
  event->device_address = address;
  event->status = status;

  js_call_threadsafe_function(server->tsfn_notify_sent, event);
}

static void
bare_bluetooth_android_register_natives() {
  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto loader = bare_bluetooth_android_get_class_loader(jenv);

  {
    auto cls = loader.load_class<"to/holepunch/bare/bluetooth/GattServerCallback">();
    cls.register_natives(
      java_native_method_t<bare_bluetooth_android_on_server_connection_state_change>("nativeOnConnectionStateChange"),
      java_native_method_t<bare_bluetooth_android_on_service_added>("nativeOnServiceAdded"),
      java_native_method_t<bare_bluetooth_android_on_read_request>("nativeOnCharacteristicReadRequest"),
      java_native_method_t<bare_bluetooth_android_on_write_request>("nativeOnCharacteristicWriteRequest"),
      java_native_method_t<bare_bluetooth_android_on_descriptor_write_request>("nativeOnDescriptorWriteRequest"),
      java_native_method_t<bare_bluetooth_android_on_notification_sent>("nativeOnNotificationSent")
    );
  }

  {
    auto cls = loader.load_class<"to/holepunch/bare/bluetooth/GattCallback">();
    cls.register_natives(
      java_native_method_t<bare_bluetooth_android_on_connection_state_change>("nativeOnConnectionStateChange"),
      java_native_method_t<bare_bluetooth_android_on_services_discovered>("nativeOnServicesDiscovered"),
      java_native_method_t<bare_bluetooth_android_on_characteristic_read>("nativeOnCharacteristicRead"),
      java_native_method_t<bare_bluetooth_android_on_characteristic_write>("nativeOnCharacteristicWrite"),
      java_native_method_t<bare_bluetooth_android_on_characteristic_changed>("nativeOnCharacteristicChanged"),
      java_native_method_t<bare_bluetooth_android_on_descriptor_write>("nativeOnDescriptorWrite"),
      java_native_method_t<bare_bluetooth_android_on_mtu_changed>("nativeOnMtuChanged")
    );
  }

  {
    auto cls = loader.load_class<"to/holepunch/bare/bluetooth/ScanCallback">();
    cls.register_natives(
      java_native_method_t<bare_bluetooth_android_on_scan_result>("nativeOnScanResult"),
      java_native_method_t<bare_bluetooth_android_on_scan_failed>("nativeOnScanFailed")
    );
  }

  {
    auto cls = loader.load_class<"to/holepunch/bare/bluetooth/AdvertiseCallback">();
    cls.register_natives(
      java_native_method_t<bare_bluetooth_android_on_advertise_success>("nativeOnStartSuccess"),
      java_native_method_t<bare_bluetooth_android_on_advertise_failure>("nativeOnStartFailure")
    );
  }

  {
    auto cls = loader.load_class<"to/holepunch/bare/bluetooth/L2capConnector">();
    cls.register_natives(
      java_native_method_t<bare_bluetooth_android_on_l2cap_connector_complete>("nativeOnComplete")
    );
  }

  {
    auto cls = loader.load_class<"to/holepunch/bare/bluetooth/L2capReader">();
    cls.register_natives(
      java_native_method_t<bare_bluetooth_android_on_l2cap_reader_open>("nativeOnOpen"),
      java_native_method_t<bare_bluetooth_android_on_l2cap_reader_data>("nativeOnData"),
      java_native_method_t<bare_bluetooth_android_on_l2cap_reader_end>("nativeOnEnd"),
      java_native_method_t<bare_bluetooth_android_on_l2cap_reader_error>("nativeOnError"),
      java_native_method_t<bare_bluetooth_android_on_l2cap_reader_close>("nativeOnClose")
    );
  }

  {
    auto cls = loader.load_class<"to/holepunch/bare/bluetooth/L2capAcceptor">();
    cls.register_natives(
      java_native_method_t<bare_bluetooth_android_on_l2cap_acceptor_accepted>("nativeOnAccepted"),
      java_native_method_t<bare_bluetooth_android_on_l2cap_acceptor_error>("nativeOnError")
    );
  }
}

static js_value_t *
bare_bluetooth_android_exports(js_env_t *env, js_value_t *exports) {
  int err;

  bare_bluetooth_android_register_natives();

#define V(name, fn) \
  { \
    err = js_set_property<fn>(env, exports, name); \
    assert(err == 0); \
  }

  V("centralInit", bare_bluetooth_android_central_init)
  V("centralStartScan", bare_bluetooth_android_central_start_scan)
  V("centralStopScan", bare_bluetooth_android_central_stop_scan)
  V("centralConnect", bare_bluetooth_android_central_connect)
  V("centralDisconnect", bare_bluetooth_android_central_disconnect)
  V("centralDestroy", bare_bluetooth_android_central_destroy)
  V("createUUID", bare_bluetooth_android_create_uuid)

  V("peripheralInit", bare_bluetooth_android_peripheral_init)
  V("peripheralId", bare_bluetooth_android_peripheral_id)
  V("peripheralName", bare_bluetooth_android_peripheral_name)
  V("peripheralDiscoverServices", bare_bluetooth_android_peripheral_discover_services)
  V("peripheralDiscoverCharacteristics", bare_bluetooth_android_peripheral_discover_characteristics)
  V("peripheralRead", bare_bluetooth_android_peripheral_read)
  V("peripheralWrite", bare_bluetooth_android_peripheral_write)
  V("peripheralSubscribe", bare_bluetooth_android_peripheral_subscribe)
  V("peripheralUnsubscribe", bare_bluetooth_android_peripheral_unsubscribe)
  V("peripheralRequestMtu", bare_bluetooth_android_peripheral_request_mtu)
  V("peripheralOpenL2CAPChannel", bare_bluetooth_android_peripheral_open_l2cap_channel)
  V("peripheralDestroy", bare_bluetooth_android_peripheral_destroy)

  V("peripheralServiceCount", bare_bluetooth_android_peripheral_service_count)
  V("peripheralServiceAtIndex", bare_bluetooth_android_peripheral_service_at_index)
  V("serviceKey", bare_bluetooth_android_service_key)
  V("serviceUuid", bare_bluetooth_android_service_uuid)
  V("serviceCharacteristicCount", bare_bluetooth_android_service_characteristic_count)
  V("serviceCharacteristicAtIndex", bare_bluetooth_android_service_characteristic_at_index)
  V("characteristicKey", bare_bluetooth_android_characteristic_key)
  V("characteristicUuid", bare_bluetooth_android_characteristic_uuid)
  V("characteristicProperties", bare_bluetooth_android_characteristic_properties)

  V("serverInit", bare_bluetooth_android_server_init)
  V("serverAddService", bare_bluetooth_android_server_add_service)
  V("serverStartAdvertising", bare_bluetooth_android_server_start_advertising)
  V("serverStopAdvertising", bare_bluetooth_android_server_stop_advertising)
  V("serverRespondToRequest", bare_bluetooth_android_server_respond_to_request)
  V("serverUpdateValue", bare_bluetooth_android_server_update_value)
  V("serverPublishChannel", bare_bluetooth_android_server_publish_channel)
  V("serverUnpublishChannel", bare_bluetooth_android_server_unpublish_channel)
  V("serverDestroy", bare_bluetooth_android_server_destroy)
  V("createMutableService", bare_bluetooth_android_create_mutable_service)
  V("createMutableCharacteristic", bare_bluetooth_android_create_mutable_characteristic)
  V("serviceSetCharacteristics", bare_bluetooth_android_service_set_characteristics)

  V("l2capInit", bare_bluetooth_android_l2cap_init)
  V("l2capOpen", bare_bluetooth_android_l2cap_open)
  V("l2capWrite", bare_bluetooth_android_l2cap_write)
  V("l2capEnd", bare_bluetooth_android_l2cap_end)
  V("l2capPsm", bare_bluetooth_android_l2cap_psm)
  V("l2capPeer", bare_bluetooth_android_l2cap_peer)
#undef V

#define V(name, n) \
  { \
    js_value_t *val; \
    err = js_create_int32(env, n, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, exports, name, val); \
    assert(err == 0); \
  }

  V("STATE_OFF", 10)
  V("STATE_TURNING_ON", 11)
  V("STATE_ON", 12)
  V("STATE_TURNING_OFF", 13)

  V("PROPERTY_READ", 0x02)
  V("PROPERTY_WRITE_WITHOUT_RESPONSE", 0x04)
  V("PROPERTY_WRITE", 0x08)
  V("PROPERTY_NOTIFY", 0x10)
  V("PROPERTY_INDICATE", 0x20)

  V("PERMISSION_READABLE", 0x01)
  V("PERMISSION_WRITEABLE", 0x02)
  V("PERMISSION_READ_ENCRYPTED", 0x04)
  V("PERMISSION_WRITE_ENCRYPTED", 0x08)

  V("ATT_SUCCESS", 0x00)
  V("ATT_INVALID_HANDLE", 0x01)
  V("ATT_READ_NOT_PERMITTED", 0x02)
  V("ATT_WRITE_NOT_PERMITTED", 0x03)
  V("ATT_INSUFFICIENT_RESOURCES", 0x11)
  V("ATT_UNLIKELY_ERROR", 0x0E)

  // BluetoothProfile.STATE_* — stable since API 5, hardcoded to avoid JNI reflection
  V("CONNECTION_STATE_DISCONNECTED", 0)
  V("CONNECTION_STATE_CONNECTING", 1)
  V("CONNECTION_STATE_CONNECTED", 2)
  V("CONNECTION_STATE_DISCONNECTING", 3)

  V("SCAN_MODE_LOW_POWER", 0)
  V("SCAN_MODE_BALANCED", 1)
  V("SCAN_MODE_LOW_LATENCY", 2)
  V("SCAN_MODE_OPPORTUNISTIC", -1)
#undef V

  return exports;
}

BARE_MODULE(bare_bluetooth_android, bare_bluetooth_android_exports)
