#include <assert.h>
#include <bare.h>
#include <jni.h>
#include <jnitl.h>
#include <js.h>
#include <utf.h>

#include <stdlib.h>
#include <uv.h>
#include <android/native_activity.h>

#include <dlfcn.h>
#include <unordered_map>
#include <unordered_set>

extern ANativeActivity *bare_native_activity;

static JavaVM *bare_bluetooth_android_jvm = NULL;

struct bare_bluetooth_android_channel_t {
  js_env_t *env;
  js_ref_t *ctx;

  js_threadsafe_function_t *tsfn_data;
  js_threadsafe_function_t *tsfn_drain;
  js_threadsafe_function_t *tsfn_end;
  js_threadsafe_function_t *tsfn_error;
  js_threadsafe_function_t *tsfn_close;
  js_threadsafe_function_t *tsfn_open;

  jobject socket;
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
  char *message;
} bare_bluetooth_android_channel_error_t;

static void
bare_bluetooth_android__release_global_ref (js_env_t *env, void *data, void *hint) {
  java_env_t jenv(bare_native_activity->env);
  ((JNIEnv *) jenv)->DeleteGlobalRef((jobject) data);
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
};

typedef struct {
  int32_t state;
} bare_bluetooth_android_central_state_change_t;

typedef struct {
  char *address;
  char *name;
  int32_t rssi;
  jobject device;
} bare_bluetooth_android_central_discover_t;

typedef struct {
  char *address;
  jobject gatt;
} bare_bluetooth_android_central_connect_t;

typedef struct {
  char *address;
  char *error;
} bare_bluetooth_android_central_disconnect_t;

typedef struct {
  char *address;
  char *error;
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

  jobject gatt;
  jobject device;

  std::vector<jobject> services;
  std::vector<std::vector<jobject>> service_characteristics;

  uv_thread_t l2cap_thread;
  std::atomic<bool> l2cap_connecting;
};

typedef struct {
  uint32_t count;
  char *error;
} bare_bluetooth_android_peripheral_services_discover_t;

typedef struct {
  jobject service;
  uint32_t count;
  char *error;
} bare_bluetooth_android_peripheral_characteristics_discover_t;

typedef struct {
  jobject characteristic;
  char *uuid;
  void *data;
  size_t data_len;
  char *error;
} bare_bluetooth_android_peripheral_read_t;

typedef struct {
  jobject characteristic;
  char *uuid;
  char *error;
} bare_bluetooth_android_peripheral_write_t;

typedef struct {
  jobject characteristic;
  char *uuid;
  void *data;
  size_t data_len;
  char *error;
} bare_bluetooth_android_peripheral_notify_t;

typedef struct {
  jobject characteristic;
  char *uuid;
  bool is_notifying;
  char *error;
} bare_bluetooth_android_peripheral_notify_state_t;

typedef struct {
  jobject channel;
  char *error;
  uint32_t psm;
} bare_bluetooth_android_peripheral_channel_open_t;

typedef struct {
  int32_t mtu;
  char *error;
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

  jobject gatt_server;
  jobject advertiser;
  jobject advertise_callback;

  struct published_channel_t {
    jobject server_socket;
    uv_thread_t accept_thread;
    std::atomic<bool> accepting;
    uint16_t psm;
  };

  std::vector<published_channel_t *> published_channels;

  std::unordered_map<std::string, std::unordered_set<std::string>> subscriptions;
  std::unordered_map<std::string, jobject> connected_devices;
  std::unordered_map<std::string, jobject> characteristics;
};

typedef struct {
  int32_t state;
} bare_bluetooth_android_server_state_change_t;

typedef struct {
  char *uuid;
  char *error;
} bare_bluetooth_android_server_add_service_t;

typedef struct {
  jobject device;
  int32_t request_id;
  char *characteristic_uuid;
  int32_t offset;
} bare_bluetooth_android_server_read_request_t;

typedef struct {
  jobject device;
  int32_t request_id;
  char *characteristic_uuid;
  int32_t offset;
  void *data;
  size_t data_len;
  bool response_needed;
} bare_bluetooth_android_server_write_request_t;

typedef struct {
  char *device_address;
  char *characteristic_uuid;
} bare_bluetooth_android_server_subscribe_t;

typedef struct {
  char *device_address;
  char *characteristic_uuid;
} bare_bluetooth_android_server_unsubscribe_t;

typedef struct {
  int32_t error_code;
  char *error;
} bare_bluetooth_android_server_advertise_error_t;

typedef struct {
  uint16_t psm;
  char *error;
} bare_bluetooth_android_server_channel_publish_t;

typedef struct {
  jobject channel;
  char *error;
  uint16_t psm;
} bare_bluetooth_android_server_channel_open_t;

typedef struct {
  char *address;
  jobject device;
  int32_t new_state;
} bare_bluetooth_android_server_connection_state_t;

static void
bare_bluetooth_android_channel__on_data (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_channel_data_t *) data;
  auto *channel = (bare_bluetooth_android_channel_t *) context;

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

  free(event);

  js_call_function(env, receiver, function, 1, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_channel__on_drain (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *channel = (bare_bluetooth_android_channel_t *) context;

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
bare_bluetooth_android_channel__on_end (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *channel = (bare_bluetooth_android_channel_t *) context;

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
bare_bluetooth_android_channel__on_error (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_channel_error_t *) data;
  auto *channel = (bare_bluetooth_android_channel_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, channel->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[1];

  err = js_create_string_utf8(env, (const utf8_t *) event->message, -1, &argv[0]);
  assert(err == 0);
  free(event->message);
  free(event);

  js_call_function(env, receiver, function, 1, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_channel__on_close (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *channel = (bare_bluetooth_android_channel_t *) context;

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
bare_bluetooth_android_channel__on_open (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *channel = (bare_bluetooth_android_channel_t *) context;

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
bare_bluetooth_android_channel__reader_thread (void *arg) {
  auto *channel = (bare_bluetooth_android_channel_t *) arg;

  JNIEnv *jni_env;
  bare_bluetooth_android_jvm->AttachCurrentThread(&jni_env, NULL);

  jclass socket_class = jni_env->GetObjectClass(channel->socket);
  jmethodID get_input = jni_env->GetMethodID(socket_class, "getInputStream", "()Ljava/io/InputStream;");
  jobject input_stream = jni_env->CallObjectMethod(channel->socket, get_input);

  if (jni_env->ExceptionCheck()) {
    jni_env->ExceptionClear();
    auto *event = (bare_bluetooth_android_channel_error_t *) malloc(sizeof(bare_bluetooth_android_channel_error_t));
    event->message = strdup("Failed to get InputStream");
    js_call_threadsafe_function(channel->tsfn_error, event, js_threadsafe_function_nonblocking);
    js_call_threadsafe_function(channel->tsfn_close, NULL, js_threadsafe_function_nonblocking);
    bare_bluetooth_android_jvm->DetachCurrentThread();
    return;
  }

  js_call_threadsafe_function(channel->tsfn_open, NULL, js_threadsafe_function_nonblocking);

  jclass is_class = jni_env->GetObjectClass(input_stream);
  jmethodID read_method = jni_env->GetMethodID(is_class, "read", "([B)I");

  jbyteArray read_buf = jni_env->NewByteArray(4096);

  while (!channel->destroyed) {
    jint bytes_read = jni_env->CallIntMethod(input_stream, read_method, read_buf);

    if (jni_env->ExceptionCheck()) {
      jni_env->ExceptionClear();

      if (!channel->destroyed) {
        auto *event = (bare_bluetooth_android_channel_error_t *) malloc(sizeof(bare_bluetooth_android_channel_error_t));
        event->message = strdup("Read error");
        js_call_threadsafe_function(channel->tsfn_error, event, js_threadsafe_function_nonblocking);
      }
      break;
    }

    if (bytes_read == -1) {
      js_call_threadsafe_function(channel->tsfn_end, NULL, js_threadsafe_function_nonblocking);
      break;
    }

    if (bytes_read > 0) {
      auto *event = (bare_bluetooth_android_channel_data_t *) malloc(sizeof(bare_bluetooth_android_channel_data_t));
      event->len = (size_t) bytes_read;
      event->bytes = malloc(bytes_read);
      jni_env->GetByteArrayRegion(read_buf, 0, bytes_read, (jbyte *) event->bytes);

      js_call_threadsafe_function(channel->tsfn_data, event, js_threadsafe_function_nonblocking);
    }
  }

  jni_env->DeleteLocalRef(read_buf);
  jni_env->DeleteLocalRef(input_stream);

  jmethodID close_method = jni_env->GetMethodID(socket_class, "close", "()V");
  jni_env->CallVoidMethod(channel->socket, close_method);
  jni_env->ExceptionClear();

  js_call_threadsafe_function(channel->tsfn_close, NULL, js_threadsafe_function_nonblocking);

  bare_bluetooth_android_jvm->DetachCurrentThread();
}

static js_value_t *
bare_bluetooth_android_l2cap_init (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 8;
  js_value_t *argv[8];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 8);

  void *socket_ptr;
  err = js_get_value_external(env, argv[0], &socket_ptr);
  assert(err == 0);

  auto *channel = new bare_bluetooth_android_channel_t();
  channel->env = env;
  channel->socket = (jobject) socket_ptr;
  channel->opened = false;
  channel->destroyed = false;
  channel->finalized = false;

  if (bare_bluetooth_android_jvm == NULL) {
    java_env_t jenv(bare_native_activity->env);
    ((JNIEnv *) jenv)->GetJavaVM(&bare_bluetooth_android_jvm);
  }

  java_env_t jenv(bare_native_activity->env);
  auto socket = java_object_t<"android/bluetooth/BluetoothSocket">(jenv, channel->socket);
  auto get_device = socket.get_class().get_method<java_object_t<"android/bluetooth/BluetoothDevice">()>("getRemoteDevice");
  auto device = get_device(socket);
  auto get_address = device.get_class().get_method<std::string()>("getAddress");
  channel->peer_address = get_address(device);
  channel->psm = 0;

  err = js_create_reference(env, argv[1], 1, &channel->ctx);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[2], 0, 1, NULL, NULL, (void *) channel, bare_bluetooth_android_channel__on_data, &channel->tsfn_data);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[3], 0, 1, NULL, NULL, (void *) channel, bare_bluetooth_android_channel__on_drain, &channel->tsfn_drain);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[4], 0, 1, NULL, NULL, (void *) channel, bare_bluetooth_android_channel__on_end, &channel->tsfn_end);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[5], 0, 1, NULL, NULL, (void *) channel, bare_bluetooth_android_channel__on_error, &channel->tsfn_error);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[6], 0, 1, NULL, NULL, (void *) channel, bare_bluetooth_android_channel__on_close, &channel->tsfn_close);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[7], 0, 1, NULL, NULL, (void *) channel, bare_bluetooth_android_channel__on_open, &channel->tsfn_open);
  assert(err == 0);

  js_value_t *handle;
  err = js_create_external(env, (void *) channel, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_bluetooth_android_l2cap_open (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_channel_t *channel;
  err = js_get_value_external(env, argv[0], (void **) &channel);
  assert(err == 0);

  bool expected = false;
  if (!channel->opened.compare_exchange_strong(expected, true)) return NULL;

  err = uv_thread_create(&channel->reader_thread, bare_bluetooth_android_channel__reader_thread, (void *) channel);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_l2cap_write (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_channel_t *channel;
  err = js_get_value_external(env, argv[0], (void **) &channel);
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

  uint8_t *data = (uint8_t *) buf + offset;

  java_env_t jenv(bare_native_activity->env);
  auto socket = java_object_t<"android/bluetooth/BluetoothSocket">(jenv, channel->socket);
  auto get_output = socket.get_class().get_method<java_object_t<"java/io/OutputStream">()>("getOutputStream");
  auto output = get_output(socket);

  jbyteArray byte_array = ((JNIEnv *) jenv)->NewByteArray((jsize) length);
  ((JNIEnv *) jenv)->SetByteArrayRegion(byte_array, 0, (jsize) length, (const jbyte *) data);

  auto write_method = output.get_class().get_method<void(java_array_t<unsigned char>)>("write");

  write_method(output, java_array_t<unsigned char>(jenv, byte_array));

  bool write_ok = true;
  if (((JNIEnv *) jenv)->ExceptionCheck()) {
    ((JNIEnv *) jenv)->ExceptionClear();
    write_ok = false;
  }

  ((JNIEnv *) jenv)->DeleteLocalRef(byte_array);

  if (write_ok) {
    js_call_threadsafe_function(channel->tsfn_drain, NULL, js_threadsafe_function_nonblocking);
  } else {
    auto *event = (bare_bluetooth_android_channel_error_t *) malloc(sizeof(bare_bluetooth_android_channel_error_t));
    event->message = strdup("Write error");
    js_call_threadsafe_function(channel->tsfn_error, event, js_threadsafe_function_nonblocking);
  }

  js_value_t *result;
  err = js_create_int32(env, write_ok ? (int32_t) length : 0, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_l2cap_end (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_channel_t *channel;
  err = js_get_value_external(env, argv[0], (void **) &channel);
  assert(err == 0);

  bool expected = false;
  if (!channel->destroyed.compare_exchange_strong(expected, true)) return NULL;

  if (!channel->opened) {
    js_call_threadsafe_function(channel->tsfn_close, NULL, js_threadsafe_function_nonblocking);
    return NULL;
  }

  java_env_t jenv(bare_native_activity->env);
  auto socket = java_object_t<"android/bluetooth/BluetoothSocket">(jenv, channel->socket);
  auto close = socket.get_class().get_method<void()>("close");
  close(socket);
  ((JNIEnv *) jenv)->ExceptionClear();

  return NULL;
}

static js_value_t *
bare_bluetooth_android_l2cap_psm (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_channel_t *channel;
  err = js_get_value_external(env, argv[0], (void **) &channel);
  assert(err == 0);

  js_value_t *result;
  err = js_create_uint32(env, (uint32_t) channel->psm, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_l2cap_peer (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_channel_t *channel;
  err = js_get_value_external(env, argv[0], (void **) &channel);
  assert(err == 0);

  if (channel->peer_address.empty()) {
    js_value_t *result;
    err = js_get_null(env, &result);
    assert(err == 0);
    return result;
  }

  js_value_t *result;
  err = js_create_string_utf8(env, (const utf8_t *) channel->peer_address.c_str(), channel->peer_address.size(), &result);
  assert(err == 0);

  return result;
}

static void
bare_bluetooth_android_central__on_state_change (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_central_state_change_t *) data;
  auto *central = (bare_bluetooth_android_central_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[1];
  err = js_create_int32(env, event->state, &argv[0]);
  assert(err == 0);

  free(event);

  js_call_function(env, receiver, function, 1, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_discover (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_central_discover_t *) data;
  auto *central = (bare_bluetooth_android_central_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[4];

  err = js_create_external(env, (void *) event->device, bare_bluetooth_android__release_global_ref, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, (const utf8_t *) event->address, -1, &argv[1]);
  assert(err == 0);

  if (event->name) {
    err = js_create_string_utf8(env, (const utf8_t *) event->name, -1, &argv[2]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[2]);
    assert(err == 0);
  }

  err = js_create_int32(env, event->rssi, &argv[3]);
  assert(err == 0);

  free(event->address);
  if (event->name) free(event->name);
  free(event);

  js_call_function(env, receiver, function, 4, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_connect (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_central_connect_t *) data;
  auto *central = (bare_bluetooth_android_central_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_external(env, (void *) event->gatt, bare_bluetooth_android__release_global_ref, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, (const utf8_t *) event->address, -1, &argv[1]);
  assert(err == 0);

  free(event->address);
  free(event);

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_disconnect (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_central_disconnect_t *) data;
  auto *central = (bare_bluetooth_android_central_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_string_utf8(env, (const utf8_t *) event->address, -1, &argv[0]);
  assert(err == 0);

  free(event->address);

  if (event->error) {
    err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[1]);
    assert(err == 0);
    free(event->error);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  free(event);

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_connect_fail (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_central_connect_fail_t *) data;
  auto *central = (bare_bluetooth_android_central_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_string_utf8(env, (const utf8_t *) event->address, -1, &argv[0]);
  assert(err == 0);

  free(event->address);

  err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[1]);
  assert(err == 0);

  free(event->error);
  free(event);

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_central__on_scan_fail (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_central_scan_fail_t *) data;
  auto *central = (bare_bluetooth_android_central_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, central->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[1];
  err = js_create_int32(env, event->error_code, &argv[0]);
  assert(err == 0);

  free(event);

  js_call_function(env, receiver, function, 1, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static js_value_t *
bare_bluetooth_android_central_init (js_env_t *env, js_callback_info_t *info) {
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

  err = js_create_threadsafe_function(env, argv[1], 0, 1, NULL, NULL, (void *) central, bare_bluetooth_android_central__on_state_change, &central->tsfn_state_change);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[2], 0, 1, NULL, NULL, (void *) central, bare_bluetooth_android_central__on_discover, &central->tsfn_discover);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[3], 0, 1, NULL, NULL, (void *) central, bare_bluetooth_android_central__on_connect, &central->tsfn_connect);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[4], 0, 1, NULL, NULL, (void *) central, bare_bluetooth_android_central__on_disconnect, &central->tsfn_disconnect);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[5], 0, 1, NULL, NULL, (void *) central, bare_bluetooth_android_central__on_connect_fail, &central->tsfn_connect_fail);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[6], 0, 1, NULL, NULL, (void *) central, bare_bluetooth_android_central__on_scan_fail, &central->tsfn_scan_fail);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);

  auto activity = java_object_t<"android/content/Context">(jenv, bare_native_activity->clazz);
  auto get_system_service = activity.get_class().get_method<java_object_t<"java/lang/Object">(std::string)>("getSystemService");
  auto manager_obj = get_system_service(activity, std::string("bluetooth"));

  auto bt_manager = java_object_t<"android/bluetooth/BluetoothManager">(jenv, (jobject) manager_obj);
  auto get_adapter = bt_manager.get_class().get_method<java_object_t<"android/bluetooth/BluetoothAdapter">()>("getAdapter");
  auto adapter_local = get_adapter(bt_manager);

  central->adapter = java_global_ref_t<java_object_t<"android/bluetooth/BluetoothAdapter">>(jenv, (jobject) adapter_local);

  auto get_scanner = central->adapter.get_class().get_method<java_object_t<"android/bluetooth/le/BluetoothLeScanner">()>("getBluetoothLeScanner");
  auto scanner_local = get_scanner(central->adapter);

  central->scanner = java_global_ref_t<java_object_t<"android/bluetooth/le/BluetoothLeScanner">>(jenv, (jobject) scanner_local);

  auto get_state = central->adapter.get_class().get_method<int()>("getState");
  int android_state = get_state(central->adapter);

  auto *state_event = (bare_bluetooth_android_central_state_change_t *) malloc(sizeof(bare_bluetooth_android_central_state_change_t));
  state_event->state = android_state;
  js_call_threadsafe_function(central->tsfn_state_change, state_event, js_threadsafe_function_nonblocking);

  js_value_t *handle;
  err = js_create_external(env, (void *) central, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_bluetooth_android_central_start_scan (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1 || argc == 2);

  bare_bluetooth_android_central_t *central;
  err = js_get_value_external(env, argv[0], (void **) &central);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);

  jobject filter_list = nullptr;

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

        auto builder = filter_builder_class();
        auto set_service_uuid = builder.get_class().get_method<java_object_t<"android/bluetooth/le/ScanFilter$Builder">(java_object_t<"android/os/ParcelUuid">)>("setServiceUuid");
        auto parcel_uuid = parcel_uuid_class(java_object_t<"java/util/UUID">(jenv, (jobject) uuid_ptr));
        set_service_uuid(builder, parcel_uuid);

        auto build_filter = builder.get_class().get_method<java_object_t<"android/bluetooth/le/ScanFilter">()>("build");
        auto filter = build_filter(builder);
        list_add(list, java_object_t<"java/lang/Object">(jenv, (jobject) filter));
      }

      filter_list = (jobject) list;
    }
  }

  auto callback_class = java_class_t<"to/holepunch/bare/bluetooth/ScanCallback">(jenv);
  auto callback_local = callback_class((long) central);

  central->scan_callback = java_global_ref_t<java_object_t<"to/holepunch/bare/bluetooth/ScanCallback">>(jenv, (jobject) callback_local);

  auto settings_builder_class = java_class_t<"android/bluetooth/le/ScanSettings$Builder">(jenv);
  auto settings_builder = settings_builder_class();

  auto set_scan_mode = settings_builder.get_class().get_method<java_object_t<"android/bluetooth/le/ScanSettings$Builder">(int)>("setScanMode");
  set_scan_mode(settings_builder, 2);

  auto build_settings = settings_builder.get_class().get_method<java_object_t<"android/bluetooth/le/ScanSettings">()>("build");
  auto settings = build_settings(settings_builder);

  auto start_scan = central->scanner.get_class().get_method<void(java_object_t<"java/util/List">, java_object_t<"android/bluetooth/le/ScanSettings">, java_object_t<"android/bluetooth/le/ScanCallback">)>("startScan");
  start_scan(central->scanner, java_object_t<"java/util/List">(jenv, filter_list), java_object_t<"android/bluetooth/le/ScanSettings">(jenv, (jobject) settings), java_object_t<"android/bluetooth/le/ScanCallback">(jenv, (jobject) central->scan_callback));

  return NULL;
}

static js_value_t *
bare_bluetooth_android_central_stop_scan (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_central_t *central;
  err = js_get_value_external(env, argv[0], (void **) &central);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);

  auto stop_scan = central->scanner.get_class().get_method<void(java_object_t<"android/bluetooth/le/ScanCallback">)>("stopScan");
  stop_scan(central->scanner, java_object_t<"android/bluetooth/le/ScanCallback">(jenv, (jobject) central->scan_callback));

  return NULL;
}

static js_value_t *
bare_bluetooth_android_central_connect (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_central_t *central;
  err = js_get_value_external(env, argv[0], (void **) &central);
  assert(err == 0);

  void *device_ptr;
  err = js_get_value_external(env, argv[1], &device_ptr);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);

  auto device = java_object_t<"android/bluetooth/BluetoothDevice">(jenv, (jobject) device_ptr);

  auto gatt_callback_class = java_class_t<"to/holepunch/bare/bluetooth/GattCallback">(jenv);
  auto gatt_callback = gatt_callback_class((long) central);

  auto context = java_object_t<"android/content/Context">(jenv, bare_native_activity->clazz);

  auto connect_gatt = device.get_class().get_method<java_object_t<"android/bluetooth/BluetoothGatt">(java_object_t<"android/content/Context">, bool, java_object_t<"android/bluetooth/BluetoothGattCallback">, int)>("connectGatt");
  connect_gatt(device, context, false, java_object_t<"android/bluetooth/BluetoothGattCallback">(jenv, (jobject) gatt_callback), 2);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_central_disconnect (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_central_t *central;
  err = js_get_value_external(env, argv[0], (void **) &central);
  assert(err == 0);

  void *gatt_ptr;
  err = js_get_value_external(env, argv[1], &gatt_ptr);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);

  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, (jobject) gatt_ptr);

  auto disconnect = gatt.get_class().get_method<void()>("disconnect");
  disconnect(gatt);

  auto close = gatt.get_class().get_method<void()>("close");
  close(gatt);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_central_destroy (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_central_t *central;
  err = js_get_value_external(env, argv[0], (void **) &central);
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
bare_bluetooth_android_create_uuid (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  size_t len;
  err = js_get_value_string_utf8(env, argv[0], NULL, 0, &len);
  assert(err == 0);

  char *str = (char *) malloc(len + 1);
  err = js_get_value_string_utf8(env, argv[0], (utf8_t *) str, len + 1, NULL);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);

  auto uuid_class = java_class_t<"java/util/UUID">(jenv);
  auto from_string = uuid_class.get_static_method<java_object_t<"java/util/UUID">(std::string)>("fromString");
  auto uuid_local = from_string(std::string(str));

  free(str);

  jobject uuid_global = ((JNIEnv *) jenv)->NewGlobalRef((jobject) uuid_local);

  js_value_t *handle;
  err = js_create_external(env, (void *) uuid_global, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_ScanCallback_nativeOnScanResult (JNIEnv *jni_env, jclass, jlong native_ptr, jint callback_type, jobject result) {
  auto *central = (bare_bluetooth_android_central_t *) native_ptr;

  jclass scan_result_class = jni_env->GetObjectClass(result);
  jmethodID get_device = jni_env->GetMethodID(scan_result_class, "getDevice", "()Landroid/bluetooth/BluetoothDevice;");
  jobject device = jni_env->CallObjectMethod(result, get_device);

  jclass device_class = jni_env->GetObjectClass(device);
  jmethodID get_address = jni_env->GetMethodID(device_class, "getAddress", "()Ljava/lang/String;");
  jstring address_jstr = (jstring) jni_env->CallObjectMethod(device, get_address);
  const char *address_chars = jni_env->GetStringUTFChars(address_jstr, NULL);

  jmethodID get_name = jni_env->GetMethodID(device_class, "getName", "()Ljava/lang/String;");
  jstring name_jstr = (jstring) jni_env->CallObjectMethod(device, get_name);

  jmethodID get_rssi = jni_env->GetMethodID(scan_result_class, "getRssi", "()I");
  jint rssi = jni_env->CallIntMethod(result, get_rssi);

  auto *event = (bare_bluetooth_android_central_discover_t *) malloc(sizeof(bare_bluetooth_android_central_discover_t));
  event->address = strdup(address_chars);
  event->rssi = rssi;
  event->device = jni_env->NewGlobalRef(device);

  jni_env->ReleaseStringUTFChars(address_jstr, address_chars);

  if (name_jstr) {
    const char *name_chars = jni_env->GetStringUTFChars(name_jstr, NULL);
    event->name = strdup(name_chars);
    jni_env->ReleaseStringUTFChars(name_jstr, name_chars);
  } else {
    event->name = NULL;
  }

  js_call_threadsafe_function(central->tsfn_discover, event, js_threadsafe_function_nonblocking);
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_ScanCallback_nativeOnScanFailed (JNIEnv *jni_env, jclass, jlong native_ptr, jint error_code) {
  auto *central = (bare_bluetooth_android_central_t *) native_ptr;

  auto *event = (bare_bluetooth_android_central_scan_fail_t *) malloc(sizeof(bare_bluetooth_android_central_scan_fail_t));
  event->error_code = error_code;

  js_call_threadsafe_function(central->tsfn_scan_fail, event, js_threadsafe_function_nonblocking);
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnConnectionStateChange (JNIEnv *jni_env, jclass, jlong native_ptr, jobject gatt, jint status, jint new_state) {
  auto *central = (bare_bluetooth_android_central_t *) native_ptr;

  jclass gatt_class = jni_env->GetObjectClass(gatt);
  jmethodID get_device = jni_env->GetMethodID(gatt_class, "getDevice", "()Landroid/bluetooth/BluetoothDevice;");
  jobject device = jni_env->CallObjectMethod(gatt, get_device);

  jclass device_class = jni_env->GetObjectClass(device);
  jmethodID get_address = jni_env->GetMethodID(device_class, "getAddress", "()Ljava/lang/String;");
  jstring address_jstr = (jstring) jni_env->CallObjectMethod(device, get_address);
  const char *address_chars = jni_env->GetStringUTFChars(address_jstr, NULL);

  std::string address(address_chars);

  if (new_state == 2 && status == 0) {
    central->connected_addresses.insert(address);

    auto *event = (bare_bluetooth_android_central_connect_t *) malloc(sizeof(bare_bluetooth_android_central_connect_t));
    event->address = strdup(address_chars);
    event->gatt = jni_env->NewGlobalRef(gatt);

    js_call_threadsafe_function(central->tsfn_connect, event, js_threadsafe_function_nonblocking);
  } else if (new_state == 0) {
    bool was_connected = central->connected_addresses.erase(address) > 0;

    if (was_connected) {
      auto *event = (bare_bluetooth_android_central_disconnect_t *) malloc(sizeof(bare_bluetooth_android_central_disconnect_t));
      event->address = strdup(address_chars);

      if (status != 0) {
        char error_buf[64];
        snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
        event->error = strdup(error_buf);
      } else {
        event->error = NULL;
      }

      js_call_threadsafe_function(central->tsfn_disconnect, event, js_threadsafe_function_nonblocking);
    } else {
      auto *event = (bare_bluetooth_android_central_connect_fail_t *) malloc(sizeof(bare_bluetooth_android_central_connect_fail_t));
      event->address = strdup(address_chars);

      char error_buf[64];
      snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
      event->error = strdup(error_buf);

      js_call_threadsafe_function(central->tsfn_connect_fail, event, js_threadsafe_function_nonblocking);
    }
  }

  jni_env->ReleaseStringUTFChars(address_jstr, address_chars);
}

static void
bare_bluetooth_android_peripheral__on_services_discover (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_peripheral_services_discover_t *) data;
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_uint32(env, event->count, &argv[0]);
  assert(err == 0);

  if (event->error) {
    err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[1]);
    assert(err == 0);
    free(event->error);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  free(event);

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_characteristics_discover (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_peripheral_characteristics_discover_t *) data;
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[3];

  err = js_create_external(env, (void *) event->service, NULL, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_uint32(env, event->count, &argv[1]);
  assert(err == 0);

  if (event->error) {
    err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[2]);
    assert(err == 0);
    free(event->error);
  } else {
    err = js_get_null(env, &argv[2]);
    assert(err == 0);
  }

  free(event);

  js_call_function(env, receiver, function, 3, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_read (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_peripheral_read_t *) data;
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[4];

  err = js_create_external(env, (void *) event->characteristic, NULL, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, (const utf8_t *) event->uuid, -1, &argv[1]);
  assert(err == 0);
  free(event->uuid);

  if (event->data) {
    js_value_t *arraybuffer;
    void *buf;
    err = js_create_arraybuffer(env, event->data_len, &buf, &arraybuffer);
    assert(err == 0);
    memcpy(buf, event->data, event->data_len);
    free(event->data);

    err = js_create_typedarray(env, js_uint8array, event->data_len, arraybuffer, 0, &argv[2]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[2]);
    assert(err == 0);
  }

  if (event->error) {
    err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[3]);
    assert(err == 0);
    free(event->error);
  } else {
    err = js_get_null(env, &argv[3]);
    assert(err == 0);
  }

  free(event);

  js_call_function(env, receiver, function, 4, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_write (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_peripheral_write_t *) data;
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[3];

  err = js_create_external(env, (void *) event->characteristic, NULL, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, (const utf8_t *) event->uuid, -1, &argv[1]);
  assert(err == 0);
  free(event->uuid);

  if (event->error) {
    err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[2]);
    assert(err == 0);
    free(event->error);
  } else {
    err = js_get_null(env, &argv[2]);
    assert(err == 0);
  }

  free(event);

  js_call_function(env, receiver, function, 3, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_notify (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_peripheral_notify_t *) data;
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[4];

  err = js_create_external(env, (void *) event->characteristic, NULL, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, (const utf8_t *) event->uuid, -1, &argv[1]);
  assert(err == 0);
  free(event->uuid);

  if (event->data) {
    js_value_t *arraybuffer;
    void *buf;
    err = js_create_arraybuffer(env, event->data_len, &buf, &arraybuffer);
    assert(err == 0);
    memcpy(buf, event->data, event->data_len);
    free(event->data);

    err = js_create_typedarray(env, js_uint8array, event->data_len, arraybuffer, 0, &argv[2]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[2]);
    assert(err == 0);
  }

  if (event->error) {
    err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[3]);
    assert(err == 0);
    free(event->error);
  } else {
    err = js_get_null(env, &argv[3]);
    assert(err == 0);
  }

  free(event);

  js_call_function(env, receiver, function, 4, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_notify_state (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_peripheral_notify_state_t *) data;
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[4];

  err = js_create_external(env, (void *) event->characteristic, NULL, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, (const utf8_t *) event->uuid, -1, &argv[1]);
  assert(err == 0);
  free(event->uuid);

  err = js_get_boolean(env, event->is_notifying, &argv[2]);
  assert(err == 0);

  if (event->error) {
    err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[3]);
    assert(err == 0);
    free(event->error);
  } else {
    err = js_get_null(env, &argv[3]);
    assert(err == 0);
  }

  free(event);

  js_call_function(env, receiver, function, 4, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_channel_open (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_peripheral_channel_open_t *) data;
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[3];

  if (event->channel) {
    err = js_create_external(env, (void *) event->channel, NULL, NULL, &argv[0]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[0]);
    assert(err == 0);
  }

  if (event->error) {
    err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[1]);
    assert(err == 0);
    free(event->error);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  err = js_create_uint32(env, event->psm, &argv[2]);
  assert(err == 0);

  free(event);

  js_call_function(env, receiver, function, 3, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_peripheral__on_mtu_changed (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_peripheral_mtu_changed_t *) data;
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, peripheral->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_int32(env, event->mtu, &argv[0]);
  assert(err == 0);

  if (event->error) {
    err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[1]);
    assert(err == 0);
    free(event->error);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  free(event);

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static js_value_t *
bare_bluetooth_android_peripheral_init (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 10;
  js_value_t *argv[10];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 10);

  void *gatt_ptr;
  err = js_get_value_external(env, argv[0], &gatt_ptr);
  assert(err == 0);

  auto *peripheral = new bare_bluetooth_android_peripheral_t();
  peripheral->env = env;
  peripheral->destroyed = false;
  peripheral->l2cap_connecting = false;

  java_env_t jenv(bare_native_activity->env);
  peripheral->gatt = ((JNIEnv *) jenv)->NewGlobalRef((jobject) gatt_ptr);

  auto gatt_obj = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto get_device = gatt_obj.get_class().get_method<java_object_t<"android/bluetooth/BluetoothDevice">()>("getDevice");
  auto device_local = get_device(gatt_obj);
  peripheral->device = ((JNIEnv *) jenv)->NewGlobalRef((jobject) device_local);

  err = js_create_reference(env, argv[1], 1, &peripheral->ctx);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[2], 0, 1, NULL, NULL, (void *) peripheral, bare_bluetooth_android_peripheral__on_services_discover, &peripheral->tsfn_services_discover);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[3], 0, 1, NULL, NULL, (void *) peripheral, bare_bluetooth_android_peripheral__on_characteristics_discover, &peripheral->tsfn_characteristics_discover);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[4], 0, 1, NULL, NULL, (void *) peripheral, bare_bluetooth_android_peripheral__on_read, &peripheral->tsfn_read);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[5], 0, 1, NULL, NULL, (void *) peripheral, bare_bluetooth_android_peripheral__on_write, &peripheral->tsfn_write);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[6], 0, 1, NULL, NULL, (void *) peripheral, bare_bluetooth_android_peripheral__on_notify, &peripheral->tsfn_notify);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[7], 0, 1, NULL, NULL, (void *) peripheral, bare_bluetooth_android_peripheral__on_notify_state, &peripheral->tsfn_notify_state);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[8], 0, 1, NULL, NULL, (void *) peripheral, bare_bluetooth_android_peripheral__on_channel_open, &peripheral->tsfn_channel_open);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[9], 0, 1, NULL, NULL, (void *) peripheral, bare_bluetooth_android_peripheral__on_mtu_changed, &peripheral->tsfn_mtu_changed);
  assert(err == 0);

  js_value_t *handle;
  err = js_create_external(env, (void *) peripheral, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_bluetooth_android_peripheral_id (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto device = java_object_t<"android/bluetooth/BluetoothDevice">(jenv, peripheral->device);
  auto get_address = device.get_class().get_method<std::string()>("getAddress");
  std::string address = get_address(device);

  js_value_t *result;
  err = js_create_string_utf8(env, (const utf8_t *) address.c_str(), address.size(), &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_peripheral_name (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto device = java_object_t<"android/bluetooth/BluetoothDevice">(jenv, peripheral->device);
  auto get_name = device.get_class().get_method<java_object_t<"java/lang/String">()>("getName");
  auto name_obj = get_name(device);

  if ((jobject) name_obj == nullptr) {
    js_value_t *result;
    err = js_get_null(env, &result);
    assert(err == 0);
    return result;
  }

  const char *name_chars = ((JNIEnv *) jenv)->GetStringUTFChars((jstring)(jobject) name_obj, NULL);
  js_value_t *result;
  err = js_create_string_utf8(env, (const utf8_t *) name_chars, -1, &result);
  assert(err == 0);
  ((JNIEnv *) jenv)->ReleaseStringUTFChars((jstring)(jobject) name_obj, name_chars);

  return result;
}

static js_value_t *
bare_bluetooth_android_peripheral_discover_services (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto discover = gatt.get_class().get_method<bool()>("discoverServices");
  discover(gatt);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_discover_characteristics (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
  assert(err == 0);

  void *service_ptr;
  err = js_get_value_external(env, argv[1], &service_ptr);
  assert(err == 0);

  int service_index = -1;
  for (size_t i = 0; i < peripheral->services.size(); i++) {
    if (peripheral->services[i] == (jobject) service_ptr) {
      service_index = (int) i;
      break;
    }
  }

  auto *event = (bare_bluetooth_android_peripheral_characteristics_discover_t *) malloc(sizeof(bare_bluetooth_android_peripheral_characteristics_discover_t));
  event->service = (jobject) service_ptr;

  if (service_index >= 0 && (size_t) service_index < peripheral->service_characteristics.size()) {
    event->count = (uint32_t) peripheral->service_characteristics[service_index].size();
    event->error = NULL;
  } else {
    event->count = 0;
    event->error = strdup("Service not found in cache");
  }

  js_call_threadsafe_function(peripheral->tsfn_characteristics_discover, event, js_threadsafe_function_nonblocking);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_read (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
  assert(err == 0);

  void *char_ptr;
  err = js_get_value_external(env, argv[1], &char_ptr);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, (jobject) char_ptr);
  auto read_characteristic = gatt.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattCharacteristic">)>("readCharacteristic");
  read_characteristic(gatt, characteristic);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_write (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 4;
  js_value_t *argv[4];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 4);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
  assert(err == 0);

  void *char_ptr;
  err = js_get_value_external(env, argv[1], &char_ptr);
  assert(err == 0);

  js_value_t *arraybuffer;
  size_t offset, length;
  err = js_get_typedarray_info(env, argv[2], NULL, (void **) NULL, &length, &arraybuffer, &offset);
  assert(err == 0);

  void *buf;
  err = js_get_arraybuffer_info(env, arraybuffer, &buf, NULL);
  assert(err == 0);

  uint8_t *data = (uint8_t *) buf + offset;

  bool with_response;
  err = js_get_value_bool(env, argv[3], &with_response);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, (jobject) char_ptr);

  auto set_write_type = characteristic.get_class().get_method<void(int)>("setWriteType");
  set_write_type(characteristic, with_response ? 2 : 1);

  jbyteArray byte_array = ((JNIEnv *) jenv)->NewByteArray((jsize) length);
  ((JNIEnv *) jenv)->SetByteArrayRegion(byte_array, 0, (jsize) length, (const jbyte *) data);
  auto set_value = characteristic.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
  set_value(characteristic, java_array_t<unsigned char>(jenv, byte_array));

  auto write_characteristic = gatt.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattCharacteristic">)>("writeCharacteristic");
  write_characteristic(gatt, characteristic);

  ((JNIEnv *) jenv)->DeleteLocalRef(byte_array);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_subscribe (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
  assert(err == 0);

  void *char_ptr;
  err = js_get_value_external(env, argv[1], &char_ptr);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, (jobject) char_ptr);

  auto set_notify = gatt.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattCharacteristic">, bool)>("setCharacteristicNotification");
  set_notify(gatt, characteristic, true);

  auto uuid_class = java_class_t<"java/util/UUID">(jenv);
  auto from_string = uuid_class.get_static_method<java_object_t<"java/util/UUID">(std::string)>("fromString");
  auto cccd_uuid = from_string(std::string("00002902-0000-1000-8000-00805f9b34fb"));

  auto get_descriptor = characteristic.get_class().get_method<java_object_t<"android/bluetooth/BluetoothGattDescriptor">(java_object_t<"java/util/UUID">)>("getDescriptor");
  auto descriptor = get_descriptor(characteristic, cccd_uuid);

  if ((jobject) descriptor != nullptr) {
    jbyteArray enable_value = ((JNIEnv *) jenv)->NewByteArray(2);
    jbyte enable_bytes[] = {0x01, 0x00};
    ((JNIEnv *) jenv)->SetByteArrayRegion(enable_value, 0, 2, enable_bytes);

    auto set_descriptor_value = descriptor.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
    set_descriptor_value(descriptor, java_array_t<unsigned char>(jenv, enable_value));

    auto write_descriptor = gatt.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattDescriptor">)>("writeDescriptor");
    write_descriptor(gatt, descriptor);

    ((JNIEnv *) jenv)->DeleteLocalRef(enable_value);
  }

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_unsubscribe (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
  assert(err == 0);

  void *char_ptr;
  err = js_get_value_external(env, argv[1], &char_ptr);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto gatt = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, peripheral->gatt);
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, (jobject) char_ptr);

  auto set_notify = gatt.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattCharacteristic">, bool)>("setCharacteristicNotification");
  set_notify(gatt, characteristic, false);

  auto uuid_class = java_class_t<"java/util/UUID">(jenv);
  auto from_string = uuid_class.get_static_method<java_object_t<"java/util/UUID">(std::string)>("fromString");
  auto cccd_uuid = from_string(std::string("00002902-0000-1000-8000-00805f9b34fb"));

  auto get_descriptor = characteristic.get_class().get_method<java_object_t<"android/bluetooth/BluetoothGattDescriptor">(java_object_t<"java/util/UUID">)>("getDescriptor");
  auto descriptor = get_descriptor(characteristic, cccd_uuid);

  if ((jobject) descriptor != nullptr) {
    jbyteArray disable_value = ((JNIEnv *) jenv)->NewByteArray(2);
    jbyte disable_bytes[] = {0x00, 0x00};
    ((JNIEnv *) jenv)->SetByteArrayRegion(disable_value, 0, 2, disable_bytes);

    auto set_descriptor_value = descriptor.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
    set_descriptor_value(descriptor, java_array_t<unsigned char>(jenv, disable_value));

    auto write_descriptor = gatt.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattDescriptor">)>("writeDescriptor");
    write_descriptor(gatt, descriptor);

    ((JNIEnv *) jenv)->DeleteLocalRef(disable_value);
  }

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_request_mtu (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
  assert(err == 0);

  int32_t mtu;
  err = js_get_value_int32(env, argv[1], &mtu);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
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
bare_bluetooth_android_peripheral__l2cap_open_thread (void *arg) {
  auto *ctx = (bare_bluetooth_android_peripheral_l2cap_open_ctx_t *) arg;
  auto *peripheral = ctx->peripheral;
  uint32_t psm = ctx->psm;
  free(ctx);

  JNIEnv *jni_env;
  bare_bluetooth_android_jvm->AttachCurrentThread(&jni_env, NULL);

  jclass device_class = jni_env->GetObjectClass(peripheral->device);
  jmethodID create_channel = jni_env->GetMethodID(device_class, "createInsecureL2capChannel", "(I)Landroid/bluetooth/BluetoothSocket;");
  jobject socket = jni_env->CallObjectMethod(peripheral->device, create_channel, (jint) psm);

  if (jni_env->ExceptionCheck() || socket == NULL) {
    jni_env->ExceptionClear();
    if (!peripheral->destroyed) {
      auto *event = (bare_bluetooth_android_peripheral_channel_open_t *) malloc(sizeof(bare_bluetooth_android_peripheral_channel_open_t));
      event->channel = NULL;
      event->error = strdup("Failed to create L2CAP channel");
      event->psm = psm;
      js_call_threadsafe_function(peripheral->tsfn_channel_open, event, js_threadsafe_function_nonblocking);
    }
    peripheral->l2cap_connecting = false;
    bare_bluetooth_android_jvm->DetachCurrentThread();
    return;
  }

  jclass socket_class = jni_env->GetObjectClass(socket);
  jmethodID connect = jni_env->GetMethodID(socket_class, "connect", "()V");
  jni_env->CallVoidMethod(socket, connect);

  if (jni_env->ExceptionCheck()) {
    jni_env->ExceptionClear();
    if (!peripheral->destroyed) {
      auto *event = (bare_bluetooth_android_peripheral_channel_open_t *) malloc(sizeof(bare_bluetooth_android_peripheral_channel_open_t));
      event->channel = NULL;
      event->error = strdup("L2CAP connect failed");
      event->psm = psm;
      js_call_threadsafe_function(peripheral->tsfn_channel_open, event, js_threadsafe_function_nonblocking);
    }
    peripheral->l2cap_connecting = false;
    bare_bluetooth_android_jvm->DetachCurrentThread();
    return;
  }

  if (!peripheral->destroyed) {
    auto *event = (bare_bluetooth_android_peripheral_channel_open_t *) malloc(sizeof(bare_bluetooth_android_peripheral_channel_open_t));
    event->channel = jni_env->NewGlobalRef(socket);
    event->error = NULL;
    event->psm = psm;
    js_call_threadsafe_function(peripheral->tsfn_channel_open, event, js_threadsafe_function_nonblocking);
  }

  peripheral->l2cap_connecting = false;
  bare_bluetooth_android_jvm->DetachCurrentThread();
}

static js_value_t *
bare_bluetooth_android_peripheral_open_l2cap_channel (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
  assert(err == 0);

  uint32_t psm;
  err = js_get_value_uint32(env, argv[1], &psm);
  assert(err == 0);

  auto *ctx = (bare_bluetooth_android_peripheral_l2cap_open_ctx_t *) malloc(sizeof(bare_bluetooth_android_peripheral_l2cap_open_ctx_t));
  ctx->peripheral = peripheral;
  ctx->psm = psm;

  peripheral->l2cap_connecting = true;
  err = uv_thread_create(&peripheral->l2cap_thread, bare_bluetooth_android_peripheral__l2cap_open_thread, (void *) ctx);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_destroy (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
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

  java_env_t jenv(bare_native_activity->env);

  for (auto &chars : peripheral->service_characteristics) {
    for (auto &c : chars) {
      ((JNIEnv *) jenv)->DeleteGlobalRef(c);
    }
  }

  for (auto &s : peripheral->services) {
    ((JNIEnv *) jenv)->DeleteGlobalRef(s);
  }

  ((JNIEnv *) jenv)->DeleteGlobalRef(peripheral->gatt);
  ((JNIEnv *) jenv)->DeleteGlobalRef(peripheral->device);

  delete peripheral;

  return NULL;
}

static js_value_t *
bare_bluetooth_android_peripheral_service_count (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
  assert(err == 0);

  js_value_t *result;
  err = js_create_uint32(env, (uint32_t) peripheral->services.size(), &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_peripheral_service_at_index (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_peripheral_t *peripheral;
  err = js_get_value_external(env, argv[0], (void **) &peripheral);
  assert(err == 0);

  uint32_t index;
  err = js_get_value_uint32(env, argv[1], &index);
  assert(err == 0);

  js_value_t *result;
  err = js_create_external(env, (void *) peripheral->services[index], NULL, NULL, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_service_key (js_env_t *env, js_callback_info_t *info) {
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
  err = js_create_string_utf8(env, (const utf8_t *) key, -1, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_service_uuid (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  void *handle;
  err = js_get_value_external(env, argv[0], &handle);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto service = java_object_t<"android/bluetooth/BluetoothGattService">(jenv, (jobject) handle);
  auto get_uuid = service.get_class().get_method<java_object_t<"java/util/UUID">()>("getUuid");
  auto uuid_obj = get_uuid(service);
  auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
  std::string uuid_str = to_string(uuid_obj);

  js_value_t *result;
  err = js_create_string_utf8(env, (const utf8_t *) uuid_str.c_str(), uuid_str.size(), &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_service_characteristic_count (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  void *handle;
  err = js_get_value_external(env, argv[0], &handle);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto service = java_object_t<"android/bluetooth/BluetoothGattService">(jenv, (jobject) handle);
  auto get_characteristics = service.get_class().get_method<java_object_t<"java/util/List">()>("getCharacteristics");
  auto list = get_characteristics(service);
  auto list_size = list.get_class().get_method<int()>("size");
  int count = list_size(list);

  js_value_t *result;
  err = js_create_uint32(env, (uint32_t) count, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_service_characteristic_at_index (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  void *handle;
  err = js_get_value_external(env, argv[0], &handle);
  assert(err == 0);

  uint32_t index;
  err = js_get_value_uint32(env, argv[1], &index);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto service = java_object_t<"android/bluetooth/BluetoothGattService">(jenv, (jobject) handle);
  auto get_characteristics = service.get_class().get_method<java_object_t<"java/util/List">()>("getCharacteristics");
  auto list = get_characteristics(service);
  auto list_get = list.get_class().get_method<java_object_t<"java/lang/Object">(int)>("get");
  auto char_obj = list_get(list, (int) index);

  js_value_t *result;
  err = js_create_external(env, (void *)(jobject) char_obj, NULL, NULL, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_characteristic_key (js_env_t *env, js_callback_info_t *info) {
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
  err = js_create_string_utf8(env, (const utf8_t *) key, -1, &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_characteristic_uuid (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  void *handle;
  err = js_get_value_external(env, argv[0], &handle);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, (jobject) handle);
  auto get_uuid = characteristic.get_class().get_method<java_object_t<"java/util/UUID">()>("getUuid");
  auto uuid_obj = get_uuid(characteristic);
  auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
  std::string uuid_str = to_string(uuid_obj);

  js_value_t *result;
  err = js_create_string_utf8(env, (const utf8_t *) uuid_str.c_str(), uuid_str.size(), &result);
  assert(err == 0);

  return result;
}

static js_value_t *
bare_bluetooth_android_characteristic_properties (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  void *handle;
  err = js_get_value_external(env, argv[0], &handle);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, (jobject) handle);
  auto get_properties = characteristic.get_class().get_method<int()>("getProperties");
  int properties = get_properties(characteristic);

  js_value_t *result;
  err = js_create_int32(env, properties, &result);
  assert(err == 0);

  return result;
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnServicesDiscovered (JNIEnv *jni_env, jclass, jlong native_ptr, jobject gatt, jint status) {
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) native_ptr;

  java_env_t jenv(jni_env);

  for (auto &chars : peripheral->service_characteristics) {
    for (auto &c : chars) {
      jni_env->DeleteGlobalRef(c);
    }
  }
  for (auto &s : peripheral->services) {
    jni_env->DeleteGlobalRef(s);
  }
  peripheral->services.clear();
  peripheral->service_characteristics.clear();

  if (status != 0) {
    auto *event = (bare_bluetooth_android_peripheral_services_discover_t *) malloc(sizeof(bare_bluetooth_android_peripheral_services_discover_t));
    event->count = 0;
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = strdup(error_buf);

    js_call_threadsafe_function(peripheral->tsfn_services_discover, event, js_threadsafe_function_nonblocking);
    return;
  }

  auto gatt_obj = java_object_t<"android/bluetooth/BluetoothGatt">(jenv, gatt);
  auto get_services = gatt_obj.get_class().get_method<java_object_t<"java/util/List">()>("getServices");
  auto services_list = get_services(gatt_obj);
  auto list_size = services_list.get_class().get_method<int()>("size");
  auto list_get = services_list.get_class().get_method<java_object_t<"java/lang/Object">(int)>("get");
  int service_count = list_size(services_list);

  for (int i = 0; i < service_count; i++) {
    auto service_obj = list_get(services_list, i);
    jobject service_global = jni_env->NewGlobalRef((jobject) service_obj);
    peripheral->services.push_back(service_global);

    auto service_typed = java_object_t<"android/bluetooth/BluetoothGattService">(jenv, (jobject) service_obj);
    auto get_chars = service_typed.get_class().get_method<java_object_t<"java/util/List">()>("getCharacteristics");
    auto chars_list = get_chars(service_typed);
    auto chars_size = chars_list.get_class().get_method<int()>("size");
    auto chars_get = chars_list.get_class().get_method<java_object_t<"java/lang/Object">(int)>("get");
    int char_count = chars_size(chars_list);

    std::vector<jobject> chars;
    for (int j = 0; j < char_count; j++) {
      auto char_obj = chars_get(chars_list, j);
      chars.push_back(jni_env->NewGlobalRef((jobject) char_obj));
    }
    peripheral->service_characteristics.push_back(std::move(chars));
  }

  auto *event = (bare_bluetooth_android_peripheral_services_discover_t *) malloc(sizeof(bare_bluetooth_android_peripheral_services_discover_t));
  event->count = (uint32_t) service_count;
  event->error = NULL;

  js_call_threadsafe_function(peripheral->tsfn_services_discover, event, js_threadsafe_function_nonblocking);
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnCharacteristicRead (JNIEnv *jni_env, jclass, jlong native_ptr, jobject gatt, jobject characteristic, jbyteArray value, jint status) {
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) native_ptr;

  jclass char_class = jni_env->GetObjectClass(characteristic);
  jmethodID get_uuid = jni_env->GetMethodID(char_class, "getUuid", "()Ljava/util/UUID;");
  jobject uuid_obj = jni_env->CallObjectMethod(characteristic, get_uuid);
  jclass uuid_class = jni_env->GetObjectClass(uuid_obj);
  jmethodID to_string = jni_env->GetMethodID(uuid_class, "toString", "()Ljava/lang/String;");
  jstring uuid_jstr = (jstring) jni_env->CallObjectMethod(uuid_obj, to_string);
  const char *uuid_chars = jni_env->GetStringUTFChars(uuid_jstr, NULL);

  auto *event = (bare_bluetooth_android_peripheral_read_t *) malloc(sizeof(bare_bluetooth_android_peripheral_read_t));
  event->characteristic = jni_env->NewGlobalRef(characteristic);
  event->uuid = strdup(uuid_chars);

  jni_env->ReleaseStringUTFChars(uuid_jstr, uuid_chars);

  if (status == 0 && value != NULL) {
    jsize len = jni_env->GetArrayLength(value);
    event->data = malloc(len);
    event->data_len = (size_t) len;
    jni_env->GetByteArrayRegion(value, 0, len, (jbyte *) event->data);
  } else {
    event->data = NULL;
    event->data_len = 0;
  }

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = strdup(error_buf);
  } else {
    event->error = NULL;
  }

  js_call_threadsafe_function(peripheral->tsfn_read, event, js_threadsafe_function_nonblocking);
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnCharacteristicWrite (JNIEnv *jni_env, jclass, jlong native_ptr, jobject gatt, jobject characteristic, jint status) {
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) native_ptr;

  jclass char_class = jni_env->GetObjectClass(characteristic);
  jmethodID get_uuid = jni_env->GetMethodID(char_class, "getUuid", "()Ljava/util/UUID;");
  jobject uuid_obj = jni_env->CallObjectMethod(characteristic, get_uuid);
  jclass uuid_class = jni_env->GetObjectClass(uuid_obj);
  jmethodID to_string = jni_env->GetMethodID(uuid_class, "toString", "()Ljava/lang/String;");
  jstring uuid_jstr = (jstring) jni_env->CallObjectMethod(uuid_obj, to_string);
  const char *uuid_chars = jni_env->GetStringUTFChars(uuid_jstr, NULL);

  auto *event = (bare_bluetooth_android_peripheral_write_t *) malloc(sizeof(bare_bluetooth_android_peripheral_write_t));
  event->characteristic = jni_env->NewGlobalRef(characteristic);
  event->uuid = strdup(uuid_chars);

  jni_env->ReleaseStringUTFChars(uuid_jstr, uuid_chars);

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = strdup(error_buf);
  } else {
    event->error = NULL;
  }

  js_call_threadsafe_function(peripheral->tsfn_write, event, js_threadsafe_function_nonblocking);
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnCharacteristicChanged (JNIEnv *jni_env, jclass, jlong native_ptr, jobject gatt, jobject characteristic, jbyteArray value) {
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) native_ptr;

  jclass char_class = jni_env->GetObjectClass(characteristic);
  jmethodID get_uuid = jni_env->GetMethodID(char_class, "getUuid", "()Ljava/util/UUID;");
  jobject uuid_obj = jni_env->CallObjectMethod(characteristic, get_uuid);
  jclass uuid_class = jni_env->GetObjectClass(uuid_obj);
  jmethodID to_string = jni_env->GetMethodID(uuid_class, "toString", "()Ljava/lang/String;");
  jstring uuid_jstr = (jstring) jni_env->CallObjectMethod(uuid_obj, to_string);
  const char *uuid_chars = jni_env->GetStringUTFChars(uuid_jstr, NULL);

  auto *event = (bare_bluetooth_android_peripheral_notify_t *) malloc(sizeof(bare_bluetooth_android_peripheral_notify_t));
  event->characteristic = jni_env->NewGlobalRef(characteristic);
  event->uuid = strdup(uuid_chars);
  event->error = NULL;

  jni_env->ReleaseStringUTFChars(uuid_jstr, uuid_chars);

  if (value != NULL) {
    jsize len = jni_env->GetArrayLength(value);
    event->data = malloc(len);
    event->data_len = (size_t) len;
    jni_env->GetByteArrayRegion(value, 0, len, (jbyte *) event->data);
  } else {
    event->data = NULL;
    event->data_len = 0;
  }

  js_call_threadsafe_function(peripheral->tsfn_notify, event, js_threadsafe_function_nonblocking);
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnDescriptorWrite (JNIEnv *jni_env, jclass, jlong native_ptr, jobject gatt, jobject descriptor, jint status) {
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) native_ptr;

  jclass desc_class = jni_env->GetObjectClass(descriptor);
  jmethodID get_uuid = jni_env->GetMethodID(desc_class, "getUuid", "()Ljava/util/UUID;");
  jobject uuid_obj = jni_env->CallObjectMethod(descriptor, get_uuid);
  jclass uuid_class = jni_env->GetObjectClass(uuid_obj);
  jmethodID to_string = jni_env->GetMethodID(uuid_class, "toString", "()Ljava/lang/String;");
  jstring uuid_jstr = (jstring) jni_env->CallObjectMethod(uuid_obj, to_string);
  const char *uuid_chars = jni_env->GetStringUTFChars(uuid_jstr, NULL);

  bool is_cccd = strcmp(uuid_chars, "00002902-0000-1000-8000-00805f9b34fb") == 0;
  jni_env->ReleaseStringUTFChars(uuid_jstr, uuid_chars);

  if (!is_cccd) return;

  jmethodID get_characteristic = jni_env->GetMethodID(desc_class, "getCharacteristic", "()Landroid/bluetooth/BluetoothGattCharacteristic;");
  jobject characteristic = jni_env->CallObjectMethod(descriptor, get_characteristic);

  jclass char_class = jni_env->GetObjectClass(characteristic);
  jmethodID char_get_uuid = jni_env->GetMethodID(char_class, "getUuid", "()Ljava/util/UUID;");
  jobject char_uuid_obj = jni_env->CallObjectMethod(characteristic, char_get_uuid);
  jstring char_uuid_jstr = (jstring) jni_env->CallObjectMethod(char_uuid_obj, to_string);
  const char *char_uuid_chars = jni_env->GetStringUTFChars(char_uuid_jstr, NULL);

  jmethodID get_value = jni_env->GetMethodID(desc_class, "getValue", "()[B");
  jbyteArray value = (jbyteArray) jni_env->CallObjectMethod(descriptor, get_value);
  bool is_notifying = false;

  if (value != NULL) {
    jsize len = jni_env->GetArrayLength(value);
    if (len >= 1) {
      jbyte first_byte;
      jni_env->GetByteArrayRegion(value, 0, 1, &first_byte);
      is_notifying = (first_byte != 0);
    }
  }

  auto *event = (bare_bluetooth_android_peripheral_notify_state_t *) malloc(sizeof(bare_bluetooth_android_peripheral_notify_state_t));
  event->characteristic = jni_env->NewGlobalRef(characteristic);
  event->uuid = strdup(char_uuid_chars);
  event->is_notifying = is_notifying;

  jni_env->ReleaseStringUTFChars(char_uuid_jstr, char_uuid_chars);

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = strdup(error_buf);
  } else {
    event->error = NULL;
  }

  js_call_threadsafe_function(peripheral->tsfn_notify_state, event, js_threadsafe_function_nonblocking);
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnMtuChanged (JNIEnv *jni_env, jclass, jlong native_ptr, jobject gatt, jint mtu, jint status) {
  auto *peripheral = (bare_bluetooth_android_peripheral_t *) native_ptr;

  auto *event = (bare_bluetooth_android_peripheral_mtu_changed_t *) malloc(sizeof(bare_bluetooth_android_peripheral_mtu_changed_t));
  event->mtu = mtu;

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = strdup(error_buf);
  } else {
    event->error = NULL;
  }

  js_call_threadsafe_function(peripheral->tsfn_mtu_changed, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_server__on_state_change (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_server_state_change_t *) data;
  auto *server = (bare_bluetooth_android_server_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[1];
  err = js_create_int32(env, event->state, &argv[0]);
  assert(err == 0);

  free(event);

  js_call_function(env, receiver, function, 1, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_add_service (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_server_add_service_t *) data;
  auto *server = (bare_bluetooth_android_server_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_string_utf8(env, (const utf8_t *) event->uuid, -1, &argv[0]);
  assert(err == 0);
  free(event->uuid);

  if (event->error) {
    err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[1]);
    assert(err == 0);
    free(event->error);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  free(event);

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_read_request (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_server_read_request_t *) data;
  auto *server = (bare_bluetooth_android_server_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[4];

  err = js_create_external(env, (void *) event->device, NULL, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_int32(env, event->request_id, &argv[1]);
  assert(err == 0);

  err = js_create_string_utf8(env, (const utf8_t *) event->characteristic_uuid, -1, &argv[2]);
  assert(err == 0);
  free(event->characteristic_uuid);

  err = js_create_int32(env, event->offset, &argv[3]);
  assert(err == 0);

  free(event);

  js_call_function(env, receiver, function, 4, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_write_request (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_server_write_request_t *) data;
  auto *server = (bare_bluetooth_android_server_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[6];

  err = js_create_external(env, (void *) event->device, NULL, NULL, &argv[0]);
  assert(err == 0);

  err = js_create_int32(env, event->request_id, &argv[1]);
  assert(err == 0);

  err = js_create_string_utf8(env, (const utf8_t *) event->characteristic_uuid, -1, &argv[2]);
  assert(err == 0);
  free(event->characteristic_uuid);

  err = js_create_int32(env, event->offset, &argv[3]);
  assert(err == 0);

  if (event->data) {
    js_value_t *arraybuffer;
    void *buf;
    err = js_create_arraybuffer(env, event->data_len, &buf, &arraybuffer);
    assert(err == 0);
    memcpy(buf, event->data, event->data_len);
    free(event->data);

    err = js_create_typedarray(env, js_uint8array, event->data_len, arraybuffer, 0, &argv[4]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[4]);
    assert(err == 0);
  }

  err = js_get_boolean(env, event->response_needed, &argv[5]);
  assert(err == 0);

  free(event);

  js_call_function(env, receiver, function, 6, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_subscribe (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_server_subscribe_t *) data;
  auto *server = (bare_bluetooth_android_server_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_string_utf8(env, (const utf8_t *) event->device_address, -1, &argv[0]);
  assert(err == 0);
  free(event->device_address);

  err = js_create_string_utf8(env, (const utf8_t *) event->characteristic_uuid, -1, &argv[1]);
  assert(err == 0);
  free(event->characteristic_uuid);

  free(event);

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_unsubscribe (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_server_unsubscribe_t *) data;
  auto *server = (bare_bluetooth_android_server_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_string_utf8(env, (const utf8_t *) event->device_address, -1, &argv[0]);
  assert(err == 0);
  free(event->device_address);

  err = js_create_string_utf8(env, (const utf8_t *) event->characteristic_uuid, -1, &argv[1]);
  assert(err == 0);
  free(event->characteristic_uuid);

  free(event);

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_advertise_error (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_server_advertise_error_t *) data;
  auto *server = (bare_bluetooth_android_server_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_int32(env, event->error_code, &argv[0]);
  assert(err == 0);

  err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[1]);
  assert(err == 0);
  free(event->error);

  free(event);

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_channel_publish (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_server_channel_publish_t *) data;
  auto *server = (bare_bluetooth_android_server_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[2];

  err = js_create_uint32(env, event->psm, &argv[0]);
  assert(err == 0);

  if (event->error) {
    err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[1]);
    assert(err == 0);
    free(event->error);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  free(event);

  js_call_function(env, receiver, function, 2, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_channel_open (js_env_t *env, js_value_t *function, void *context, void *data) {
  int err;

  auto *event = (bare_bluetooth_android_server_channel_open_t *) data;
  auto *server = (bare_bluetooth_android_server_t *) context;

  js_handle_scope_t *scope;
  err = js_open_handle_scope(env, &scope);
  assert(err == 0);

  js_value_t *receiver;
  err = js_get_reference_value(env, server->ctx, &receiver);
  assert(err == 0);

  js_value_t *argv[3];

  if (event->channel) {
    err = js_create_external(env, (void *) event->channel, NULL, NULL, &argv[0]);
    assert(err == 0);
  } else {
    err = js_get_null(env, &argv[0]);
    assert(err == 0);
  }

  if (event->error) {
    err = js_create_string_utf8(env, (const utf8_t *) event->error, -1, &argv[1]);
    assert(err == 0);
    free(event->error);
  } else {
    err = js_get_null(env, &argv[1]);
    assert(err == 0);
  }

  err = js_create_uint32(env, event->psm, &argv[2]);
  assert(err == 0);

  free(event);

  js_call_function(env, receiver, function, 3, argv, NULL);

  err = js_close_handle_scope(env, scope);
  assert(err == 0);
}

static void
bare_bluetooth_android_server__on_connection_state (js_env_t *env, js_value_t *function, void *context, void *data) {
  auto *event = (bare_bluetooth_android_server_connection_state_t *) data;
  auto *server = (bare_bluetooth_android_server_t *) context;

  java_env_t jenv(bare_native_activity->env);

  if (event->new_state == 2) {
    server->connected_devices[std::string(event->address)] = event->device;
  } else if (event->new_state == 0) {
    auto it = server->connected_devices.find(std::string(event->address));
    if (it != server->connected_devices.end()) {
      ((JNIEnv *) jenv)->DeleteGlobalRef(it->second);
      server->connected_devices.erase(it);
    }

    for (auto &pair : server->subscriptions) {
      pair.second.erase(std::string(event->address));
    }

    if (event->device) {
      ((JNIEnv *) jenv)->DeleteGlobalRef(event->device);
    }
  }

  free(event->address);
  free(event);
}

static js_value_t *
bare_bluetooth_android_server_init (js_env_t *env, js_callback_info_t *info) {
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

  err = js_create_threadsafe_function(env, argv[1], 0, 1, NULL, NULL, (void *) server, bare_bluetooth_android_server__on_state_change, &server->tsfn_state_change);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[2], 0, 1, NULL, NULL, (void *) server, bare_bluetooth_android_server__on_add_service, &server->tsfn_add_service);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[3], 0, 1, NULL, NULL, (void *) server, bare_bluetooth_android_server__on_read_request, &server->tsfn_read_request);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[4], 0, 1, NULL, NULL, (void *) server, bare_bluetooth_android_server__on_write_request, &server->tsfn_write_request);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[5], 0, 1, NULL, NULL, (void *) server, bare_bluetooth_android_server__on_subscribe, &server->tsfn_subscribe);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[6], 0, 1, NULL, NULL, (void *) server, bare_bluetooth_android_server__on_unsubscribe, &server->tsfn_unsubscribe);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[7], 0, 1, NULL, NULL, (void *) server, bare_bluetooth_android_server__on_advertise_error, &server->tsfn_advertise_error);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[8], 0, 1, NULL, NULL, (void *) server, bare_bluetooth_android_server__on_channel_publish, &server->tsfn_channel_publish);
  assert(err == 0);

  err = js_create_threadsafe_function(env, argv[9], 0, 1, NULL, NULL, (void *) server, bare_bluetooth_android_server__on_channel_open, &server->tsfn_channel_open);
  assert(err == 0);

  {
    js_value_t *noop;
    err = js_create_function(env, "noop", -1, [](js_env_t *, js_callback_info_t *) -> js_value_t * { return NULL; }, NULL, &noop);
    assert(err == 0);

    err = js_create_threadsafe_function(env, noop, 0, 1, NULL, NULL, (void *) server, bare_bluetooth_android_server__on_connection_state, &server->tsfn_server_connection_state);
    assert(err == 0);
  }

  java_env_t jenv(bare_native_activity->env);

  auto activity = java_object_t<"android/content/Context">(jenv, bare_native_activity->clazz);
  auto get_system_service = activity.get_class().get_method<java_object_t<"java/lang/Object">(std::string)>("getSystemService");
  auto manager_obj = get_system_service(activity, std::string("bluetooth"));
  auto bt_manager = java_object_t<"android/bluetooth/BluetoothManager">(jenv, (jobject) manager_obj);

  auto callback_class = java_class_t<"to/holepunch/bare/bluetooth/GattServerCallback">(jenv);
  auto callback_local = callback_class((long) server);

  auto open_gatt_server = bt_manager.get_class().get_method<java_object_t<"android/bluetooth/BluetoothGattServer">(java_object_t<"android/content/Context">, java_object_t<"android/bluetooth/BluetoothGattServerCallback">)>("openGattServer");
  auto gatt_server_local = open_gatt_server(bt_manager, activity, java_object_t<"android/bluetooth/BluetoothGattServerCallback">(jenv, (jobject) callback_local));

  server->gatt_server = ((JNIEnv *) jenv)->NewGlobalRef((jobject) gatt_server_local);

  auto get_adapter = bt_manager.get_class().get_method<java_object_t<"android/bluetooth/BluetoothAdapter">()>("getAdapter");
  auto adapter = get_adapter(bt_manager);
  auto get_advertiser = adapter.get_class().get_method<java_object_t<"android/bluetooth/le/BluetoothLeAdvertiser">()>("getBluetoothLeAdvertiser");
  auto advertiser_local = get_advertiser(adapter);

  server->advertiser = ((JNIEnv *) jenv)->NewGlobalRef((jobject) advertiser_local);
  server->advertise_callback = NULL;

  auto get_state = adapter.get_class().get_method<int()>("getState");
  int android_state = get_state(adapter);

  auto *state_event = (bare_bluetooth_android_server_state_change_t *) malloc(sizeof(bare_bluetooth_android_server_state_change_t));
  state_event->state = android_state;
  js_call_threadsafe_function(server->tsfn_state_change, state_event, js_threadsafe_function_nonblocking);

  js_value_t *handle;
  err = js_create_external(env, (void *) server, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_bluetooth_android_server_add_service (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], (void **) &server);
  assert(err == 0);

  void *service_ptr;
  err = js_get_value_external(env, argv[1], &service_ptr);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto gatt_server = java_object_t<"android/bluetooth/BluetoothGattServer">(jenv, server->gatt_server);
  auto service = java_object_t<"android/bluetooth/BluetoothGattService">(jenv, (jobject) service_ptr);

  auto add_service = gatt_server.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattService">)>("addService");
  bool result = add_service(gatt_server, service);

  auto get_chars = service.get_class().get_method<java_object_t<"java/util/List">()>("getCharacteristics");
  auto chars_list = get_chars(service);
  auto list_size = chars_list.get_class().get_method<int()>("size");
  auto list_get = chars_list.get_class().get_method<java_object_t<"java/lang/Object">(int)>("get");
  int count = list_size(chars_list);

  for (int i = 0; i < count; i++) {
    auto char_obj = list_get(chars_list, i);
    auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, (jobject) char_obj);
    auto get_uuid = characteristic.get_class().get_method<java_object_t<"java/util/UUID">()>("getUuid");
    auto uuid_obj = get_uuid(characteristic);
    auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
    std::string uuid_str = to_string(uuid_obj);

    server->characteristics[uuid_str] = ((JNIEnv *) jenv)->NewGlobalRef((jobject) char_obj);
  }

  if (!result) {
    auto get_uuid = service.get_class().get_method<java_object_t<"java/util/UUID">()>("getUuid");
    auto uuid_obj = get_uuid(service);
    auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
    std::string uuid_str = to_string(uuid_obj);

    auto *event = (bare_bluetooth_android_server_add_service_t *) malloc(sizeof(bare_bluetooth_android_server_add_service_t));
    event->uuid = strdup(uuid_str.c_str());
    event->error = strdup("Failed to add service");
    js_call_threadsafe_function(server->tsfn_add_service, event, js_threadsafe_function_nonblocking);
  }

  return NULL;
}

static js_value_t *
bare_bluetooth_android_server_start_advertising (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 3;
  js_value_t *argv[3];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 3);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], (void **) &server);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);

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

      auto parcel_uuid = parcel_uuid_class(java_object_t<"java/util/UUID">(jenv, (jobject) uuid_ptr));
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

  auto adv_callback_class = java_class_t<"to/holepunch/bare/bluetooth/AdvertiseCallback">(jenv);
  auto adv_callback = adv_callback_class((long) server);
  server->advertise_callback = ((JNIEnv *) jenv)->NewGlobalRef((jobject) adv_callback);

  auto advertiser = java_object_t<"android/bluetooth/le/BluetoothLeAdvertiser">(jenv, server->advertiser);
  auto start_advertising = advertiser.get_class().get_method<void(java_object_t<"android/bluetooth/le/AdvertiseSettings">, java_object_t<"android/bluetooth/le/AdvertiseData">, java_object_t<"android/bluetooth/le/AdvertiseCallback">)>("startAdvertising");
  start_advertising(advertiser, settings, adv_data, java_object_t<"android/bluetooth/le/AdvertiseCallback">(jenv, server->advertise_callback));

  return NULL;
}

static js_value_t *
bare_bluetooth_android_server_stop_advertising (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], (void **) &server);
  assert(err == 0);

  if (server->advertise_callback) {
    java_env_t jenv(bare_native_activity->env);
    auto advertiser = java_object_t<"android/bluetooth/le/BluetoothLeAdvertiser">(jenv, server->advertiser);
    auto stop = advertiser.get_class().get_method<void(java_object_t<"android/bluetooth/le/AdvertiseCallback">)>("stopAdvertising");
    stop(advertiser, java_object_t<"android/bluetooth/le/AdvertiseCallback">(jenv, server->advertise_callback));

    ((JNIEnv *) jenv)->DeleteGlobalRef(server->advertise_callback);
    server->advertise_callback = NULL;
  }

  return NULL;
}

static js_value_t *
bare_bluetooth_android_server_respond_to_request (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 6;
  js_value_t *argv[6];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 6);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], (void **) &server);
  assert(err == 0);

  void *device_ptr;
  err = js_get_value_external(env, argv[1], &device_ptr);
  assert(err == 0);

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

  java_env_t jenv(bare_native_activity->env);
  auto gatt_server = java_object_t<"android/bluetooth/BluetoothGattServer">(jenv, server->gatt_server);
  auto device = java_object_t<"android/bluetooth/BluetoothDevice">(jenv, (jobject) device_ptr);

  jbyteArray response_data = NULL;

  if (data_type != js_null && data_type != js_undefined) {
    size_t length;
    js_value_t *arraybuffer;
    size_t typed_offset;
    err = js_get_typedarray_info(env, argv[5], NULL, NULL, &length, &arraybuffer, &typed_offset);
    assert(err == 0);

    void *buf;
    err = js_get_arraybuffer_info(env, arraybuffer, &buf, NULL);
    assert(err == 0);

    response_data = ((JNIEnv *) jenv)->NewByteArray((jsize) length);
    ((JNIEnv *) jenv)->SetByteArrayRegion(response_data, 0, (jsize) length, (const jbyte *) ((uint8_t *) buf + typed_offset));
  }

  auto send_response = gatt_server.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothDevice">, int, int, int, java_array_t<unsigned char>)>("sendResponse");
  send_response(gatt_server, device, request_id, result_code, offset, java_array_t<unsigned char>(jenv, response_data));

  if (response_data) {
    ((JNIEnv *) jenv)->DeleteLocalRef(response_data);
  }

  return NULL;
}

static js_value_t *
bare_bluetooth_android_server_update_value (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 3;
  js_value_t *argv[3];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 3);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], (void **) &server);
  assert(err == 0);

  void *char_ptr;
  err = js_get_value_external(env, argv[1], &char_ptr);
  assert(err == 0);

  size_t length;
  js_value_t *arraybuffer;
  size_t offset;
  err = js_get_typedarray_info(env, argv[2], NULL, NULL, &length, &arraybuffer, &offset);
  assert(err == 0);

  void *buf;
  err = js_get_arraybuffer_info(env, arraybuffer, &buf, NULL);
  assert(err == 0);

  uint8_t *data = (uint8_t *) buf + offset;

  java_env_t jenv(bare_native_activity->env);
  auto characteristic = java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, (jobject) char_ptr);
  auto gatt_server = java_object_t<"android/bluetooth/BluetoothGattServer">(jenv, server->gatt_server);

  jbyteArray byte_array = ((JNIEnv *) jenv)->NewByteArray((jsize) length);
  ((JNIEnv *) jenv)->SetByteArrayRegion(byte_array, 0, (jsize) length, (const jbyte *) data);
  auto set_value = characteristic.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
  set_value(characteristic, java_array_t<unsigned char>(jenv, byte_array));

  auto get_uuid = characteristic.get_class().get_method<java_object_t<"java/util/UUID">()>("getUuid");
  auto uuid_obj = get_uuid(characteristic);
  auto to_string = uuid_obj.get_class().get_method<std::string()>("toString");
  std::string uuid_str = to_string(uuid_obj);

  auto it = server->subscriptions.find(uuid_str);
  if (it != server->subscriptions.end()) {
    auto notify = gatt_server.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothDevice">, java_object_t<"android/bluetooth/BluetoothGattCharacteristic">, bool)>("notifyCharacteristicChanged");

    for (const auto &addr : it->second) {
      auto dev_it = server->connected_devices.find(addr);
      if (dev_it != server->connected_devices.end()) {
        auto device = java_object_t<"android/bluetooth/BluetoothDevice">(jenv, dev_it->second);
        notify(gatt_server, device, characteristic, false);
      }
    }
  }

  ((JNIEnv *) jenv)->DeleteLocalRef(byte_array);

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
bare_bluetooth_android_server__accept_thread (void *arg) {
  auto *ctx = (bare_bluetooth_android_server_accept_ctx_t *) arg;
  auto *server = ctx->server;
  auto *ch = ctx->channel;
  free(ctx);

  JNIEnv *jni_env;
  bare_bluetooth_android_jvm->AttachCurrentThread(&jni_env, NULL);

  while (ch->accepting) {
    jclass ss_class = jni_env->GetObjectClass(ch->server_socket);
    jmethodID accept = jni_env->GetMethodID(ss_class, "accept", "()Landroid/bluetooth/BluetoothSocket;");
    jobject client_socket = jni_env->CallObjectMethod(ch->server_socket, accept);

    if (jni_env->ExceptionCheck() || client_socket == NULL) {
      jni_env->ExceptionClear();
      break;
    }

    auto *event = (bare_bluetooth_android_server_channel_open_t *) malloc(sizeof(bare_bluetooth_android_server_channel_open_t));
    event->channel = jni_env->NewGlobalRef(client_socket);
    event->error = NULL;
    event->psm = ch->psm;

    js_call_threadsafe_function(server->tsfn_channel_open, event, js_threadsafe_function_nonblocking);
  }

  bare_bluetooth_android_jvm->DetachCurrentThread();
}

static js_value_t *
bare_bluetooth_android_server_publish_channel (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], (void **) &server);
  assert(err == 0);

  bool encrypted;
  err = js_get_value_bool(env, argv[1], &encrypted);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);

  auto activity = java_object_t<"android/content/Context">(jenv, bare_native_activity->clazz);
  auto get_system_service = activity.get_class().get_method<java_object_t<"java/lang/Object">(std::string)>("getSystemService");
  auto manager_obj = get_system_service(activity, std::string("bluetooth"));
  auto bt_manager = java_object_t<"android/bluetooth/BluetoothManager">(jenv, (jobject) manager_obj);
  auto get_adapter = bt_manager.get_class().get_method<java_object_t<"android/bluetooth/BluetoothAdapter">()>("getAdapter");
  auto adapter = get_adapter(bt_manager);

  jobject server_socket_local;
  if (encrypted) {
    auto listen = adapter.get_class().get_method<java_object_t<"android/bluetooth/BluetoothServerSocket">()>("listenUsingL2capChannel");
    auto ss = listen(adapter);
    server_socket_local = (jobject) ss;
  } else {
    auto listen = adapter.get_class().get_method<java_object_t<"android/bluetooth/BluetoothServerSocket">()>("listenUsingInsecureL2capChannel");
    auto ss = listen(adapter);
    server_socket_local = (jobject) ss;
  }

  if (((JNIEnv *) jenv)->ExceptionCheck() || server_socket_local == NULL) {
    ((JNIEnv *) jenv)->ExceptionClear();
    auto *event = (bare_bluetooth_android_server_channel_publish_t *) malloc(sizeof(bare_bluetooth_android_server_channel_publish_t));
    event->psm = 0;
    event->error = strdup("Failed to create L2CAP server socket");
    js_call_threadsafe_function(server->tsfn_channel_publish, event, js_threadsafe_function_nonblocking);
    return NULL;
  }

  jobject server_socket_global = ((JNIEnv *) jenv)->NewGlobalRef(server_socket_local);

  auto ss_obj = java_object_t<"android/bluetooth/BluetoothServerSocket">(jenv, server_socket_global);
  auto get_psm = ss_obj.get_class().get_method<int()>("getPsm");
  int psm = get_psm(ss_obj);

  auto *ch = new bare_bluetooth_android_server_t::published_channel_t();
  ch->server_socket = server_socket_global;
  ch->accepting = true;
  ch->psm = (uint16_t) psm;
  server->published_channels.push_back(ch);

  auto *event = (bare_bluetooth_android_server_channel_publish_t *) malloc(sizeof(bare_bluetooth_android_server_channel_publish_t));
  event->psm = (uint16_t) psm;
  event->error = NULL;
  js_call_threadsafe_function(server->tsfn_channel_publish, event, js_threadsafe_function_nonblocking);

  auto *ctx = (bare_bluetooth_android_server_accept_ctx_t *) malloc(sizeof(bare_bluetooth_android_server_accept_ctx_t));
  ctx->server = server;
  ctx->channel = ch;
  err = uv_thread_create(&ch->accept_thread, bare_bluetooth_android_server__accept_thread, (void *) ctx);
  assert(err == 0);

  return NULL;
}

static js_value_t *
bare_bluetooth_android_server_unpublish_channel (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], (void **) &server);
  assert(err == 0);

  uint32_t psm;
  err = js_get_value_uint32(env, argv[1], &psm);
  assert(err == 0);

  for (auto it = server->published_channels.begin(); it != server->published_channels.end(); ++it) {
    auto *ch = *it;
    if (ch->psm == (uint16_t) psm) {
      ch->accepting = false;

      java_env_t jenv(bare_native_activity->env);
      auto ss = java_object_t<"android/bluetooth/BluetoothServerSocket">(jenv, ch->server_socket);
      auto close = ss.get_class().get_method<void()>("close");
      close(ss);
      ((JNIEnv *) jenv)->ExceptionClear();

      uv_thread_join(&ch->accept_thread);

      ((JNIEnv *) jenv)->DeleteGlobalRef(ch->server_socket);
      delete ch;
      server->published_channels.erase(it);
      break;
    }
  }

  return NULL;
}

static js_value_t *
bare_bluetooth_android_server_destroy (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 1;
  js_value_t *argv[1];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 1);

  bare_bluetooth_android_server_t *server;
  err = js_get_value_external(env, argv[0], (void **) &server);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);

  for (auto *ch : server->published_channels) {
    ch->accepting = false;
    auto ss = java_object_t<"android/bluetooth/BluetoothServerSocket">(jenv, ch->server_socket);
    auto close_ss = ss.get_class().get_method<void()>("close");
    close_ss(ss);
    ((JNIEnv *) jenv)->ExceptionClear();
    uv_thread_join(&ch->accept_thread);
    ((JNIEnv *) jenv)->DeleteGlobalRef(ch->server_socket);
    delete ch;
  }
  server->published_channels.clear();

  if (server->advertise_callback) {
    auto advertiser = java_object_t<"android/bluetooth/le/BluetoothLeAdvertiser">(jenv, server->advertiser);
    auto stop = advertiser.get_class().get_method<void(java_object_t<"android/bluetooth/le/AdvertiseCallback">)>("stopAdvertising");
    stop(advertiser, java_object_t<"android/bluetooth/le/AdvertiseCallback">(jenv, server->advertise_callback));
    ((JNIEnv *) jenv)->DeleteGlobalRef(server->advertise_callback);
    server->advertise_callback = NULL;
  }

  auto gatt_server = java_object_t<"android/bluetooth/BluetoothGattServer">(jenv, server->gatt_server);
  auto close = gatt_server.get_class().get_method<void()>("close");
  close(gatt_server);

  for (auto &pair : server->characteristics) {
    ((JNIEnv *) jenv)->DeleteGlobalRef(pair.second);
  }
  server->characteristics.clear();

  for (auto &pair : server->connected_devices) {
    ((JNIEnv *) jenv)->DeleteGlobalRef(pair.second);
  }
  server->connected_devices.clear();
  server->subscriptions.clear();

  ((JNIEnv *) jenv)->DeleteGlobalRef(server->gatt_server);
  ((JNIEnv *) jenv)->DeleteGlobalRef(server->advertiser);

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
bare_bluetooth_android_create_mutable_service (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  void *uuid_ptr;
  err = js_get_value_external(env, argv[0], &uuid_ptr);
  assert(err == 0);

  bool is_primary;
  err = js_get_value_bool(env, argv[1], &is_primary);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);

  auto service_class = java_class_t<"android/bluetooth/BluetoothGattService">(jenv);
  auto service = service_class(java_object_t<"java/util/UUID">(jenv, (jobject) uuid_ptr), is_primary ? 0 : 1);

  jobject service_global = ((JNIEnv *) jenv)->NewGlobalRef((jobject) service);

  js_value_t *handle;
  err = js_create_external(env, (void *) service_global, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_bluetooth_android_create_mutable_characteristic (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 4;
  js_value_t *argv[4];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 4);

  void *uuid_ptr;
  err = js_get_value_external(env, argv[0], &uuid_ptr);
  assert(err == 0);

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

  java_env_t jenv(bare_native_activity->env);

  auto char_class = java_class_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv);
  auto characteristic = char_class(java_object_t<"java/util/UUID">(jenv, (jobject) uuid_ptr), properties, android_permissions);

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

    jbyteArray byte_array = ((JNIEnv *) jenv)->NewByteArray((jsize) length);
    ((JNIEnv *) jenv)->SetByteArrayRegion(byte_array, 0, (jsize) length, (const jbyte *) ((uint8_t *) buf + offset));
    auto set_value = characteristic.get_class().get_method<bool(java_array_t<unsigned char>)>("setValue");
    set_value(characteristic, java_array_t<unsigned char>(jenv, byte_array));
    ((JNIEnv *) jenv)->DeleteLocalRef(byte_array);
  }

  jobject char_global = ((JNIEnv *) jenv)->NewGlobalRef((jobject) characteristic);

  js_value_t *handle;
  err = js_create_external(env, (void *) char_global, NULL, NULL, &handle);
  assert(err == 0);

  return handle;
}

static js_value_t *
bare_bluetooth_android_service_set_characteristics (js_env_t *env, js_callback_info_t *info) {
  int err;

  size_t argc = 2;
  js_value_t *argv[2];

  err = js_get_callback_info(env, info, &argc, argv, NULL, NULL);
  assert(err == 0);
  assert(argc == 2);

  void *service_ptr;
  err = js_get_value_external(env, argv[0], &service_ptr);
  assert(err == 0);

  uint32_t count;
  err = js_get_array_length(env, argv[1], &count);
  assert(err == 0);

  java_env_t jenv(bare_native_activity->env);
  auto service = java_object_t<"android/bluetooth/BluetoothGattService">(jenv, (jobject) service_ptr);
  auto add_characteristic = service.get_class().get_method<bool(java_object_t<"android/bluetooth/BluetoothGattCharacteristic">)>("addCharacteristic");

  for (uint32_t i = 0; i < count; i++) {
    js_value_t *char_val;
    err = js_get_element(env, argv[1], i, &char_val);
    assert(err == 0);

    void *char_ptr;
    err = js_get_value_external(env, char_val, &char_ptr);
    assert(err == 0);

    add_characteristic(service, java_object_t<"android/bluetooth/BluetoothGattCharacteristic">(jenv, (jobject) char_ptr));
  }

  return NULL;
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_GattServerCallback_nativeOnConnectionStateChange (JNIEnv *jni_env, jclass, jlong native_ptr, jobject device, jint status, jint new_state) {
  auto *server = (bare_bluetooth_android_server_t *) native_ptr;

  jclass device_class = jni_env->GetObjectClass(device);
  jmethodID get_address = jni_env->GetMethodID(device_class, "getAddress", "()Ljava/lang/String;");
  jstring address_jstr = (jstring) jni_env->CallObjectMethod(device, get_address);
  const char *address_chars = jni_env->GetStringUTFChars(address_jstr, NULL);

  auto *event = (bare_bluetooth_android_server_connection_state_t *) malloc(sizeof(bare_bluetooth_android_server_connection_state_t));
  event->address = strdup(address_chars);
  event->new_state = new_state;
  event->device = (new_state == 2) ? jni_env->NewGlobalRef(device) : NULL;

  jni_env->ReleaseStringUTFChars(address_jstr, address_chars);

  js_call_threadsafe_function(server->tsfn_server_connection_state, event, js_threadsafe_function_nonblocking);
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_GattServerCallback_nativeOnServiceAdded (JNIEnv *jni_env, jclass, jlong native_ptr, jint status, jobject service) {
  auto *server = (bare_bluetooth_android_server_t *) native_ptr;

  jclass service_class = jni_env->GetObjectClass(service);
  jmethodID get_uuid = jni_env->GetMethodID(service_class, "getUuid", "()Ljava/util/UUID;");
  jobject uuid_obj = jni_env->CallObjectMethod(service, get_uuid);
  jclass uuid_class = jni_env->GetObjectClass(uuid_obj);
  jmethodID to_string = jni_env->GetMethodID(uuid_class, "toString", "()Ljava/lang/String;");
  jstring uuid_jstr = (jstring) jni_env->CallObjectMethod(uuid_obj, to_string);
  const char *uuid_chars = jni_env->GetStringUTFChars(uuid_jstr, NULL);

  auto *event = (bare_bluetooth_android_server_add_service_t *) malloc(sizeof(bare_bluetooth_android_server_add_service_t));
  event->uuid = strdup(uuid_chars);

  jni_env->ReleaseStringUTFChars(uuid_jstr, uuid_chars);

  if (status != 0) {
    char error_buf[64];
    snprintf(error_buf, sizeof(error_buf), "GATT error %d", status);
    event->error = strdup(error_buf);
  } else {
    event->error = NULL;
  }

  js_call_threadsafe_function(server->tsfn_add_service, event, js_threadsafe_function_nonblocking);
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_GattServerCallback_nativeOnCharacteristicReadRequest (JNIEnv *jni_env, jclass, jlong native_ptr, jobject device, jint request_id, jint offset, jobject characteristic) {
  auto *server = (bare_bluetooth_android_server_t *) native_ptr;

  jclass char_class = jni_env->GetObjectClass(characteristic);
  jmethodID get_uuid = jni_env->GetMethodID(char_class, "getUuid", "()Ljava/util/UUID;");
  jobject uuid_obj = jni_env->CallObjectMethod(characteristic, get_uuid);
  jclass uuid_class = jni_env->GetObjectClass(uuid_obj);
  jmethodID to_string = jni_env->GetMethodID(uuid_class, "toString", "()Ljava/lang/String;");
  jstring uuid_jstr = (jstring) jni_env->CallObjectMethod(uuid_obj, to_string);
  const char *uuid_chars = jni_env->GetStringUTFChars(uuid_jstr, NULL);

  auto *event = (bare_bluetooth_android_server_read_request_t *) malloc(sizeof(bare_bluetooth_android_server_read_request_t));
  event->device = jni_env->NewGlobalRef(device);
  event->request_id = request_id;
  event->characteristic_uuid = strdup(uuid_chars);
  event->offset = offset;

  jni_env->ReleaseStringUTFChars(uuid_jstr, uuid_chars);

  js_call_threadsafe_function(server->tsfn_read_request, event, js_threadsafe_function_nonblocking);
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_GattServerCallback_nativeOnCharacteristicWriteRequest (JNIEnv *jni_env, jclass, jlong native_ptr, jobject device, jint request_id, jobject characteristic, jboolean prepared_write, jboolean response_needed, jint offset, jbyteArray value) {
  auto *server = (bare_bluetooth_android_server_t *) native_ptr;

  jclass char_class = jni_env->GetObjectClass(characteristic);
  jmethodID get_uuid = jni_env->GetMethodID(char_class, "getUuid", "()Ljava/util/UUID;");
  jobject uuid_obj = jni_env->CallObjectMethod(characteristic, get_uuid);
  jclass uuid_class = jni_env->GetObjectClass(uuid_obj);
  jmethodID to_string = jni_env->GetMethodID(uuid_class, "toString", "()Ljava/lang/String;");
  jstring uuid_jstr = (jstring) jni_env->CallObjectMethod(uuid_obj, to_string);
  const char *uuid_chars = jni_env->GetStringUTFChars(uuid_jstr, NULL);

  auto *event = (bare_bluetooth_android_server_write_request_t *) malloc(sizeof(bare_bluetooth_android_server_write_request_t));
  event->device = jni_env->NewGlobalRef(device);
  event->request_id = request_id;
  event->characteristic_uuid = strdup(uuid_chars);
  event->offset = offset;
  event->response_needed = (bool) response_needed;

  jni_env->ReleaseStringUTFChars(uuid_jstr, uuid_chars);

  if (value != NULL) {
    jsize len = jni_env->GetArrayLength(value);
    event->data = malloc(len);
    event->data_len = (size_t) len;
    jni_env->GetByteArrayRegion(value, 0, len, (jbyte *) event->data);
  } else {
    event->data = NULL;
    event->data_len = 0;
  }

  js_call_threadsafe_function(server->tsfn_write_request, event, js_threadsafe_function_nonblocking);
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_GattServerCallback_nativeOnDescriptorWriteRequest (JNIEnv *jni_env, jclass, jlong native_ptr, jobject device, jint request_id, jobject descriptor, jboolean prepared_write, jboolean response_needed, jint offset, jbyteArray value) {
  auto *server = (bare_bluetooth_android_server_t *) native_ptr;

  jclass desc_class = jni_env->GetObjectClass(descriptor);
  jmethodID get_desc_uuid = jni_env->GetMethodID(desc_class, "getUuid", "()Ljava/util/UUID;");
  jobject desc_uuid_obj = jni_env->CallObjectMethod(descriptor, get_desc_uuid);
  jclass uuid_class = jni_env->GetObjectClass(desc_uuid_obj);
  jmethodID to_string = jni_env->GetMethodID(uuid_class, "toString", "()Ljava/lang/String;");
  jstring desc_uuid_jstr = (jstring) jni_env->CallObjectMethod(desc_uuid_obj, to_string);
  const char *desc_uuid_chars = jni_env->GetStringUTFChars(desc_uuid_jstr, NULL);

  bool is_cccd = strcmp(desc_uuid_chars, "00002902-0000-1000-8000-00805f9b34fb") == 0;
  jni_env->ReleaseStringUTFChars(desc_uuid_jstr, desc_uuid_chars);

  if (response_needed) {
    jclass server_class = jni_env->FindClass("android/bluetooth/BluetoothGattServer");
    jmethodID send_response = jni_env->GetMethodID(server_class, "sendResponse", "(Landroid/bluetooth/BluetoothDevice;III[B)Z");
    jni_env->CallBooleanMethod(server->gatt_server, send_response, device, request_id, 0, offset, value);
  }

  if (!is_cccd) return;

  jmethodID get_characteristic = jni_env->GetMethodID(desc_class, "getCharacteristic", "()Landroid/bluetooth/BluetoothGattCharacteristic;");
  jobject characteristic = jni_env->CallObjectMethod(descriptor, get_characteristic);
  jclass char_class = jni_env->GetObjectClass(characteristic);
  jmethodID get_char_uuid = jni_env->GetMethodID(char_class, "getUuid", "()Ljava/util/UUID;");
  jobject char_uuid_obj = jni_env->CallObjectMethod(characteristic, get_char_uuid);
  jstring char_uuid_jstr = (jstring) jni_env->CallObjectMethod(char_uuid_obj, to_string);
  const char *char_uuid_chars = jni_env->GetStringUTFChars(char_uuid_jstr, NULL);
  std::string char_uuid(char_uuid_chars);
  jni_env->ReleaseStringUTFChars(char_uuid_jstr, char_uuid_chars);

  jclass device_class = jni_env->GetObjectClass(device);
  jmethodID get_address = jni_env->GetMethodID(device_class, "getAddress", "()Ljava/lang/String;");
  jstring address_jstr = (jstring) jni_env->CallObjectMethod(device, get_address);
  const char *address_chars = jni_env->GetStringUTFChars(address_jstr, NULL);
  std::string device_address(address_chars);
  jni_env->ReleaseStringUTFChars(address_jstr, address_chars);

  bool subscribing = false;
  if (value != NULL) {
    jsize len = jni_env->GetArrayLength(value);
    if (len >= 1) {
      jbyte first_byte;
      jni_env->GetByteArrayRegion(value, 0, 1, &first_byte);
      subscribing = (first_byte != 0);
    }
  }

  if (subscribing) {
    server->subscriptions[char_uuid].insert(device_address);

    auto *event = (bare_bluetooth_android_server_subscribe_t *) malloc(sizeof(bare_bluetooth_android_server_subscribe_t));
    event->device_address = strdup(device_address.c_str());
    event->characteristic_uuid = strdup(char_uuid.c_str());
    js_call_threadsafe_function(server->tsfn_subscribe, event, js_threadsafe_function_nonblocking);
  } else {
    server->subscriptions[char_uuid].erase(device_address);

    auto *event = (bare_bluetooth_android_server_unsubscribe_t *) malloc(sizeof(bare_bluetooth_android_server_unsubscribe_t));
    event->device_address = strdup(device_address.c_str());
    event->characteristic_uuid = strdup(char_uuid.c_str());
    js_call_threadsafe_function(server->tsfn_unsubscribe, event, js_threadsafe_function_nonblocking);
  }
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_AdvertiseCallback_nativeOnStartSuccess (JNIEnv *jni_env, jclass, jlong native_ptr, jobject settings) {
  (void) jni_env;
  (void) native_ptr;
  (void) settings;
}

extern "C" JNIEXPORT void JNICALL
Java_to_holepunch_bare_bluetooth_AdvertiseCallback_nativeOnStartFailure (JNIEnv *jni_env, jclass, jlong native_ptr, jint error_code) {
  auto *server = (bare_bluetooth_android_server_t *) native_ptr;

  const char *message;
  switch (error_code) {
  case 1: message = "Data too large"; break;
  case 2: message = "Too many advertisers"; break;
  case 3: message = "Already started"; break;
  case 4: message = "Internal error"; break;
  case 5: message = "Feature unsupported"; break;
  default: message = "Unknown advertise error"; break;
  }

  auto *event = (bare_bluetooth_android_server_advertise_error_t *) malloc(sizeof(bare_bluetooth_android_server_advertise_error_t));
  event->error_code = error_code;
  event->error = strdup(message);

  js_call_threadsafe_function(server->tsfn_advertise_error, event, js_threadsafe_function_nonblocking);
}

static void
bare_bluetooth_android_register_natives () {
  JNIEnv *jni = bare_native_activity->env;

  {
    jclass cls = jni->FindClass("to/holepunch/bare/bluetooth/GattServerCallback");
    if (jni->ExceptionCheck()) jni->ExceptionClear();
    if (cls) {
      JNINativeMethod methods[] = {
        {(char *) "nativeOnConnectionStateChange", (char *) "(JLandroid/bluetooth/BluetoothDevice;II)V", (void *) Java_to_holepunch_bare_bluetooth_GattServerCallback_nativeOnConnectionStateChange},
        {(char *) "nativeOnServiceAdded", (char *) "(JILandroid/bluetooth/BluetoothGattService;)V", (void *) Java_to_holepunch_bare_bluetooth_GattServerCallback_nativeOnServiceAdded},
        {(char *) "nativeOnCharacteristicReadRequest", (char *) "(JLandroid/bluetooth/BluetoothDevice;IILandroid/bluetooth/BluetoothGattCharacteristic;)V", (void *) Java_to_holepunch_bare_bluetooth_GattServerCallback_nativeOnCharacteristicReadRequest},
        {(char *) "nativeOnCharacteristicWriteRequest", (char *) "(JLandroid/bluetooth/BluetoothDevice;ILandroid/bluetooth/BluetoothGattCharacteristic;ZZI[B)V", (void *) Java_to_holepunch_bare_bluetooth_GattServerCallback_nativeOnCharacteristicWriteRequest},
        {(char *) "nativeOnDescriptorWriteRequest", (char *) "(JLandroid/bluetooth/BluetoothDevice;ILandroid/bluetooth/BluetoothGattDescriptor;ZZI[B)V", (void *) Java_to_holepunch_bare_bluetooth_GattServerCallback_nativeOnDescriptorWriteRequest},
      };
      jni->RegisterNatives(cls, methods, sizeof(methods) / sizeof(methods[0]));
      jni->DeleteLocalRef(cls);
    }
  }

  {
    jclass cls = jni->FindClass("to/holepunch/bare/bluetooth/GattCallback");
    if (jni->ExceptionCheck()) jni->ExceptionClear();
    if (cls) {
      JNINativeMethod methods[] = {
        {(char *) "nativeOnConnectionStateChange", (char *) "(JLandroid/bluetooth/BluetoothGatt;II)V", (void *) Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnConnectionStateChange},
        {(char *) "nativeOnServicesDiscovered", (char *) "(JLandroid/bluetooth/BluetoothGatt;I)V", (void *) Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnServicesDiscovered},
        {(char *) "nativeOnCharacteristicRead", (char *) "(JLandroid/bluetooth/BluetoothGatt;Landroid/bluetooth/BluetoothGattCharacteristic;[BI)V", (void *) Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnCharacteristicRead},
        {(char *) "nativeOnCharacteristicWrite", (char *) "(JLandroid/bluetooth/BluetoothGatt;Landroid/bluetooth/BluetoothGattCharacteristic;I)V", (void *) Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnCharacteristicWrite},
        {(char *) "nativeOnCharacteristicChanged", (char *) "(JLandroid/bluetooth/BluetoothGatt;Landroid/bluetooth/BluetoothGattCharacteristic;[B)V", (void *) Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnCharacteristicChanged},
        {(char *) "nativeOnDescriptorWrite", (char *) "(JLandroid/bluetooth/BluetoothGatt;Landroid/bluetooth/BluetoothGattDescriptor;I)V", (void *) Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnDescriptorWrite},
        {(char *) "nativeOnMtuChanged", (char *) "(JLandroid/bluetooth/BluetoothGatt;II)V", (void *) Java_to_holepunch_bare_bluetooth_GattCallback_nativeOnMtuChanged},
      };
      jni->RegisterNatives(cls, methods, sizeof(methods) / sizeof(methods[0]));
      jni->DeleteLocalRef(cls);
    }
  }

  {
    jclass cls = jni->FindClass("to/holepunch/bare/bluetooth/ScanCallback");
    if (jni->ExceptionCheck()) jni->ExceptionClear();
    if (cls) {
      JNINativeMethod methods[] = {
        {(char *) "nativeOnScanResult", (char *) "(JILandroid/bluetooth/le/ScanResult;)V", (void *) Java_to_holepunch_bare_bluetooth_ScanCallback_nativeOnScanResult},
        {(char *) "nativeOnScanFailed", (char *) "(JI)V", (void *) Java_to_holepunch_bare_bluetooth_ScanCallback_nativeOnScanFailed},
      };
      jni->RegisterNatives(cls, methods, sizeof(methods) / sizeof(methods[0]));
      jni->DeleteLocalRef(cls);
    }
  }

  {
    jclass cls = jni->FindClass("to/holepunch/bare/bluetooth/AdvertiseCallback");
    if (jni->ExceptionCheck()) jni->ExceptionClear();
    if (cls) {
      JNINativeMethod methods[] = {
        {(char *) "nativeOnStartSuccess", (char *) "(JLandroid/bluetooth/le/AdvertiseSettings;)V", (void *) Java_to_holepunch_bare_bluetooth_AdvertiseCallback_nativeOnStartSuccess},
        {(char *) "nativeOnStartFailure", (char *) "(JI)V", (void *) Java_to_holepunch_bare_bluetooth_AdvertiseCallback_nativeOnStartFailure},
      };
      jni->RegisterNatives(cls, methods, sizeof(methods) / sizeof(methods[0]));
      jni->DeleteLocalRef(cls);
    }
  }
}

static js_value_t *
bare_bluetooth_android_exports (js_env_t *env, js_value_t *exports) {
  int err;

  if (jnitl__class_loader.loader == nullptr) {
    void *runtime = dlopen("libbare.so", RTLD_NOLOAD);
    if (runtime) {
      auto *runtime_cl = (jnitl_class_loader_t *) dlsym(runtime, "jnitl__class_loader");
      if (runtime_cl && runtime_cl->loader) {
        jnitl__class_loader = *runtime_cl;
      }
      dlclose(runtime);
    }
  }

  bare_bluetooth_android_register_natives();

  if (bare_bluetooth_android_jvm == NULL) {
    ((JNIEnv *) bare_native_activity->env)->GetJavaVM(&bare_bluetooth_android_jvm);
  }

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
