#include <assert.h>
#include <bare.h>
#include <jni.h>
#include <jnitl.h>
#include <js.h>
#include <utf.h>

#include <stdlib.h>
#include <uv.h>

#include <unordered_map>
#include <unordered_set>

static inline java_vm_t
bare_bluetooth_android_jvm() {
  return java_vm_t::get_created().value();
}

static inline java_class_loader_t
bare_bluetooth_android_get_class_loader(JNIEnv *env) {
  auto thread = java_thread_t::current_thread(env);
  return thread.get_context_class_loader();
}

static inline java_object_t<"android/content/Context">
bare_bluetooth_android_get_context(JNIEnv *env) {
  auto cls = java_class_t<"android/app/ActivityThread">(env);
  auto current_app = cls.get_static_method<java_object_t<"android/app/Application">()>("currentApplication");
  return java_object_t<"android/content/Context">(env, current_app());
}

template <java_class_name_t N>
static inline std::string
bare_bluetooth_android_get_uuid_string(JNIEnv *env, java_object_t<N> obj) {
  auto uuid = obj.get_class().template get_method<java_object_t<"java/util/UUID">()>("getUuid")(obj);
  return uuid.get_class().template get_method<std::string()>("toString")(uuid);
}

static inline std::string
bare_bluetooth_android_get_device_address(JNIEnv *env, java_object_t<"android/bluetooth/BluetoothGatt"> gatt) {
  auto device = gatt.get_class().get_method<java_object_t<"android/bluetooth/BluetoothDevice">()>("getDevice")(gatt);
  return device.get_class().get_method<std::string()>("getAddress")(device);
}

static inline java_array_t<unsigned char>
bare_bluetooth_android_make_byte_array(JNIEnv *env, const void *data, size_t length) {
  auto arr = java_array_t<unsigned char>(env, static_cast<int>(length));
  env->SetByteArrayRegion(arr, 0, static_cast<jsize>(length), reinterpret_cast<const jbyte *>(data));
  return arr;
}

static inline std::vector<unsigned char>
bare_bluetooth_android_copy_byte_array(JNIEnv *env, java_array_t<unsigned char> arr) {
  auto len = arr.size();
  std::vector<unsigned char> result(len);
  env->GetByteArrayRegion(arr, 0, static_cast<jsize>(len), reinterpret_cast<jbyte *>(result.data()));
  return result;
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

static std::unordered_map<std::string, bare_bluetooth_android_peripheral_t *> bare_bluetooth_android_peripherals;

static bare_bluetooth_android_peripheral_t *
bare_bluetooth_android_find_peripheral(JNIEnv *env, java_object_t<"android/bluetooth/BluetoothGatt"> gatt) {
  auto address = bare_bluetooth_android_get_device_address(env, gatt);
  auto it = bare_bluetooth_android_peripherals.find(address);
  if (it == bare_bluetooth_android_peripherals.end()) return NULL;
  return it->second;
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

  java_global_ref_t<java_object_t<"android/bluetooth/BluetoothSocket">> socket;
  int psm;
  std::string peer_address;

  uv_thread_t reader_thread;
  std::atomic<bool> opened;
  std::atomic<bool> destroyed;
  std::atomic<bool> finalized;
};

typedef struct {
  void *bytes;
  size_t len;
} bare_bluetooth_android_channel_data_t;

typedef struct {
  std::string message;
} bare_bluetooth_android_channel_error_t;

struct bare_bluetooth_android_device_handle_t {
  java_global_ref_t<java_object_t<"android/bluetooth/BluetoothDevice">> handle;
};

struct bare_bluetooth_android_gatt_handle_t {
  java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGatt">> handle;
};

struct bare_bluetooth_android_characteristic_handle_t {
  java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">> handle;
};

struct bare_bluetooth_android_service_handle_t {
  java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattService">> handle;
};

struct bare_bluetooth_android_socket_handle_t {
  java_global_ref_t<java_object_t<"android/bluetooth/BluetoothSocket">> handle;
};

struct bare_bluetooth_android_server_socket_handle_t {
  java_global_ref_t<java_object_t<"android/bluetooth/BluetoothServerSocket">> handle;
};

struct bare_bluetooth_android_uuid_handle_t {
  java_global_ref_t<java_object_t<"java/util/UUID">> handle;
};

template <typename T>
static void
bare_bluetooth_android__on_release(js_env_t *env, void *data, void *hint) {
  delete static_cast<T *>(data);
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

  java_global_ref_t<java_object_t<"android/bluetooth/BluetoothAdapter">> adapter;
  java_global_ref_t<java_object_t<"android/bluetooth/le/BluetoothLeScanner">> scanner;
  java_global_ref_t<java_object_t<"to/holepunch/bare/bluetooth/ScanCallback">> scan_callback;

  std::unordered_set<std::string> connected_addresses;
  java_global_ref_t<java_object_t<"to/holepunch/bare/bluetooth/GattCallback">> gatt_callback_ref;
};

typedef struct {
  int32_t state;
} bare_bluetooth_android_central_state_change_t;

typedef struct {
  std::string address;
  std::string name;
  int32_t rssi;
  bare_bluetooth_android_device_handle_t *device;
} bare_bluetooth_android_central_discover_t;

typedef struct {
  std::string address;
  bare_bluetooth_android_gatt_handle_t *gatt;
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

struct bare_bluetooth_android_peripheral_t {
  js_env_t *env;
  js_ref_t *ctx;
  bool destroyed;

  js_threadsafe_function_t *tsfn_services_discover;
  js_threadsafe_function_t *tsfn_characteristics_discover;
  js_threadsafe_function_t *tsfn_read;
  js_threadsafe_function_t *tsfn_write;
  js_threadsafe_function_t *tsfn_notify;
  js_threadsafe_function_t *tsfn_notify_state;
  js_threadsafe_function_t *tsfn_channel_open;
  js_threadsafe_function_t *tsfn_mtu_changed;

  java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGatt">> gatt;
  java_global_ref_t<java_object_t<"android/bluetooth/BluetoothDevice">> device;

  std::vector<java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattService">>> services;
  std::vector<std::vector<java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">>>> service_characteristics;

  uv_thread_t l2cap_thread;
  std::atomic<bool> l2cap_connecting;
};

typedef struct {
  uint32_t count;
  std::string error;
} bare_bluetooth_android_peripheral_services_discover_t;

typedef struct {
  bare_bluetooth_android_service_handle_t *service;
  uint32_t count;
  std::string error;
} bare_bluetooth_android_peripheral_characteristics_discover_t;

typedef struct {
  bare_bluetooth_android_characteristic_handle_t *characteristic;
  std::string uuid;
  std::vector<unsigned char> data;
  std::string error;
} bare_bluetooth_android_peripheral_read_t;

typedef struct {
  bare_bluetooth_android_characteristic_handle_t *characteristic;
  std::string uuid;
  std::string error;
} bare_bluetooth_android_peripheral_write_t;

typedef struct {
  bare_bluetooth_android_characteristic_handle_t *characteristic;
  std::string uuid;
  std::vector<unsigned char> data;
  std::string error;
} bare_bluetooth_android_peripheral_notify_t;

typedef struct {
  bare_bluetooth_android_characteristic_handle_t *characteristic;
  std::string uuid;
  bool is_notifying;
  std::string error;
} bare_bluetooth_android_peripheral_notify_state_t;

typedef struct {
  bare_bluetooth_android_socket_handle_t *channel;
  std::string error;
  uint32_t psm;
} bare_bluetooth_android_peripheral_channel_open_t;

typedef struct {
  int32_t mtu;
  std::string error;
} bare_bluetooth_android_peripheral_mtu_changed_t;

struct bare_bluetooth_android_server_t {
  js_env_t *env;
  js_ref_t *ctx;

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

  java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattServer">> gatt_server;
  java_global_ref_t<java_object_t<"android/bluetooth/le/BluetoothLeAdvertiser">> advertiser;
  java_global_ref_t<java_object_t<"android/bluetooth/le/AdvertiseCallback">> advertise_callback;

  struct published_channel_t {
    java_global_ref_t<java_object_t<"android/bluetooth/BluetoothServerSocket">> server_socket;
    uv_thread_t accept_thread;
    std::atomic<bool> accepting;
    uint16_t psm;
  };

  std::vector<published_channel_t *> published_channels;

  std::unordered_map<std::string, std::unordered_set<std::string>> subscriptions;
  std::unordered_map<std::string, java_global_ref_t<java_object_t<"android/bluetooth/BluetoothDevice">>> connected_devices;
  std::unordered_map<std::string, java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">>> characteristics;
};

typedef struct {
  int32_t state;
} bare_bluetooth_android_server_state_change_t;

typedef struct {
  std::string uuid;
  std::string error;
} bare_bluetooth_android_server_add_service_t;

typedef struct {
  bare_bluetooth_android_device_handle_t *device;
  int32_t request_id;
  std::string characteristic_uuid;
  int32_t offset;
} bare_bluetooth_android_server_read_request_t;

typedef struct {
  bare_bluetooth_android_device_handle_t *device;
  int32_t request_id;
  std::string characteristic_uuid;
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
  uint16_t psm;
  std::string error;
} bare_bluetooth_android_server_channel_publish_t;

struct bare_bluetooth_android_server_channel_open_t {
  bare_bluetooth_android_socket_handle_t *channel;
  std::string error;
  uint16_t psm;
};

struct bare_bluetooth_android_server_connection_state_t {
  std::string address;
  bare_bluetooth_android_device_handle_t *device;
  int32_t new_state;
};

static void
bare_bluetooth_android_channel__on_data(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_channel_data_t *>(data);
  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, channel->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[1];

  js_value_t *arraybuffer;
  void *buf;
  err = js_create_arraybuffer(env, event->len, &buf, &arraybuffer);
  assert(err == 0);
  memcpy(buf, event->bytes, event->len);
  free(event->bytes);

  err = js_create_typedarray(env, js_uint8array, event->len, arraybuffer, 0, &argv[0]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 1, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_channel__on_drain(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, channel->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, receiver, function, 0, NULL, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_channel__on_end(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, channel->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, receiver, function, 0, NULL, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_channel__on_error(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_channel_error_t *>(data);
  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, channel->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[1];

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->message.c_str()), -1, &argv[0]);
  assert(err == 0);
  delete event;

  js_call_function(env, receiver, function, 1, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_channel__on_close(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, channel->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, receiver, function, 0, NULL, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);

  bool expected = false;
  if (channel->finalized.compare_exchange_strong(expected, true)) {
    js_release_threadsafe_function(channel->tsfn_open, js_threadsafe_function_abort);
    js_release_threadsafe_function(channel->tsfn_close, js_threadsafe_function_abort);
    js_release_threadsafe_function(channel->tsfn_error, js_threadsafe_function_abort);
    js_release_threadsafe_function(channel->tsfn_end, js_threadsafe_function_abort);
    js_release_threadsafe_function(channel->tsfn_drain, js_threadsafe_function_abort);
    js_release_threadsafe_function(channel->tsfn_data, js_threadsafe_function_abort);

    err = js_delete_reference(env, channel->ctx);
    assert(err == 0);

    delete channel;
  }
}

static void
bare_bluetooth_android_channel__on_open(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, channel->ctx, &receiver);
  assert(err == 0);

  js_call_function(env, receiver, function, 0, NULL, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_channel__reader_thread(void *arg) {
  auto *channel = static_cast<bare_bluetooth_android_channel_t *>(arg);

  auto jenv = bare_bluetooth_android_jvm().attach_current_thread();

  auto socket = java_object_t<"android/bluetooth/BluetoothSocket">(jenv, channel->socket);
  auto get_input = socket.get_class().get_method<java_object_t<"java/io/InputStream">()>("getInputStream");
  auto input_stream = get_input(socket);

  if (bare_bluetooth_android_check_exception(jenv)) {
    auto *event = new bare_bluetooth_android_channel_error_t();
    event->message = std::string("Failed to get InputStream");
    js_call_threadsafe_function(channel->tsfn_error, event, js_threadsafe_function_nonblocking);
    js_call_threadsafe_function(channel->tsfn_close, NULL, js_threadsafe_function_nonblocking);
    return;
  }

  js_call_threadsafe_function(channel->tsfn_open, NULL, js_threadsafe_function_nonblocking);

  auto read_method = input_stream.get_class().get_method<int(java_array_t<unsigned char>)>("read");

  JNIEnv *jni_env = jenv;
  jbyteArray read_buf = jni_env->NewByteArray(4096);

  while (!channel->destroyed) {
    int bytes_read = read_method(input_stream, java_array_t<unsigned char>(jenv, read_buf));

    if (bare_bluetooth_android_check_exception(jenv)) {
      if (!channel->destroyed) {
        auto *event = new bare_bluetooth_android_channel_error_t();
        event->message = std::string("Read error");
        js_call_threadsafe_function(channel->tsfn_error, event, js_threadsafe_function_nonblocking);
      }
      break;
    }

    if (bytes_read == -1) {
      js_call_threadsafe_function(channel->tsfn_end, NULL, js_threadsafe_function_nonblocking);
      break;
    }

    if (bytes_read > 0) {
      auto *event = new bare_bluetooth_android_channel_data_t();
      event->len = static_cast<size_t>(bytes_read);
      event->bytes = malloc(bytes_read);
      jni_env->GetByteArrayRegion(read_buf, 0, bytes_read, reinterpret_cast<jbyte *>(event->bytes));

      js_call_threadsafe_function(channel->tsfn_data, event, js_threadsafe_function_nonblocking);
    }
  }

  auto close_method = socket.get_class().get_method<void()>("close");
  close_method(socket);
  static_cast<JNIEnv *>(jenv)->ExceptionClear();

  js_call_threadsafe_function(channel->tsfn_close, NULL, js_threadsafe_function_nonblocking);
}

static js_value_t *
bare_bluetooth_android_l2cap_init(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 8;
  js_value_t *argv[8];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 8);

  void *socket_ptr;
  err = js_get_value_external(env, argv[0], &socket_ptr);
  assert(err == 0);

  auto *socket_handle = static_cast<bare_bluetooth_android_socket_handle_t *>(socket_ptr);

  auto *channel = new bare_bluetooth_android_channel_t();
  channel->env = env;
  channel->socket = java_global_ref_t<java_object_t<"android/bluetooth/BluetoothSocket">>(std::move(socket_handle->handle));
  channel->opened = false;
  channel->destroyed = false;
  channel->finalized = false;

  delete socket_handle;

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto socket = java_object_t<"android/bluetooth/BluetoothSocket">(jenv, channel->socket);
  auto get_device = socket.get_class().get_method<java_object_t<"android/bluetooth/BluetoothDevice">()>("getRemoteDevice");
  auto device = get_device(socket);
  auto get_address = device.get_class().get_method<std::string()>("getAddress");
  channel->peer_address = get_address(device);
  channel->psm = 0;

  err = js_create_reference(env, argv[1], 1, &channel->ctx);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[2], 0, 1, NULL, NULL, channel, bare_bluetooth_android_channel__on_data, &channel->tsfn_data);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[3], 0, 1, NULL, NULL, channel, bare_bluetooth_android_channel__on_drain, &channel->tsfn_drain);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[4], 0, 1, NULL, NULL, channel, bare_bluetooth_android_channel__on_end, &channel->tsfn_end);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[5], 0, 1, NULL, NULL, channel, bare_bluetooth_android_channel__on_error, &channel->tsfn_error);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[6], 0, 1, NULL, NULL, channel, bare_bluetooth_android_channel__on_close, &channel->tsfn_close);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[7], 0, 1, NULL, NULL, channel, bare_bluetooth_android_channel__on_open, &channel->tsfn_open);
  assert(err == 0);

  js_value_t *handle;
  err = js_create_external(env, channel, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_bluetooth_android_l2cap_open(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_channel_t *channel;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&channel));
  assert(err == 0);

  bool expected = false;
  if (!channel->opened.compare_exchange_strong(expected, true)) return NULL;

  err = uv_thread_create(&channel->reader_thread, bare_bluetooth_android_channel__reader_thread, channel);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_l2cap_write(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_channel_t *channel;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&channel));
  assert(err == 0);

  if (channel->destroyed || !channel->opened) {
    js_value_t *result;
    err = js_create_int32(env, 0, &result);
    assert(err == 0);
    return result;
  }

  size_t length;
  js_value_t *arraybuffer;
  size_t offset;
  err = js_get_typedarray_info(env, argv[1], NULL, NULL, &length, &arraybuffer, &offset);
  assert(err == 0);

  void *buf;
  err = js_get_arraybuffer_info(env, arraybuffer, &buf, NULL);
  assert(err == 0);

  uint8_t *data = static_cast<uint8_t *>(buf) + offset;

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto socket = java_object_t<"android/bluetooth/BluetoothSocket">(jenv, channel->socket);
  auto get_output = socket.get_class().get_method<java_object_t<"java/io/OutputStream">()>("getOutputStream");
  auto output = get_output(socket);

  auto byte_array = bare_bluetooth_android_make_byte_array(jenv, data, length);

  auto write_method = output.get_class().get_method<void(java_array_t<unsigned char>)>("write");
  write_method(output, byte_array);

  bool write_ok = !bare_bluetooth_android_check_exception(jenv);

  if (write_ok) {
    js_call_threadsafe_function(channel->tsfn_drain, NULL, js_threadsafe_function_nonblocking);
  } else {
    auto *event = new bare_bluetooth_android_channel_error_t();
    event->message = std::string("Write error");
    js_call_threadsafe_function(channel->tsfn_error, event, js_threadsafe_function_nonblocking);
  }

  js_value_t *result;
  err = js_create_int32(env, write_ok ? static_cast<int32_t>(length) : 0, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_l2cap_end(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_channel_t *channel;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&channel));
  assert(err == 0);

  bool expected = false;
  if (!channel->destroyed.compare_exchange_strong(expected, true)) return NULL;

  if (!channel->opened) {
    js_call_threadsafe_function(channel->tsfn_close, NULL, js_threadsafe_function_nonblocking);
    return NULL;
  }

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto socket = java_object_t<"android/bluetooth/BluetoothSocket">(jenv, channel->socket);
  auto close = socket.get_class().get_method<void()>("close");
  close(socket);
  static_cast<JNIEnv *>(jenv)->ExceptionClear();

  return NULL;
}

static js_value_t *
bare_bluetooth_android_l2cap_psm(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_channel_t *channel;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&channel));
  assert(err == 0);

  js_value_t *result;
  err = js_create_uint32(env, static_cast<uint32_t>(channel->psm), &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_l2cap_peer(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_channel_t *channel;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&channel));
  assert(err == 0);

  if (channel->peer_address.empty()) {
    js_value_t *result;
    err = js_get_null(env, &result);
    assert(err == 0);
    return result;
  }

  js_value_t *result;
  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(channel->peer_address.c_str()), channel->peer_address.size(), &result);
  assert(err == 0);

  return result;
}

static void
bare_bluetooth_android_central__on_state_change(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_central_state_change_t *>(data);
  auto *central = static_cast<bare_bluetooth_android_central_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[1];
  err = js_create_int32(env, event->state, &argv[0]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 1, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_discover(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_central_discover_t *>(data);
  auto *central = static_cast<bare_bluetooth_android_central_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[4];

  err = js_create_external(env, event->device, bare_bluetooth_android__on_release<bare_bluetooth_android_device_handle_t>, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->address.c_str()), -1, &argv[1]);
  assert(err == 0);

  if (!event->name.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->name.c_str()), -1, &argv[2]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[2]);
    assert(err == 0);
  }

  err = js_create_int32(env, event->rssi, &argv[3]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 4, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_connect(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_central_connect_t *>(data);
  auto *central = static_cast<bare_bluetooth_android_central_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_external(env, event->gatt, bare_bluetooth_android__on_release<bare_bluetooth_android_gatt_handle_t>, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->address.c_str()), -1, &argv[1]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_disconnect(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_central_disconnect_t *>(data);
  auto *central = static_cast<bare_bluetooth_android_central_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->address.c_str()), -1, &argv[0]);
  assert(err == 0);

  if (!event->error.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[1]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  delete event;

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_connect_fail(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_central_connect_fail_t *>(data);
  auto *central = static_cast<bare_bluetooth_android_central_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->address.c_str()), -1, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[1]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_scan_fail(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_central_scan_fail_t *>(data);
  auto *central = static_cast<bare_bluetooth_android_central_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[1];
  err = js_create_int32(env, event->error_code, &argv[0]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 1, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static js_value_t *
bare_bluetooth_android_central_init(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 7;
  js_value_t *argv[7];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 7);

  auto *central = new bare_bluetooth_android_central_t();
  central->env = env;

  err = js_create_reference(env, argv[0], 1, &central->ctx);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[1], 0, 1, NULL, NULL, central, bare_bluetooth_android_central__on_state_change, &central->tsfn_state_change);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[2], 0, 1, NULL, NULL, central, bare_bluetooth_android_central__on_discover, &central->tsfn_discover);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[3], 0, 1, NULL, NULL, central, bare_bluetooth_android_central__on_connect, &central->tsfn_connect);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[4], 0, 1, NULL, NULL, central, bare_bluetooth_android_central__on_disconnect, &central->tsfn_disconnect);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[5], 0, 1, NULL, NULL, central, bare_bluetooth_android_central__on_connect_fail, &central->tsfn_connect_fail);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[6], 0, 1, NULL, NULL, central, bare_bluetooth_android_central__on_scan_fail, &central->tsfn_scan_fail);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto activity = bare_bluetooth_android_get_context(jenv);
  auto get_system_service = activity.get_class().get_method<java_object_t<"java/lang/Object">(std::string)>("getSystemService");
  auto manager_obj = get_system_service(activity, std::string("bluetooth"));

  auto bt_manager = java_object_t<"android/bluetooth/BluetoothManager">(jenv, manager_obj);
  auto get_adapter = bt_manager.get_class().get_method<java_object_t<"android/bluetooth/BluetoothAdapter">()>("getAdapter");
  auto adapter_local = get_adapter(bt_manager);

  central->adapter = java_global_ref_t<java_object_t<"android/bluetooth/BluetoothAdapter">>(jenv, adapter_local);

  auto get_scanner = central->adapter.get_class().get_method<java_object_t<"android/bluetooth/le/BluetoothLeScanner">()>("getBluetoothLeScanner");
  auto scanner_local = get_scanner(central->adapter);

  central->scanner = java_global_ref_t<java_object_t<"android/bluetooth/le/BluetoothLeScanner">>(jenv, scanner_local);

  auto get_state = central->adapter.get_class().get_method<int()>("getState");
  int android_state = get_state(central->adapter);

  auto *state_event = new bare_bluetooth_android_central_state_change_t();
  state_event->state = android_state;
  js_call_threadsafe_function(central->tsfn_state_change, state_event, js_threadsafe_function_nonblocking);

  js_value_t *handle;
  err = js_create_external(env, central, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_bluetooth_android_central_start_scan(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1 || argc == 2);

  bare_bluetooth_android_central_t *central;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&central));
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  java_object_t<"java/util/List"> filter_list;

  if (argc == 2) {
    js_value_type_t type;
    err = js_typeof(env, argv[1], &type);
    assert(err == 0);

    if (type != js_null && type != js_undefined) {
      uint32_t uuid_count;
      err = js_get_array_length(env, argv[1], &uuid_count);
      assert(err == 0);

      auto arraylist_class = java_class_t<"java/util/ArrayList">(jenv);
      auto list = arraylist_class();
      auto list_add = list.get_class().get_method<bool(java_object_t<"java/lang/Object">)>("add");

      auto filter_builder_class = java_class_t<"android/bluetooth/le/ScanFilter$Builder">(jenv);
      auto parcel_uuid_class = java_class_t<"android/os/ParcelUuid">(jenv);

      for (uint32_t i = 0; i < uuid_count; i++) {
        js_value_t *uuid_val;
        err = js_get_element(env, argv[1], i, &uuid_val);
        assert(err == 0);

        void *uuid_ptr;
        err = js_get_value_external(env, uuid_val, &uuid_ptr);
        assert(err == 0);

        auto *uuid_handle = static_cast<bare_bluetooth_android_uuid_handle_t *>(uuid_ptr);
        auto builder = filter_builder_class();
        auto set_service_uuid = builder.get_class().get_method<java_object_t<"android/bluetooth/le/ScanFilter$Builder">(java_object_t<"android/os/ParcelUuid">)>("setServiceUuid");
        auto parcel_uuid = parcel_uuid_class(java_object_t<"java/util/UUID">(jenv, uuid_handle->handle));
        set_service_uuid(builder, parcel_uuid);

        auto build_filter = builder.get_class().get_method<java_object_t<"android/bluetooth/le/ScanFilter">()>("build");
        auto filter = build_filter(builder);
        list_add(list, java_object_t<"java/lang/Object">(jenv, filter));
      }

      filter_list = java_object_t<"java/util/List">(jenv, list);
    }
  }

  auto callback_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/ScanCallback">();
  auto callback_local = callback_class(reinterpret_cast<long>(central));

  central->scan_callback = java_global_ref_t<java_object_t<"to/holepunch/bare/bluetooth/ScanCallback">>(jenv, callback_local);

  auto settings_builder_class = java_class_t<"android/bluetooth/le/ScanSettings$Builder">(jenv);
  auto settings_builder = settings_builder_class();

  auto set_scan_mode = settings_builder.get_class().get_method<java_object_t<"android/bluetooth/le/ScanSettings$Builder">(int)>("setScanMode");
  set_scan_mode(settings_builder, 2);

  auto build_settings = settings_builder.get_class().get_method<java_object_t<"android/bluetooth/le/ScanSettings">()>("build");
  auto settings = build_settings(settings_builder);

  auto start_scan = central->scanner.get_class().get_method<void(java_object_t<"java/util/List">, java_object_t<"android/bluetooth/le/ScanSettings">, java_object_t<"android/bluetooth/le/ScanCallback">)>("startScan");
  start_scan(central->scanner, filter_list, java_object_t<"android/bluetooth/le/ScanSettings">(jenv, settings), java_object_t<"android/bluetooth/le/ScanCallback">(jenv, central->scan_callback));

  return NULL;
}

static js_value_t *
bare_bluetooth_android_central_stop_scan(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_central_t *central;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&central));
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto stop_scan = central->scanner.get_class().get_method<void(java_object_t<"android/bluetooth/le/ScanCallback">)>("stopScan");
  stop_scan(central->scanner, java_object_t<"android/bluetooth/le/ScanCallback">(jenv, central->scan_callback));

  return NULL;
}

static js_value_t *
bare_bluetooth_android_central_connect(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_central_t *central;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&central));
  assert(err == 0);

  void *device_ptr;
  err = js_get_value_external(env, argv[1], &device_ptr);
  assert(err == 0);

  auto *device_handle = static_cast<bare_bluetooth_android_device_handle_t *>(device_ptr);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto device = java_object_t<"android/bluetooth/BluetoothDevice">(jenv, device_handle->handle);

  auto gatt_callback_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/GattCallback">();
  auto gatt_callback = gatt_callback_class(reinterpret_cast<long>(central));

  central->gatt_callback_ref = java_global_ref_t<java_object_t<"to/holepunch/bare/bluetooth/GattCallback">>(jenv, gatt_callback);

  auto context = bare_bluetooth_android_get_context(jenv);

  auto connect_gatt = device.get_class().get_method<java_object_t<"android/bluetooth/BluetoothGatt">(java_object_t<"android/content/Context">, bool, java_object_t<"android/bluetooth/BluetoothGattCallback">, int)>("connectGatt");
  connect_gatt(device, context, false, java_object_t<"android/bluetooth/BluetoothGattCallback">(jenv, gatt_callback), 2);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_central_disconnect(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_central_t *central;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&central));
  assert(err == 0);

  void *gatt_ptr;
  err = js_get_value_external(env, argv[1], &gatt_ptr);
  assert(err == 0);

  auto *gatt_handle = static_cast<bare_bluetooth_android_gatt_handle_t *>(gatt_ptr);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, gatt_handle->handle);

  auto disconnect = gatt.get_class().get_method<void()>("disconnect");
  disconnect(gatt);

  auto close = gatt.get_class().get_method<void()>("close");
  close(gatt);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_central_destroy(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_central_t *central;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&central));
  assert(err == 0);

  err = js_delete_reference(env, central->ctx);
  assert(err == 0);

  js_release_threadsafe_function(central->tsfn_scan_fail, js_threadsafe_function_abort);
  js_release_threadsafe_function(central->tsfn_connect_fail, js_threadsafe_function_abort);
  js_release_threadsafe_function(central->tsfn_disconnect, js_threadsafe_function_abort);
  js_release_threadsafe_function(central->tsfn_connect, js_threadsafe_function_abort);
  js_release_threadsafe_function(central->tsfn_discover, js_threadsafe_function_abort);
  js_release_threadsafe_function(central->tsfn_state_change, js_threadsafe_function_abort);

  delete central;

  return NULL;
}

static js_value_t *
bare_bluetooth_android_create_uuid(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  size_t len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &len);
  assert(err == 0);

  char *str = static_cast<char *>(malloc(len + 1));
  err = js_get_value_string_utf8(env, argv[0], reinterpret_cast<utf8_t *>(str), len + 1, NULL);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto uuid_class = java_class_t<"java/util/UUID">(jenv);
  auto from_string = uuid_class.get_static_method<java_object_t<"java/util/UUID">(std::string)>("fromString");
  auto uuid_local = from_string(std::string(str));

  free(str);

  auto *uuid_handle = new bare_bluetooth_android_uuid_handle_t{java_global_ref_t<java_object_t<"java/util/UUID">>(jenv, uuid_local)};

  js_value_t *handle;
  err = js_create_external(env, uuid_handle, bare_bluetooth_android__on_release<bare_bluetooth_android_uuid_handle_t>, NULL, &handle);
  assert(err == 0);

  return handle;
}

static void
bare_bluetooth_android_on_scan_result(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/ScanCallback"> self, long native_ptr, int callback_type, java_object_t<"android/bluetooth/le/ScanResult"> scan_result) {
  auto *central = reinterpret_cast<bare_bluetooth_android_central_t *>(native_ptr);

  auto device = scan_result.get_class().get_method<java_object_t<"android/bluetooth/BluetoothDevice">()>("getDevice")(scan_result);
  auto address = device.get_class().get_method<std::string()>("getAddress")(device);
  auto rssi = scan_result.get_class().get_method<int()>("getRssi")(scan_result);

  auto name_obj = device.get_class().get_method<java_object_t<"java/lang/String">()>("getName")(device);

  auto *event = new bare_bluetooth_android_central_discover_t();
  event->address = address;
  event->rssi = rssi;
  event->device = new bare_bluetooth_android_device_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothDevice">>(env, device)};

  if (static_cast<jobject>(name_obj) != nullptr) {
    auto name = java_string_t(env, name_obj);
    event->name = std::string(name);
  } else {
    event->name = {};
  }

  js_call_threadsafe_function(central->tsfn_discover, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_on_scan_failed(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/ScanCallback"> self, long native_ptr, int error_code) {
  auto *central = reinterpret_cast<bare_bluetooth_android_central_t *>(native_ptr);

  auto *event = new bare_bluetooth_android_central_scan_fail_t();
  event->error_code = error_code;

  js_call_threadsafe_function(central->tsfn_scan_fail, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_on_connection_state_change(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/GattCallback"> self, long native_ptr, java_object_t<"android/bluetooth/BluetoothGatt"> gatt, int status, int new_state) {
  auto *central = reinterpret_cast<bare_bluetooth_android_central_t *>(native_ptr);

  auto address = bare_bluetooth_android_get_device_address(env, gatt);

  if (new_state == 2 && status == 0) {
    central->connected_addresses.insert(address);

    auto *event = new bare_bluetooth_android_central_connect_t();
    event->address = address;
    event->gatt = new bare_bluetooth_android_gatt_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGatt">>(env, gatt)};

    js_call_threadsafe_function(central->tsfn_connect, event, js_threadsafe_function_nonblocking);
  } else if (new_state == 0) {
    bool was_connected = central->connected_addresses.erase(address) > 0;

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

      js_call_threadsafe_function(central->tsfn_disconnect, event, js_threadsafe_function_nonblocking);
    } else {
      auto *event = new bare_bluetooth_android_central_connect_fail_t();
      event->address = address;

      char error_buf[64];
      snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
      event->error = error_buf;

      js_call_threadsafe_function(central->tsfn_connect_fail, event, js_threadsafe_function_nonblocking);
    }
  }
}

static void
bare_bluetooth_android_peripheral__on_services_discover(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_services_discover_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_uint32(env, event->count, &argv[0]);
  assert(err == 0);

  if (!event->error.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[1]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  delete event;

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_characteristics_discover(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_characteristics_discover_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[3];

  err = js_create_external(env, event->service, bare_bluetooth_android__on_release<bare_bluetooth_android_service_handle_t>, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_uint32(env, event->count, &argv[1]);
  assert(err == 0);

  if (!event->error.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[2]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[2]);
    assert(err == 0);
  }

  delete event;

  js_call_function(env, receiver, function, 3, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_read(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_read_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[4];

  err = js_create_external(env, event->characteristic, bare_bluetooth_android__on_release<bare_bluetooth_android_characteristic_handle_t>, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->uuid.c_str()), -1, &argv[1]);
  assert(err == 0);

  if (!event->data.empty()) {
    js_value_t *arraybuffer;
    void *buf;
    err = js_create_arraybuffer(env, event->data.size(), &buf, &arraybuffer);
    assert(err == 0);
    memcpy(buf, event->data.data(), event->data.size());

    err = js_create_typedarray(env, js_uint8array, event->data.size(), arraybuffer, 0, &argv[2]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[2]);
    assert(err == 0);
  }

  if (!event->error.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[3]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[3]);
    assert(err == 0);
  }

  delete event;

  js_call_function(env, receiver, function, 4, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_write(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_write_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[3];

  err = js_create_external(env, event->characteristic, bare_bluetooth_android__on_release<bare_bluetooth_android_characteristic_handle_t>, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->uuid.c_str()), -1, &argv[1]);
  assert(err == 0);

  if (!event->error.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[2]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[2]);
    assert(err == 0);
  }

  delete event;

  js_call_function(env, receiver, function, 3, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_notify(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_notify_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[4];

  err = js_create_external(env, event->characteristic, bare_bluetooth_android__on_release<bare_bluetooth_android_characteristic_handle_t>, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->uuid.c_str()), -1, &argv[1]);
  assert(err == 0);

  if (!event->data.empty()) {
    js_value_t *arraybuffer;
    void *buf;
    err = js_create_arraybuffer(env, event->data.size(), &buf, &arraybuffer);
    assert(err == 0);
    memcpy(buf, event->data.data(), event->data.size());

    err = js_create_typedarray(env, js_uint8array, event->data.size(), arraybuffer, 0, &argv[2]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[2]);
    assert(err == 0);
  }

  if (!event->error.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[3]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[3]);
    assert(err == 0);
  }

  delete event;

  js_call_function(env, receiver, function, 4, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_notify_state(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_notify_state_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[4];

  err = js_create_external(env, event->characteristic, bare_bluetooth_android__on_release<bare_bluetooth_android_characteristic_handle_t>, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->uuid.c_str()), -1, &argv[1]);
  assert(err == 0);

  err = js_get_boolean(env, event->is_notifying, &argv[2]);
  assert(err == 0);

  if (!event->error.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[3]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[3]);
    assert(err == 0);
  }

  delete event;

  js_call_function(env, receiver, function, 4, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_channel_open(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_channel_open_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[3];

  if (event->channel) {
    err = js_create_external(env, event->channel, bare_bluetooth_android__on_release<bare_bluetooth_android_socket_handle_t>, NULL, &argv[0]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[0]);
    assert(err == 0);
  }

  if (!event->error.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[1]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  err = js_create_uint32(env, event->psm, &argv[2]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 3, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_mtu_changed(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_peripheral_mtu_changed_t *>(data);
  auto *peripheral = static_cast<bare_bluetooth_android_peripheral_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_int32(env, event->mtu, &argv[0]);
  assert(err == 0);

  if (!event->error.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[1]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  delete event;

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static js_value_t *
bare_bluetooth_android_peripheral_init(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 10;
  js_value_t *argv[10];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 10);

  void *gatt_ptr;
  err = js_get_value_external(env, argv[0], &gatt_ptr);
  assert(err == 0);

  auto *gatt_handle = static_cast<bare_bluetooth_android_gatt_handle_t *>(gatt_ptr);

  auto *peripheral = new bare_bluetooth_android_peripheral_t();
  peripheral->env = env;
  peripheral->destroyed = false;
  peripheral->l2cap_connecting = false;

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  peripheral->gatt = java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGatt">>(jenv, gatt_handle->handle);

  auto gatt_obj = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto get_device = gatt_obj.get_class().get_method<java_object_t<"android/bluetooth/BluetoothDevice">()>("getDevice");
  auto device_local = get_device(gatt_obj);
  peripheral->device = java_global_ref_t<java_object_t<"android/bluetooth/BluetoothDevice">>(jenv, device_local);

  {
    auto address = device_local.get_class().get_method<std::string()>("getAddress")(device_local);
    bare_bluetooth_android_peripherals[address] = peripheral;
  }

  err = js_create_reference(env, argv[1], 1, &peripheral->ctx);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[2], 0, 1, NULL, NULL, peripheral, bare_bluetooth_android_peripheral__on_services_discover, &peripheral->tsfn_services_discover);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[3], 0, 1, NULL, NULL, peripheral, bare_bluetooth_android_peripheral__on_characteristics_discover, &peripheral->tsfn_characteristics_discover);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[4], 0, 1, NULL, NULL, peripheral, bare_bluetooth_android_peripheral__on_read, &peripheral->tsfn_read);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[5], 0, 1, NULL, NULL, peripheral, bare_bluetooth_android_peripheral__on_write, &peripheral->tsfn_write);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[6], 0, 1, NULL, NULL, peripheral, bare_bluetooth_android_peripheral__on_notify, &peripheral->tsfn_notify);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[7], 0, 1, NULL, NULL, peripheral, bare_bluetooth_android_peripheral__on_notify_state, &peripheral->tsfn_notify_state);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[8], 0, 1, NULL, NULL, peripheral, bare_bluetooth_android_peripheral__on_channel_open, &peripheral->tsfn_channel_open);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[9], 0, 1, NULL, NULL, peripheral, bare_bluetooth_android_peripheral__on_mtu_changed, &peripheral->tsfn_mtu_changed);
  assert(err == 0);

  js_value_t *handle;
  err = js_create_external(env, peripheral, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_bluetooth_android_peripheral_id(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto device = java_object_t<"android/bluetooth/BluetoothDevice">(jenv, peripheral->device);
  auto get_address = device.get_class().get_method<std::string()>("getAddress");
  std::string address = get_address(device);

  js_value_t *result;
  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(address.c_str()), address.size(), &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_peripheral_name(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto device = java_object_t<"android/bluetooth/BluetoothDevice">(jenv, peripheral->device);
  auto get_name = device.get_class().get_method<java_object_t<"java/lang/String">()>("getName");
  auto name_obj = get_name(device);

  if (static_cast<jobject>(name_obj) == nullptr) {
    js_value_t *result;
    err = js_get_null(env, &result);
    assert(err == 0);
    return result;
  }

  auto name = java_string_t(jenv, name_obj);
  js_value_t *result;
  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(static_cast<const char *>(name)), -1, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_peripheral_discover_services(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto discover = gatt.get_class().get_method<bool()>("discoverServices");
  discover(gatt);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_discover_characteristics(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  void *service_ptr;
  err = js_get_value_external(env, argv[1], &service_ptr);
  assert(err == 0);

  auto *service_handle = static_cast<bare_bluetooth_android_service_handle_t *>(service_ptr);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  int service_index = -1;
  for (size_t i = 0; i < peripheral->services.size(); i++) {
    if (static_cast<JNIEnv *>(jenv)->IsSameObject(peripheral->services[i], service_handle->handle)) {
      service_index = static_cast<int>(i);
      break;
    }
  }

  auto *event = new bare_bluetooth_android_peripheral_characteristics_discover_t();
  event->service = service_handle;

  if (service_index >= 0 && static_cast<size_t>(service_index) < peripheral->service_characteristics.size()) {
    event->count = static_cast<uint32_t>(peripheral->service_characteristics[service_index].size());
    event->error = {};
  } else {
    event->count = 0;
    event->error = "Service not found in cache";
  }

  js_call_threadsafe_function(peripheral->tsfn_characteristics_discover, event, js_threadsafe_function_nonblocking);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_read(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  void *char_ptr;
  err = js_get_value_external(env, argv[1], &char_ptr);
  assert(err == 0);

  auto *char_handle = static_cast<bare_bluetooth_android_characteristic_handle_t *>(char_ptr);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, char_handle->handle);
  auto read_characteristic = gatt.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattCharacteristic">)>("readCharacteristic");
  read_characteristic(gatt, characteristic);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_write(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 4;
  js_value_t *argv[4];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 4);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  void *char_ptr;
  err = js_get_value_external(env, argv[1], &char_ptr);
  assert(err == 0);

  auto *char_handle = static_cast<bare_bluetooth_android_characteristic_handle_t *>(char_ptr);

  js_value_t *arraybuffer;
  size_t offset, length;
  err = js_get_typedarray_info(env, argv[2], NULL, nullptr, &length, &arraybuffer, &offset);
  assert(err == 0);

  void *buf;
  err = js_get_arraybuffer_info(env, arraybuffer, &buf, NULL);
  assert(err == 0);

  uint8_t *data = static_cast<uint8_t *>(buf) + offset;

  bool with_response;
  err = js_get_value_bool(env, argv[3], &with_response);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, char_handle->handle);

  auto set_write_type = characteristic.get_class().get_method<void(int)>("setWriteType");
  set_write_type(characteristic, with_response ? 2 : 1);

  auto byte_array = bare_bluetooth_android_make_byte_array(jenv, data, length);
  auto set_value = characteristic.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
  set_value(characteristic, byte_array);

  auto write_characteristic = gatt.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattCharacteristic">)>("writeCharacteristic");
  write_characteristic(gatt, characteristic);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_subscribe(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  void *char_ptr;
  err = js_get_value_external(env, argv[1], &char_ptr);
  assert(err == 0);

  auto *char_handle = static_cast<bare_bluetooth_android_characteristic_handle_t *>(char_ptr);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, char_handle->handle);

  auto set_notify = gatt.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattCharacteristic">, bool)>("setCharacteristicNotification");
  set_notify(gatt, characteristic, true);

  auto uuid_class = java_class_t<"java/util/UUID">(jenv);
  auto from_string = uuid_class.get_static_method<java_object_t<"java/util/UUID">(std::string)>("fromString");
  auto cccd_uuid = from_string(std::string("00002902-0000-1000-8000-00805f9b34fb"));

  auto get_descriptor = characteristic.get_class().get_method<java_object_t<"android/bluetooth/BluetoothGattDescriptor">(java_object_t<"java/util/UUID">)>("getDescriptor");
  auto descriptor = get_descriptor(characteristic, cccd_uuid);

  if (static_cast<jobject>(descriptor) != nullptr) {
    jbyte enable_bytes[] = {0x01, 0x00};
    auto enable_value = bare_bluetooth_android_make_byte_array(jenv, enable_bytes, 2);

    auto set_descriptor_value = descriptor.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
    set_descriptor_value(descriptor, enable_value);

    auto write_descriptor = gatt.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattDescriptor">)>("writeDescriptor");
    write_descriptor(gatt, descriptor);
  }

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_unsubscribe(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  void *char_ptr;
  err = js_get_value_external(env, argv[1], &char_ptr);
  assert(err == 0);

  auto *char_handle = static_cast<bare_bluetooth_android_characteristic_handle_t *>(char_ptr);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, char_handle->handle);

  auto set_notify = gatt.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattCharacteristic">, bool)>("setCharacteristicNotification");
  set_notify(gatt, characteristic, false);

  auto uuid_class = java_class_t<"java/util/UUID">(jenv);
  auto from_string = uuid_class.get_static_method<java_object_t<"java/util/UUID">(std::string)>("fromString");
  auto cccd_uuid = from_string(std::string("00002902-0000-1000-8000-00805f9b34fb"));

  auto get_descriptor = characteristic.get_class().get_method<java_object_t<"android/bluetooth/BluetoothGattDescriptor">(java_object_t<"java/util/UUID">)>("getDescriptor");
  auto descriptor = get_descriptor(characteristic, cccd_uuid);

  if (static_cast<jobject>(descriptor) != nullptr) {
    jbyte disable_bytes[] = {0x00, 0x00};
    auto disable_value = bare_bluetooth_android_make_byte_array(jenv, disable_bytes, 2);

    auto set_descriptor_value = descriptor.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
    set_descriptor_value(descriptor, disable_value);

    auto write_descriptor = gatt.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattDescriptor">)>("writeDescriptor");
    write_descriptor(gatt, descriptor);
  }

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_request_mtu(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  int32_t mtu;
  err = js_get_value_int32(env, argv[1], &mtu);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto request_mtu = gatt.get_class().get_method<bool(int)>("requestMtu");
  request_mtu(gatt, mtu);

  return NULL;
}

struct bare_bluetooth_android_peripheral_l2cap_open_ctx_t {
  bare_bluetooth_android_peripheral_t *peripheral;
  uint32_t psm;
};

static void
bare_bluetooth_android_peripheral__l2cap_open_thread(void *arg) {
  auto *ctx = static_cast<bare_bluetooth_android_peripheral_l2cap_open_ctx_t *>(arg);
  auto *peripheral = ctx->peripheral;
  uint32_t psm = ctx->psm;
  free(ctx);

  auto jenv = bare_bluetooth_android_jvm().attach_current_thread();

  auto device = java_object_t<"android/bluetooth/BluetoothDevice">(jenv, peripheral->device);
  auto create_channel = device.get_class().get_method<java_object_t<"android/bluetooth/BluetoothSocket">(int)>("createInsecureL2capChannel");
  auto socket = create_channel(device, static_cast<int>(psm));

  if (bare_bluetooth_android_check_exception(jenv) || static_cast<jobject>(socket) == nullptr) {
    if (!peripheral->destroyed) {
      auto *event = new bare_bluetooth_android_peripheral_channel_open_t();
      event->channel = NULL;
      event->error = "Failed to create L2CAP channel";
      event->psm = psm;
      js_call_threadsafe_function(peripheral->tsfn_channel_open, event, js_threadsafe_function_nonblocking);
    }
    peripheral->l2cap_connecting = false;
    return;
  }

  auto connect = socket.get_class().get_method<void()>("connect");
  connect(socket);

  if (bare_bluetooth_android_check_exception(jenv)) {
    if (!peripheral->destroyed) {
      auto *event = new bare_bluetooth_android_peripheral_channel_open_t();
      event->channel = NULL;
      event->error = "L2CAP connect failed";
      event->psm = psm;
      js_call_threadsafe_function(peripheral->tsfn_channel_open, event, js_threadsafe_function_nonblocking);
    }
    peripheral->l2cap_connecting = false;
    return;
  }

  if (!peripheral->destroyed) {
    auto *event = new bare_bluetooth_android_peripheral_channel_open_t();
    event->channel = new bare_bluetooth_android_socket_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothSocket">>(jenv, socket)};
    event->error = {};
    event->psm = psm;
    js_call_threadsafe_function(peripheral->tsfn_channel_open, event, js_threadsafe_function_nonblocking);
  }

  peripheral->l2cap_connecting = false;
}

static js_value_t *
bare_bluetooth_android_peripheral_open_l2cap_channel(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  uint32_t psm;
  err = js_get_value_uint32(env, argv[1], &psm);
  assert(err == 0);

  auto *ctx = static_cast<bare_bluetooth_android_peripheral_l2cap_open_ctx_t *>(malloc(sizeof(bare_bluetooth_android_peripheral_l2cap_open_ctx_t)));
  ctx->peripheral = peripheral;
  ctx->psm = psm;

  peripheral->l2cap_connecting = true;
  err = uv_thread_create(&peripheral->l2cap_thread, bare_bluetooth_android_peripheral__l2cap_open_thread, ctx);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_destroy(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  if (peripheral->destroyed) return NULL;
  peripheral->destroyed = true;

  if (peripheral->l2cap_connecting) {
    uv_thread_join(&peripheral->l2cap_thread);
  }

  err = js_delete_reference(env, peripheral->ctx);
  assert(err == 0);

  js_release_threadsafe_function(peripheral->tsfn_mtu_changed, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_channel_open, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_notify_state, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_notify, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_write, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_read, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_characteristics_discover, js_threadsafe_function_abort);
  js_release_threadsafe_function(peripheral->tsfn_services_discover, js_threadsafe_function_abort);

  delete peripheral;

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_service_count(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  js_value_t *result;
  err = js_create_uint32(env, static_cast<uint32_t>(peripheral->services.size()), &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_peripheral_service_at_index(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&peripheral));
  assert(err == 0);

  uint32_t index;
  err = js_get_value_uint32(env, argv[1], &index);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto *service_handle = new bare_bluetooth_android_service_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattService">>(jenv, peripheral->services[index])};

  js_value_t *result;
  err = js_create_external(env, service_handle, bare_bluetooth_android__on_release<bare_bluetooth_android_service_handle_t>, NULL, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_service_key(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  void *handle;
  err = js_get_value_external(env, argv[0], &handle);
  assert(err == 0);

  char key[32];
  snprintf(key, sizeof(key), "%p", handle);

  js_value_t *result;
  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(key), -1, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_service_uuid(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  void *handle;
  err = js_get_value_external(env, argv[0], &handle);
  assert(err == 0);

  auto *service_handle = static_cast<bare_bluetooth_android_service_handle_t *>(handle);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto service = java_object_t<"android/bluetooth/BluetoothGattService">(jenv, service_handle->handle);
  auto get_uuid = service.get_class().get_method<java_object_t<"java/util/UUID">()>("getUuid");
  auto uuid_obj = get_uuid(service);
  auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
  std::string uuid_str = to_string(uuid_obj);

  js_value_t *result;
  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(uuid_str.c_str()), uuid_str.size(), &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_service_characteristic_count(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  void *handle;
  err = js_get_value_external(env, argv[0], &handle);
  assert(err == 0);

  auto *service_handle = static_cast<bare_bluetooth_android_service_handle_t *>(handle);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto service = java_object_t<"android/bluetooth/BluetoothGattService">(jenv, service_handle->handle);
  auto get_characteristics = service.get_class().get_method<java_object_t<"java/util/List">()>("getCharacteristics");
  auto list = get_characteristics(service);
  auto list_size = list.get_class().get_method<int()>("size");
  int count = list_size(list);

  js_value_t *result;
  err = js_create_uint32(env, static_cast<uint32_t>(count), &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_service_characteristic_at_index(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  void *handle;
  err = js_get_value_external(env, argv[0], &handle);
  assert(err == 0);

  auto *service_handle = static_cast<bare_bluetooth_android_service_handle_t *>(handle);

  uint32_t index;
  err = js_get_value_uint32(env, argv[1], &index);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto service = java_object_t<"android/bluetooth/BluetoothGattService">(jenv, service_handle->handle);
  auto get_characteristics = service.get_class().get_method<java_object_t<"java/util/List">()>("getCharacteristics");
  auto list = get_characteristics(service);
  auto list_get = list.get_class().get_method<java_object_t<"java/lang/Object">(int)>("get");
  auto char_obj = list_get(list, static_cast<int>(index));

  auto *char_handle = new bare_bluetooth_android_characteristic_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">>(jenv, char_obj)};

  js_value_t *result;
  err = js_create_external(env, char_handle, bare_bluetooth_android__on_release<bare_bluetooth_android_characteristic_handle_t>, NULL, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_characteristic_key(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  void *handle;
  err = js_get_value_external(env, argv[0], &handle);
  assert(err == 0);

  char key[32];
  snprintf(key, sizeof(key), "%p", handle);

  js_value_t *result;
  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(key), -1, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_characteristic_uuid(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  void *handle;
  err = js_get_value_external(env, argv[0], &handle);
  assert(err == 0);

  auto *char_handle = static_cast<bare_bluetooth_android_characteristic_handle_t *>(handle);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, char_handle->handle);
  auto get_uuid = characteristic.get_class().get_method<java_object_t<"java/util/UUID">()>("getUuid");
  auto uuid_obj = get_uuid(characteristic);
  auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
  std::string uuid_str = to_string(uuid_obj);

  js_value_t *result;
  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(uuid_str.c_str()), uuid_str.size(), &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_characteristic_properties(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  void *handle;
  err = js_get_value_external(env, argv[0], &handle);
  assert(err == 0);

  auto *char_handle = static_cast<bare_bluetooth_android_characteristic_handle_t *>(handle);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, char_handle->handle);
  auto get_properties = characteristic.get_class().get_method<int()>("getProperties");
  int properties = get_properties(characteristic);

  js_value_t *result;
  err = js_create_int32(env, properties, &result);
  assert(err == 0);

  return result;
}

static void
bare_bluetooth_android_on_services_discovered(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/GattCallback"> self, long native_ptr, java_object_t<"android/bluetooth/BluetoothGatt"> gatt, int status) {
  auto *peripheral = bare_bluetooth_android_find_peripheral(env, gatt);
  if (!peripheral) return;

  peripheral->service_characteristics.clear();
  peripheral->services.clear();

  if (status != 0) {
    auto *event = new bare_bluetooth_android_peripheral_services_discover_t();
    event->count = 0;
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = error_buf;

    js_call_threadsafe_function(peripheral->tsfn_services_discover, event, js_threadsafe_function_nonblocking);
    return;
  }

  auto gatt_obj = java_object_t<"android/bluetooth/BluetoothGatt">(env, gatt);
  auto get_services = gatt_obj.get_class().get_method<java_object_t<"java/util/List">()>("getServices");
  auto services_list = get_services(gatt_obj);
  auto list_size = services_list.get_class().get_method<int()>("size");
  auto list_get = services_list.get_class().get_method<java_object_t<"java/lang/Object">(int)>("get");
  int service_count = list_size(services_list);

  for (int i = 0; i < service_count; i++) {
    auto service_obj = list_get(services_list, i);
    peripheral->services.emplace_back(env, service_obj);

    auto service_typed = java_object_t<"android/bluetooth/BluetoothGattService">(env, service_obj);
    auto get_chars = service_typed.get_class().get_method<java_object_t<"java/util/List">()>("getCharacteristics");
    auto chars_list = get_chars(service_typed);
    auto chars_size = chars_list.get_class().get_method<int()>("size");
    auto chars_get = chars_list.get_class().get_method<java_object_t<"java/lang/Object">(int)>("get");
    int char_count = chars_size(chars_list);

    std::vector<java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">>> chars;
    for (int j = 0; j < char_count; j++) {
      auto char_obj = chars_get(chars_list, j);
      chars.emplace_back(env, char_obj);
    }
    peripheral->service_characteristics.push_back(std::move(chars));
  }

  auto *event = new bare_bluetooth_android_peripheral_services_discover_t();
  event->count = static_cast<uint32_t>(service_count);
  event->error = {};

  js_call_threadsafe_function(peripheral->tsfn_services_discover, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_on_characteristic_read(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/GattCallback"> self, long native_ptr, java_object_t<"android/bluetooth/BluetoothGatt"> gatt, java_object_t<"android/bluetooth/BluetoothGattCharacteristic"> characteristic, java_array_t<unsigned char> value, int status) {
  auto *peripheral = bare_bluetooth_android_find_peripheral(env, gatt);
  if (!peripheral) return;

  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);

  auto *event = new bare_bluetooth_android_peripheral_read_t();
  event->characteristic = new bare_bluetooth_android_characteristic_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">>(env, characteristic)};
  event->uuid = uuid_str;

  if (status == 0 && static_cast<jobject>(value) != nullptr) {
    event->data = bare_bluetooth_android_copy_byte_array(env, value);
  }

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = error_buf;
  } else {
    event->error = {};
  }

  js_call_threadsafe_function(peripheral->tsfn_read, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_on_characteristic_write(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/GattCallback"> self, long native_ptr, java_object_t<"android/bluetooth/BluetoothGatt"> gatt, java_object_t<"android/bluetooth/BluetoothGattCharacteristic"> characteristic, int status) {
  auto *peripheral = bare_bluetooth_android_find_peripheral(env, gatt);
  if (!peripheral) return;

  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);

  auto *event = new bare_bluetooth_android_peripheral_write_t();
  event->characteristic = new bare_bluetooth_android_characteristic_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">>(env, characteristic)};
  event->uuid = uuid_str;

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = error_buf;
  } else {
    event->error = {};
  }

  js_call_threadsafe_function(peripheral->tsfn_write, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_on_characteristic_changed(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/GattCallback"> self, long native_ptr, java_object_t<"android/bluetooth/BluetoothGatt"> gatt, java_object_t<"android/bluetooth/BluetoothGattCharacteristic"> characteristic, java_array_t<unsigned char> value) {
  auto *peripheral = bare_bluetooth_android_find_peripheral(env, gatt);
  if (!peripheral) return;

  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);

  auto *event = new bare_bluetooth_android_peripheral_notify_t();
  event->characteristic = new bare_bluetooth_android_characteristic_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">>(env, characteristic)};
  event->uuid = uuid_str;
  event->error = {};

  if (static_cast<jobject>(value) != nullptr) {
    event->data = bare_bluetooth_android_copy_byte_array(env, value);
  }

  js_call_threadsafe_function(peripheral->tsfn_notify, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_on_descriptor_write(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/GattCallback"> self, long native_ptr, java_object_t<"android/bluetooth/BluetoothGatt"> gatt, java_object_t<"android/bluetooth/BluetoothGattDescriptor"> descriptor, int status) {
  auto *peripheral = bare_bluetooth_android_find_peripheral(env, gatt);
  if (!peripheral) return;

  auto desc_uuid = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattDescriptor">(env, descriptor);

  bool is_cccd = desc_uuid == "00002902-0000-1000-8000-00805f9b34fb";
  if (!is_cccd) return;

  auto desc_obj = java_object_t<"android/bluetooth/BluetoothGattDescriptor">(env, descriptor);
  auto characteristic = desc_obj.get_class().get_method<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">()>("getCharacteristic")(desc_obj);
  auto char_uuid = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);

  auto value = desc_obj.get_class().get_method<java_object_t<"java/lang/Object">()>("getValue")(desc_obj);
  bool is_notifying = false;

  if (static_cast<jobject>(value) != nullptr) {
    auto arr = java_array_t<unsigned char>(env, value);
    if (arr.size() >= 1) {
      is_notifying = (arr[static_cast<size_t>(0)] != 0);
    }
  }

  auto *event = new bare_bluetooth_android_peripheral_notify_state_t();
  event->characteristic = new bare_bluetooth_android_characteristic_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">>(env, characteristic)};
  event->uuid = char_uuid;
  event->is_notifying = is_notifying;

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = error_buf;
  } else {
    event->error = {};
  }

  js_call_threadsafe_function(peripheral->tsfn_notify_state, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_on_mtu_changed(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/GattCallback"> self, long native_ptr, java_object_t<"android/bluetooth/BluetoothGatt"> gatt, int mtu, int status) {
  auto *peripheral = bare_bluetooth_android_find_peripheral(env, gatt);
  if (!peripheral) return;

  auto *event = new bare_bluetooth_android_peripheral_mtu_changed_t();
  event->mtu = mtu;

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = error_buf;
  } else {
    event->error = {};
  }

  js_call_threadsafe_function(peripheral->tsfn_mtu_changed, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_server__on_state_change(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_state_change_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[1];
  err = js_create_int32(env, event->state, &argv[0]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 1, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_add_service(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_add_service_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->uuid.c_str()), -1, &argv[0]);
  assert(err == 0);

  if (!event->error.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[1]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  delete event;

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_read_request(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_read_request_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[4];

  err = js_create_external(env, event->device, bare_bluetooth_android__on_release<bare_bluetooth_android_device_handle_t>, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_int32(env, event->request_id, &argv[1]);
  assert(err == 0);

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->characteristic_uuid.c_str()), -1, &argv[2]);
  assert(err == 0);

  err = js_create_int32(env, event->offset, &argv[3]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 4, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_write_request(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_write_request_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[6];

  err = js_create_external(env, event->device, bare_bluetooth_android__on_release<bare_bluetooth_android_device_handle_t>, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_int32(env, event->request_id, &argv[1]);
  assert(err == 0);

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->characteristic_uuid.c_str()), -1, &argv[2]);
  assert(err == 0);

  err = js_create_int32(env, event->offset, &argv[3]);
  assert(err == 0);

  if (!event->data.empty()) {
    js_value_t *arraybuffer;
    void *buf;
    err = js_create_arraybuffer(env, event->data.size(), &buf, &arraybuffer);
    assert(err == 0);
    memcpy(buf, event->data.data(), event->data.size());

    err = js_create_typedarray(env, js_uint8array, event->data.size(), arraybuffer, 0, &argv[4]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[4]);
    assert(err == 0);
  }

  err = js_get_boolean(env, event->response_needed, &argv[5]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 6, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_subscribe(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_subscribe_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->device_address.c_str()), -1, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->characteristic_uuid.c_str()), -1, &argv[1]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_unsubscribe(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_unsubscribe_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->device_address.c_str()), -1, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->characteristic_uuid.c_str()), -1, &argv[1]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_advertise_error(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_advertise_error_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_int32(env, event->error_code, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[1]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_channel_publish(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_channel_publish_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_uint32(env, event->psm, &argv[0]);
  assert(err == 0);

  if (!event->error.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[1]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  delete event;

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_channel_open(js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = static_cast<bare_bluetooth_android_server_channel_open_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[3];

  if (event->channel) {
    err = js_create_external(env, event->channel, bare_bluetooth_android__on_release<bare_bluetooth_android_socket_handle_t>, NULL, &argv[0]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[0]);
    assert(err == 0);
  }

  if (!event->error.empty()) {
    err = js_create_string_utf8(env, reinterpret_cast<const utf8_t *>(event->error.c_str()), -1, &argv[1]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  err = js_create_uint32(env, event->psm, &argv[2]);
  assert(err == 0);

  delete event;

  js_call_function(env, receiver, function, 3, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_connection_state(js_env_t *env, js_value_t *function, void *context, void *data) {
  auto *event = static_cast<bare_bluetooth_android_server_connection_state_t *>(data);
  auto *server = static_cast<bare_bluetooth_android_server_t *>(context);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  if (event->new_state == 2) {
    server->connected_devices.emplace(event->address, java_global_ref_t<java_object_t<"android/bluetooth/BluetoothDevice">>(jenv, event->device->handle));
    delete event->device;
  } else if (event->new_state == 0) {
    server->connected_devices.erase(event->address);

    for (auto &pair : server->subscriptions) {
      pair.second.erase(std::string(event->address));
    }

    if (event->device) {
      delete event->device;
    }
  }

  delete event;
}

static js_value_t *
bare_bluetooth_android_server_init(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 10;
  js_value_t *argv[10];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 10);

  auto *server = new bare_bluetooth_android_server_t();
  server->env = env;

  err = js_create_reference(env, argv[0], 1, &server->ctx);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[1], 0, 1, NULL, NULL, server, bare_bluetooth_android_server__on_state_change, &server->tsfn_state_change);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[2], 0, 1, NULL, NULL, server, bare_bluetooth_android_server__on_add_service, &server->tsfn_add_service);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[3], 0, 1, NULL, NULL, server, bare_bluetooth_android_server__on_read_request, &server->tsfn_read_request);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[4], 0, 1, NULL, NULL, server, bare_bluetooth_android_server__on_write_request, &server->tsfn_write_request);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[5], 0, 1, NULL, NULL, server, bare_bluetooth_android_server__on_subscribe, &server->tsfn_subscribe);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[6], 0, 1, NULL, NULL, server, bare_bluetooth_android_server__on_unsubscribe, &server->tsfn_unsubscribe);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[7], 0, 1, NULL, NULL, server, bare_bluetooth_android_server__on_advertise_error, &server->tsfn_advertise_error);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[8], 0, 1, NULL, NULL, server, bare_bluetooth_android_server__on_channel_publish, &server->tsfn_channel_publish);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[9], 0, 1, NULL, NULL, server, bare_bluetooth_android_server__on_channel_open, &server->tsfn_channel_open);
  assert(err == 0);

  {
    js_value_t *noop;
    err = js_create_function(env, "noop", -1, [](js_env_t *, js_callback_info_t *) -> js_value_t * { return NULL; }, NULL, &noop);
    assert(err == 0);

    err = js_create_threadsafe_function(env, noop, 0, 1, NULL, NULL, server, bare_bluetooth_android_server__on_connection_state, &server->tsfn_server_connection_state);
    assert(err == 0);
  }

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto activity = bare_bluetooth_android_get_context(jenv);
  auto get_system_service = activity.get_class().get_method<java_object_t<"java/lang/Object">(std::string)>("getSystemService");
  auto manager_obj = get_system_service(activity, std::string("bluetooth"));
  auto bt_manager = java_object_t<"android/bluetooth/BluetoothManager">(jenv, manager_obj);

  auto callback_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/GattServerCallback">();
  auto callback_local = callback_class(reinterpret_cast<long>(server));

  auto open_gatt_server = bt_manager.get_class().get_method<java_object_t<"android/bluetooth/BluetoothGattServer">(java_object_t<"android/content/Context">, java_object_t<"android/bluetooth/BluetoothGattServerCallback">)>("openGattServer");
  auto gatt_server_local = open_gatt_server(bt_manager, activity, java_object_t<"android/bluetooth/BluetoothGattServerCallback">(jenv, callback_local));

  server->gatt_server = java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattServer">>(jenv, gatt_server_local);

  auto get_adapter = bt_manager.get_class().get_method<java_object_t<"android/bluetooth/BluetoothAdapter">()>("getAdapter");
  auto adapter = get_adapter(bt_manager);
  auto get_advertiser = adapter.get_class().get_method<java_object_t<"android/bluetooth/le/BluetoothLeAdvertiser">()>("getBluetoothLeAdvertiser");
  auto advertiser_local = get_advertiser(adapter);

  server->advertiser = java_global_ref_t<java_object_t<"android/bluetooth/le/BluetoothLeAdvertiser">>(jenv, advertiser_local);

  auto get_state = adapter.get_class().get_method<int()>("getState");
  int android_state = get_state(adapter);

  auto *state_event = new bare_bluetooth_android_server_state_change_t();
  state_event->state = android_state;
  js_call_threadsafe_function(server->tsfn_state_change, state_event, js_threadsafe_function_nonblocking);

  js_value_t *handle;
  err = js_create_external(env, server, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_bluetooth_android_server_add_service(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&server));
  assert(err == 0);

  void *service_ptr;
  err = js_get_value_external(env, argv[1], &service_ptr);
  assert(err == 0);

  auto *service_handle = static_cast<bare_bluetooth_android_service_handle_t *>(service_ptr);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt_server = java_object_t<"android/bluetooth/BluetoothGattServer">(jenv, server->gatt_server);
  auto service = java_object_t<"android/bluetooth/BluetoothGattService">(jenv, service_handle->handle);

  auto add_service = gatt_server.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattService">)>("addService");
  bool result = add_service(gatt_server, service);

  auto get_chars = service.get_class().get_method<java_object_t<"java/util/List">()>("getCharacteristics");
  auto chars_list = get_chars(service);
  auto list_size = chars_list.get_class().get_method<int()>("size");
  auto list_get = chars_list.get_class().get_method<java_object_t<"java/lang/Object">(int)>("get");
  int count = list_size(chars_list);

  for (int i = 0; i < count; i++) {
    auto char_obj = list_get(chars_list, i);
    auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, char_obj);
    auto get_uuid = characteristic.get_class().get_method<java_object_t<"java/util/UUID">()>("getUuid");
    auto uuid_obj = get_uuid(characteristic);
    auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
    std::string uuid_str = to_string(uuid_obj);

    server->characteristics.emplace(uuid_str, java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">>(jenv, char_obj));
  }

  if (!result) {
    auto get_uuid = service.get_class().get_method<java_object_t<"java/util/UUID">()>("getUuid");
    auto uuid_obj = get_uuid(service);
    auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
    std::string uuid_str = to_string(uuid_obj);

    auto *event = new bare_bluetooth_android_server_add_service_t();
    event->uuid = uuid_str;
    event->error = "Failed to add service";
    js_call_threadsafe_function(server->tsfn_add_service, event, js_threadsafe_function_nonblocking);
  }

  return NULL;
}

static js_value_t *
bare_bluetooth_android_server_start_advertising(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 3;
  js_value_t *argv[3];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 3);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&server));
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto settings_builder_class = java_class_t<"android/bluetooth/le/AdvertiseSettings$Builder">(jenv);
  auto settings_builder = settings_builder_class();
  auto set_mode = settings_builder.get_class().get_method<java_object_t<"android/bluetooth/le/AdvertiseSettings$Builder">(int)>("setAdvertiseMode");
  set_mode(settings_builder, 1);
  auto set_connectable = settings_builder.get_class().get_method<java_object_t<"android/bluetooth/le/AdvertiseSettings$Builder">(bool)>("setConnectable");
  set_connectable(settings_builder, true);
  auto build_settings = settings_builder.get_class().get_method<java_object_t<"android/bluetooth/le/AdvertiseSettings">()>("build");
  auto settings = build_settings(settings_builder);

  auto data_builder_class = java_class_t<"android/bluetooth/le/AdvertiseData$Builder">(jenv);
  auto data_builder = data_builder_class();

  js_value_type_t type;
  err = js_typeof(env, argv[2], &type);
  assert(err == 0);

  if (type != js_null && type != js_undefined) {
    uint32_t uuid_count;
    err = js_get_array_length(env, argv[2], &uuid_count);
    assert(err == 0);

    auto add_service_uuid = data_builder.get_class().get_method<java_object_t<"android/bluetooth/le/AdvertiseData$Builder">(java_object_t<"android/os/ParcelUuid">)>("addServiceUuid");
    auto parcel_uuid_class = java_class_t<"android/os/ParcelUuid">(jenv);

    for (uint32_t i = 0; i < uuid_count; i++) {
      js_value_t *uuid_val;
      err = js_get_element(env, argv[2], i, &uuid_val);
      assert(err == 0);

      void *uuid_ptr;
      err = js_get_value_external(env, uuid_val, &uuid_ptr);
      assert(err == 0);

      auto *uuid_handle = static_cast<bare_bluetooth_android_uuid_handle_t *>(uuid_ptr);
      auto parcel_uuid = parcel_uuid_class(java_object_t<"java/util/UUID">(jenv, uuid_handle->handle));
      add_service_uuid(data_builder, parcel_uuid);
    }
  }

  js_value_type_t name_type;
  err = js_typeof(env, argv[1], &name_type);
  assert(err == 0);

  auto set_include_name = data_builder.get_class().get_method<java_object_t<"android/bluetooth/le/AdvertiseData$Builder">(bool)>("setIncludeDeviceName");
  set_include_name(data_builder, name_type != js_null && name_type != js_undefined);

  auto build_data = data_builder.get_class().get_method<java_object_t<"android/bluetooth/le/AdvertiseData">()>("build");
  auto adv_data = build_data(data_builder);

  auto adv_callback_class = bare_bluetooth_android_get_class_loader(jenv).load_class<"to/holepunch/bare/bluetooth/AdvertiseCallback">();
  auto adv_callback = adv_callback_class(reinterpret_cast<long>(server));
  server->advertise_callback = java_global_ref_t<java_object_t<"android/bluetooth/le/AdvertiseCallback">>(jenv, adv_callback);

  auto advertiser = java_object_t<"android/bluetooth/le/BluetoothLeAdvertiser">(jenv, server->advertiser);
  auto start_advertising = advertiser.get_class().get_method<void(java_object_t<"android/bluetooth/le/AdvertiseSettings">, java_object_t<"android/bluetooth/le/AdvertiseData">, java_object_t<"android/bluetooth/le/AdvertiseCallback">)>("startAdvertising");
  start_advertising(advertiser, settings, adv_data, java_object_t<"android/bluetooth/le/AdvertiseCallback">(jenv, server->advertise_callback));

  return NULL;
}

static js_value_t *
bare_bluetooth_android_server_stop_advertising(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&server));
  assert(err == 0);

  if (static_cast<jobject>(server->advertise_callback)) {
    auto jenv = bare_bluetooth_android_jvm().get_env().value();
    auto advertiser = java_object_t<"android/bluetooth/le/BluetoothLeAdvertiser">(jenv, server->advertiser);
    auto stop = advertiser.get_class().get_method<void(java_object_t<"android/bluetooth/le/AdvertiseCallback">)>("stopAdvertising");
    stop(advertiser, java_object_t<"android/bluetooth/le/AdvertiseCallback">(jenv, static_cast<jobject>(server->advertise_callback)));

    server->advertise_callback = {};
  }

  return NULL;
}

static js_value_t *
bare_bluetooth_android_server_respond_to_request(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 6;
  js_value_t *argv[6];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 6);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&server));
  assert(err == 0);

  void *device_ptr;
  err = js_get_value_external(env, argv[1], &device_ptr);
  assert(err == 0);

  auto *device_handle = static_cast<bare_bluetooth_android_device_handle_t *>(device_ptr);

  int32_t request_id;
  err = js_get_value_int32(env, argv[2], &request_id);
  assert(err == 0);

  int32_t result_code;
  err = js_get_value_int32(env, argv[3], &result_code);
  assert(err == 0);

  int32_t offset;
  err = js_get_value_int32(env, argv[4], &offset);
  assert(err == 0);

  js_value_type_t data_type;
  err = js_typeof(env, argv[5], &data_type);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto gatt_server = java_object_t<"android/bluetooth/BluetoothGattServer">(jenv, server->gatt_server);
  auto device = java_object_t<"android/bluetooth/BluetoothDevice">(jenv, device_handle->handle);

  java_array_t<unsigned char> response_data;

  if (data_type != js_null && data_type != js_undefined) {
    size_t length;
    js_value_t *arraybuffer;
    size_t typed_offset;
    err = js_get_typedarray_info(env, argv[5], NULL, NULL, &length, &arraybuffer, &typed_offset);
    assert(err == 0);

    void *buf;
    err = js_get_arraybuffer_info(env, arraybuffer, &buf, NULL);
    assert(err == 0);

    response_data = bare_bluetooth_android_make_byte_array(jenv, static_cast<uint8_t *>(buf) + typed_offset, length);
  }

  auto send_response = gatt_server.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothDevice">, int, int, int, java_array_t<unsigned char>)>("sendResponse");
  send_response(gatt_server, device, request_id, result_code, offset, response_data);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_server_update_value(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 3;
  js_value_t *argv[3];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 3);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&server));
  assert(err == 0);

  void *char_ptr;
  err = js_get_value_external(env, argv[1], &char_ptr);
  assert(err == 0);

  auto *char_handle = static_cast<bare_bluetooth_android_characteristic_handle_t *>(char_ptr);

  size_t length;
  js_value_t *arraybuffer;
  size_t offset;
  err = js_get_typedarray_info(env, argv[2], NULL, NULL, &length, &arraybuffer, &offset);
  assert(err == 0);

  void *buf;
  err = js_get_arraybuffer_info(env, arraybuffer, &buf, NULL);
  assert(err == 0);

  uint8_t *data = static_cast<uint8_t *>(buf) + offset;

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, char_handle->handle);
  auto gatt_server = java_object_t<"android/bluetooth/BluetoothGattServer">(jenv, server->gatt_server);

  auto byte_array = bare_bluetooth_android_make_byte_array(jenv, data, length);
  auto set_value = characteristic.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
  set_value(characteristic, byte_array);

  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(jenv, char_handle->handle);

  auto it = server->subscriptions.find(uuid_str);
  if (it != server->subscriptions.end()) {
    auto notify = gatt_server.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothDevice">, java_object_t<"android/bluetooth/BluetoothGattCharacteristic">, bool)>("notifyCharacteristicChanged");

    for (const auto &addr : it->second) {
      auto dev_it = server->connected_devices.find(addr);
      if (dev_it != server->connected_devices.end()) {
        notify(gatt_server, dev_it->second, characteristic, false);
      }
    }
  }

  js_value_t *result;
  err = js_get_boolean(env, true, &result);
  assert(err == 0);

  return result;
}

struct bare_bluetooth_android_server_accept_ctx_t {
  bare_bluetooth_android_server_t *server;
  bare_bluetooth_android_server_t::published_channel_t *channel;
};

static void
bare_bluetooth_android_server__accept_thread(void *arg) {
  auto *ctx = static_cast<bare_bluetooth_android_server_accept_ctx_t *>(arg);
  auto *server = ctx->server;
  auto *ch = ctx->channel;
  free(ctx);

  auto jenv = bare_bluetooth_android_jvm().attach_current_thread();

  auto ss = java_object_t<"android/bluetooth/BluetoothServerSocket">(jenv, ch->server_socket);
  auto accept_method = ss.get_class().get_method<java_object_t<"android/bluetooth/BluetoothSocket">()>("accept");

  while (ch->accepting) {
    auto client_socket = accept_method(ss);

    if (bare_bluetooth_android_check_exception(jenv) || static_cast<jobject>(client_socket) == nullptr) {
      break;
    }

    auto *event = new bare_bluetooth_android_server_channel_open_t();
    event->channel = new bare_bluetooth_android_socket_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothSocket">>(jenv, client_socket)};
    event->error = {};
    event->psm = ch->psm;

    js_call_threadsafe_function(server->tsfn_channel_open, event, js_threadsafe_function_nonblocking);
  }
}

static js_value_t *
bare_bluetooth_android_server_publish_channel(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&server));
  assert(err == 0);

  bool encrypted;
  err = js_get_value_bool(env, argv[1], &encrypted);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto activity = bare_bluetooth_android_get_context(jenv);
  auto get_system_service = activity.get_class().get_method<java_object_t<"java/lang/Object">(std::string)>("getSystemService");
  auto manager_obj = get_system_service(activity, std::string("bluetooth"));
  auto bt_manager = java_object_t<"android/bluetooth/BluetoothManager">(jenv, manager_obj);
  auto get_adapter = bt_manager.get_class().get_method<java_object_t<"android/bluetooth/BluetoothAdapter">()>("getAdapter");
  auto adapter = get_adapter(bt_manager);

  java_object_t<"android/bluetooth/BluetoothServerSocket"> server_socket_local;
  if (encrypted) {
    auto listen = adapter.get_class().get_method<java_object_t<"android/bluetooth/BluetoothServerSocket">()>("listenUsingL2capChannel");
    server_socket_local = listen(adapter);
  } else {
    auto listen = adapter.get_class().get_method<java_object_t<"android/bluetooth/BluetoothServerSocket">()>("listenUsingInsecureL2capChannel");
    server_socket_local = listen(adapter);
  }

  if (bare_bluetooth_android_check_exception(jenv) || static_cast<jobject>(server_socket_local) == nullptr) {
    auto *event = new bare_bluetooth_android_server_channel_publish_t();
    event->psm = 0;
    event->error = "Failed to create L2CAP server socket";
    js_call_threadsafe_function(server->tsfn_channel_publish, event, js_threadsafe_function_nonblocking);
    return NULL;
  }

  auto *ch = new bare_bluetooth_android_server_t::published_channel_t();
  ch->server_socket = java_global_ref_t<java_object_t<"android/bluetooth/BluetoothServerSocket">>(jenv, server_socket_local);

  auto get_psm = ch->server_socket.get_class().get_method<int()>("getPsm");
  int psm = get_psm(ch->server_socket);
  ch->accepting = true;
  ch->psm = static_cast<uint16_t>(psm);
  server->published_channels.push_back(ch);

  auto *event = new bare_bluetooth_android_server_channel_publish_t();
  event->psm = static_cast<uint16_t>(psm);
  event->error = {};
  js_call_threadsafe_function(server->tsfn_channel_publish, event, js_threadsafe_function_nonblocking);

  auto *ctx = static_cast<bare_bluetooth_android_server_accept_ctx_t *>(malloc(sizeof(bare_bluetooth_android_server_accept_ctx_t)));
  ctx->server = server;
  ctx->channel = ch;
  err = uv_thread_create(&ch->accept_thread, bare_bluetooth_android_server__accept_thread, ctx);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_server_unpublish_channel(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&server));
  assert(err == 0);

  uint32_t psm;
  err = js_get_value_uint32(env, argv[1], &psm);
  assert(err == 0);

  for (auto it = server->published_channels.begin(); it != server->published_channels.end(); ++it) {
    auto *ch = *it;
    if (ch->psm == static_cast<uint16_t>(psm)) {
      ch->accepting = false;

      auto jenv = bare_bluetooth_android_jvm().get_env().value();
      auto close = ch->server_socket.get_class().get_method<void()>("close");
      close(ch->server_socket);
      static_cast<JNIEnv *>(jenv)->ExceptionClear();

      uv_thread_join(&ch->accept_thread);

      delete ch;
      server->published_channels.erase(it);
      break;
    }
  }

  return NULL;
}

static js_value_t *
bare_bluetooth_android_server_destroy(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], reinterpret_cast<void **>(&server));
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  for (auto *ch : server->published_channels) {
    ch->accepting = false;
    auto close_ss = ch->server_socket.get_class().get_method<void()>("close");
    close_ss(ch->server_socket);
    static_cast<JNIEnv *>(jenv)->ExceptionClear();
    uv_thread_join(&ch->accept_thread);
    delete ch;
  }
  server->published_channels.clear();

  if (static_cast<jobject>(server->advertise_callback)) {
    auto advertiser = java_object_t<"android/bluetooth/le/BluetoothLeAdvertiser">(jenv, server->advertiser);
    auto stop = advertiser.get_class().get_method<void(java_object_t<"android/bluetooth/le/AdvertiseCallback">)>("stopAdvertising");
    stop(advertiser, java_object_t<"android/bluetooth/le/AdvertiseCallback">(jenv, static_cast<jobject>(server->advertise_callback)));
    server->advertise_callback = {};
  }

  auto gatt_server = java_object_t<"android/bluetooth/BluetoothGattServer">(jenv, server->gatt_server);
  auto close = gatt_server.get_class().get_method<void()>("close");
  close(gatt_server);

  server->characteristics.clear();
  server->connected_devices.clear();
  server->subscriptions.clear();

  err = js_delete_reference(env, server->ctx);
  assert(err == 0);

  js_release_threadsafe_function(server->tsfn_server_connection_state, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_channel_open, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_channel_publish, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_advertise_error, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_unsubscribe, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_subscribe, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_write_request, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_read_request, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_add_service, js_threadsafe_function_abort);
  js_release_threadsafe_function(server->tsfn_state_change, js_threadsafe_function_abort);

  delete server;

  return NULL;
}

static js_value_t *
bare_bluetooth_android_create_mutable_service(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  void *uuid_ptr;
  err = js_get_value_external(env, argv[0], &uuid_ptr);
  assert(err == 0);

  auto *uuid_handle = static_cast<bare_bluetooth_android_uuid_handle_t *>(uuid_ptr);

  bool is_primary;
  err = js_get_value_bool(env, argv[1], &is_primary);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto service_class = java_class_t<"android/bluetooth/BluetoothGattService">(jenv);
  auto service = service_class(java_object_t<"java/util/UUID">(jenv, uuid_handle->handle), is_primary ? 0 : 1);

  auto *service_handle = new bare_bluetooth_android_service_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattService">>(jenv, service)};

  js_value_t *handle;
  err = js_create_external(env, service_handle, bare_bluetooth_android__on_release<bare_bluetooth_android_service_handle_t>, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_bluetooth_android_create_mutable_characteristic(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 4;
  js_value_t *argv[4];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 4);

  void *uuid_ptr;
  err = js_get_value_external(env, argv[0], &uuid_ptr);
  assert(err == 0);

  auto *uuid_handle = static_cast<bare_bluetooth_android_uuid_handle_t *>(uuid_ptr);

  int32_t properties;
  err = js_get_value_int32(env, argv[1], &properties);
  assert(err == 0);

  int32_t js_permissions;
  err = js_get_value_int32(env, argv[2], &js_permissions);
  assert(err == 0);

  int32_t android_permissions = 0;
  if (js_permissions & 0x01) android_permissions |= 0x01;
  if (js_permissions & 0x02) android_permissions |= 0x10;
  if (js_permissions & 0x04) android_permissions |= 0x02;
  if (js_permissions & 0x08) android_permissions |= 0x20;

  auto jenv = bare_bluetooth_android_jvm().get_env().value();

  auto char_class = java_class_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv);
  auto characteristic = char_class(java_object_t<"java/util/UUID">(jenv, uuid_handle->handle), properties, android_permissions);

  if (properties & 0x30) {
    auto uuid_class = java_class_t<"java/util/UUID">(jenv);
    auto from_string = uuid_class.get_static_method<java_object_t<"java/util/UUID">(std::string)>("fromString");
    auto cccd_uuid = from_string(std::string("00002902-0000-1000-8000-00805f9b34fb"));

    auto desc_class = java_class_t<"android/bluetooth/BluetoothGattDescriptor">(jenv);
    auto cccd = desc_class(cccd_uuid, 0x11);

    auto add_descriptor = characteristic.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattDescriptor">)>("addDescriptor");
    add_descriptor(characteristic, cccd);
  }

  js_value_type_t val_type;
  err = js_typeof(env, argv[3], &val_type);
  assert(err == 0);

  if (val_type != js_null && val_type != js_undefined) {
    size_t length;
    js_value_t *arraybuffer;
    size_t offset;
    err = js_get_typedarray_info(env, argv[3], NULL, NULL, &length, &arraybuffer, &offset);
    assert(err == 0);

    void *buf;
    err = js_get_arraybuffer_info(env, arraybuffer, &buf, NULL);
    assert(err == 0);

    auto byte_array = bare_bluetooth_android_make_byte_array(jenv, static_cast<uint8_t *>(buf) + offset, length);
    auto set_value = characteristic.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
    set_value(characteristic, byte_array);
  }

  auto *char_handle = new bare_bluetooth_android_characteristic_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">>(jenv, characteristic)};

  js_value_t *handle;
  err = js_create_external(env, char_handle, bare_bluetooth_android__on_release<bare_bluetooth_android_characteristic_handle_t>, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_bluetooth_android_service_set_characteristics(js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  void *service_ptr;
  err = js_get_value_external(env, argv[0], &service_ptr);
  assert(err == 0);

  auto *service_handle = static_cast<bare_bluetooth_android_service_handle_t *>(service_ptr);

  uint32_t count;
  err = js_get_array_length(env, argv[1], &count);
  assert(err == 0);

  auto jenv = bare_bluetooth_android_jvm().get_env().value();
  auto service = java_object_t<"android/bluetooth/BluetoothGattService">(jenv, service_handle->handle);
  auto add_characteristic = service.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattCharacteristic">)>("addCharacteristic");

  for (uint32_t i = 0; i < count; i++) {
    js_value_t *char_val;
    err = js_get_element(env, argv[1], i, &char_val);
    assert(err == 0);

    void *char_ptr;
    err = js_get_value_external(env, char_val, &char_ptr);
    assert(err == 0);

    auto *char_handle = static_cast<bare_bluetooth_android_characteristic_handle_t *>(char_ptr);
    add_characteristic(service, java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, char_handle->handle));
  }

  return NULL;
}

static void
bare_bluetooth_android_on_server_connection_state_change(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/GattServerCallback"> self, long native_ptr, java_object_t<"android/bluetooth/BluetoothDevice"> device, int status, int new_state) {
  auto *server = reinterpret_cast<bare_bluetooth_android_server_t *>(native_ptr);

  auto device_obj = java_object_t<"android/bluetooth/BluetoothDevice">(env, device);
  auto address = device_obj.get_class().get_method<std::string()>("getAddress")(device_obj);

  auto *event = new bare_bluetooth_android_server_connection_state_t();
  event->address = address;
  event->new_state = new_state;
  event->device = (new_state == 2) ? new bare_bluetooth_android_device_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothDevice">>(env, device)} : nullptr;

  js_call_threadsafe_function(server->tsfn_server_connection_state, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_on_service_added(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/GattServerCallback"> self, long native_ptr, int status, java_object_t<"android/bluetooth/BluetoothGattService"> service) {
  auto *server = reinterpret_cast<bare_bluetooth_android_server_t *>(native_ptr);

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

  js_call_threadsafe_function(server->tsfn_add_service, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_on_read_request(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/GattServerCallback"> self, long native_ptr, java_object_t<"android/bluetooth/BluetoothDevice"> device, int request_id, int offset, java_object_t<"android/bluetooth/BluetoothGattCharacteristic"> characteristic) {
  auto *server = reinterpret_cast<bare_bluetooth_android_server_t *>(native_ptr);

  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);

  auto *event = new bare_bluetooth_android_server_read_request_t();
  event->device = new bare_bluetooth_android_device_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothDevice">>(env, device)};
  event->request_id = request_id;
  event->characteristic_uuid = uuid_str;
  event->offset = offset;

  js_call_threadsafe_function(server->tsfn_read_request, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_on_write_request(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/GattServerCallback"> self, long native_ptr, java_object_t<"android/bluetooth/BluetoothDevice"> device, int request_id, java_object_t<"android/bluetooth/BluetoothGattCharacteristic"> characteristic, bool prepared_write, bool response_needed, int offset, java_array_t<unsigned char> value) {
  auto *server = reinterpret_cast<bare_bluetooth_android_server_t *>(native_ptr);

  auto uuid_str = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);

  auto *event = new bare_bluetooth_android_server_write_request_t();
  event->device = new bare_bluetooth_android_device_handle_t{java_global_ref_t<java_object_t<"android/bluetooth/BluetoothDevice">>(env, device)};
  event->request_id = request_id;
  event->characteristic_uuid = uuid_str;
  event->offset = offset;
  event->response_needed = response_needed;

  if (static_cast<jobject>(value) != nullptr) {
    event->data = bare_bluetooth_android_copy_byte_array(env, value);
  }

  js_call_threadsafe_function(server->tsfn_write_request, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_on_descriptor_write_request(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/GattServerCallback"> self, long native_ptr, java_object_t<"android/bluetooth/BluetoothDevice"> device, int request_id, java_object_t<"android/bluetooth/BluetoothGattDescriptor"> descriptor, bool prepared_write, bool response_needed, int offset, java_array_t<unsigned char> value) {
  auto *server = reinterpret_cast<bare_bluetooth_android_server_t *>(native_ptr);

  auto desc_uuid = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattDescriptor">(env, descriptor);

  if (response_needed) {
    auto gatt_server = java_object_t<"android/bluetooth/BluetoothGattServer">(env, server->gatt_server);
    gatt_server.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothDevice">, int, int, int, java_object_t<"java/lang/Object">)>("sendResponse")(
      gatt_server, java_object_t<"android/bluetooth/BluetoothDevice">(env, device), static_cast<int>(request_id), 0, static_cast<int>(offset), java_object_t<"java/lang/Object">(env, value)
    );
  }

  bool is_cccd = desc_uuid == "00002902-0000-1000-8000-00805f9b34fb";
  if (!is_cccd) return;

  auto desc_obj = java_object_t<"android/bluetooth/BluetoothGattDescriptor">(env, descriptor);
  auto characteristic = desc_obj.get_class().get_method<java_object_t<"android/bluetooth/BluetoothGattCharacteristic">()>("getCharacteristic")(desc_obj);
  auto char_uuid = bare_bluetooth_android_get_uuid_string<"android/bluetooth/BluetoothGattCharacteristic">(env, characteristic);

  auto device_obj = java_object_t<"android/bluetooth/BluetoothDevice">(env, device);
  auto device_address = device_obj.get_class().get_method<std::string()>("getAddress")(device_obj);

  bool subscribing = false;
  if (static_cast<jobject>(value) != nullptr) {
    auto arr = java_array_t<unsigned char>(env, value);
    if (arr.size() >= 1) {
      subscribing = (arr[static_cast<size_t>(0)] != 0);
    }
  }

  if (subscribing) {
    server->subscriptions[char_uuid].insert(device_address);

    auto *event = new bare_bluetooth_android_server_subscribe_t();
    event->device_address = device_address;
    event->characteristic_uuid = char_uuid;
    js_call_threadsafe_function(server->tsfn_subscribe, event, js_threadsafe_function_nonblocking);
  } else {
    server->subscriptions[char_uuid].erase(device_address);

    auto *event = new bare_bluetooth_android_server_unsubscribe_t();
    event->device_address = device_address;
    event->characteristic_uuid = char_uuid;
    js_call_threadsafe_function(server->tsfn_unsubscribe, event, js_threadsafe_function_nonblocking);
  }
}

static void
bare_bluetooth_android_on_advertise_success(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/AdvertiseCallback"> self, long native_ptr, java_object_t<"android/bluetooth/le/AdvertiseSettings"> settings) {
  (void) env;
  (void) native_ptr;
  (void) settings;
}

static void
bare_bluetooth_android_on_advertise_failure(java_env_t env, java_object_t<"to/holepunch/bare/bluetooth/AdvertiseCallback"> self, long native_ptr, int error_code) {
  auto *server = reinterpret_cast<bare_bluetooth_android_server_t *>(native_ptr);

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

  js_call_threadsafe_function(server->tsfn_advertise_error, event, js_threadsafe_function_nonblocking);
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
      java_native_method_t<bare_bluetooth_android_on_descriptor_write_request>("nativeOnDescriptorWriteRequest")
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
}

static js_value_t *
bare_bluetooth_android_exports(js_env_t *env, js_value_t *exports) {
  int err;

  bare_bluetooth_android_register_natives();

#define V(name, fn) \
  { \
    js_value_t *val; \
    err = js_create_function(env, name, -1, fn, nullptr, &val); \
    assert(err == 0); \
    err = js_set_named_property(env, exports, name, val); \
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
#undef V

  return exports;
}

BARE_MODULE(bare_bluetooth_android, bare_bluetooth_android_exports)
