#include <stdio.h>

// NAPI
#include <node_api.h>


typedef struct {
    napi_ref        _callback;
    napi_async_work _request;
} carrier;

carrier the_carrier;

// number of iteration
int num_frames;


napi_value initStream(napi_env env, napi_callback_info info) {

    napi_status status;
    size_t argc = 1;
    napi_value argv[1];

    // Parameters
    status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);    
    if (status != napi_ok) {
        napi_throw_error(env, "error", "Failed to parse arguments");
        return nullptr;
    }

    if (argc != 1) {
        napi_throw_error(env, "error", "Should have only one parameter");
        return nullptr;
    }

    // Get the oject keys of the parameter
    napi_value keys;
    status = napi_get_property_names(env, argv[0], &keys);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_get_property_names");
        return nullptr;
    }

    napi_value field, fieldvalue;
    char fieldname[256];
    char fieldstring[256];
    long fieldint;
    size_t fieldsize;
    status = napi_get_element(env, keys, 0, &field);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_get_element");
        return nullptr;
    }

    memset(fieldname, 0, 256);
    status = napi_get_value_string_utf8(env, field, fieldname, 256, &fieldsize);
    if (status != napi_ok) {
        fprintf(stderr, "   error %d\n", status);
        napi_throw_error(env, "error", "napi_get_value_string_utf8");
        return nullptr;
    }

    status = napi_get_property(env, argv[0], field, &fieldvalue);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_get_property");
        return nullptr;
    }

    napi_valuetype result;
    status = napi_typeof(env, fieldvalue, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_typeof");
        return nullptr;
    }

    status = napi_get_value_int64(env, fieldvalue, &fieldint);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_get_value_int64");
        return nullptr;
    }

    if (!strcmp(fieldname, "frames")) {
        num_frames = fieldint;
        fprintf(stderr, ">> FRAMES %d\n", num_frames);
    }


    if (status != napi_ok) {
        napi_throw_error(env, "error", "Invalid number was passed as argument");
        return nullptr;
    }

    return nullptr;
}

void doStreaming(napi_env env, void* param) {
    fprintf(stderr, "doStreaming\n");

    napi_status lstatus;

    // get the parameter of the task
    carrier* c = static_cast<carrier*>(param);

    napi_value callback;
    lstatus = napi_get_reference_value(env, c->_callback, &callback);
    if (lstatus != napi_ok) {
        fprintf(stderr, "Error1 %d\n", lstatus);
        napi_throw_error(env, "error", "napi_get_reference_value");
        return;
    }

    napi_value global;
    lstatus = napi_get_global(env, &global);
    if (lstatus != napi_ok) {
        napi_throw_error(env, "error", "napi_get_global");
        return;
    }

    // Do the work
    int i = 0;
    while (i < num_frames) {

#if 1

        // // Prepare argument
        napi_value myBuffer;
        size_t msgsize = 44;
        // void *data = (void*)malloc(msgsize);
        void *ptr;
        fprintf(stderr, "Before napi_create_buffer %ld\n", msgsize);
        // lstatus = napi_create_buffer(env, msgsize, &ptr, &myBuffer);
        // lstatus = napi_create_arraybuffer(env, msgsize, &ptr, &myBuffer);
        lstatus = napi_create_external_buffer(env,
                                 msgsize, ptr,
                                 0, 0,
                                 &myBuffer);

        // lstatus = napi_ok;
        fprintf(stderr, "After napi_create_buffer\n");
        if (lstatus != napi_ok) {
            napi_throw_error(env, "error", "napi_create_buffer");
        }


        // Trigger the callback
        napi_value result;
        fprintf(stderr, "Before call\n");
        // lstatus = napi_call_function(env, global, callback, 1, &myBuffer, &result);
        napi_value ret[1];
        ret[0] = myBuffer;
        lstatus = napi_call_function(env, global, callback, 1, ret, &result);

        // lstatus = napi_make_callback(env,
        //     nullptr,
        //     callback,
        //     1,
        //     ret,
        //     &result);

        // if (lstatus != napi_ok) {
        //     napi_throw_error(env, "error", "napi_call_function");
        // }
#else
        fprintf(stderr, "Going to sleep\n");
        sleep(3);
#endif
        i++;
    }
}

void doneStreaming(napi_env env, napi_status status, void* data) {
    fprintf(stderr, "doneStreaming %d\n", status == napi_ok);  
}

napi_value setHandler(napi_env env, napi_callback_info info) {
    napi_status status;
    size_t argc = 1;
    napi_value argv[1];

    // get the paramters
    status = napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);    
    if (status != napi_ok) {
        napi_throw_error(env, "error", "Failed to parse arguments");
    }

    if (argc != 1) {
        napi_throw_error(env, "error", "Should have only one parameter");
        return nullptr;
    }

    // first parameter is a callback
    napi_value cb = argv[0];
    napi_create_reference(env, cb, 1, &the_carrier._callback);
    return nullptr;
}


napi_value startStream(napi_env env, napi_callback_info info) {
    napi_status status;

#if 1
    status = napi_create_async_work(env,
                doStreaming, doneStreaming,
                &the_carrier, &the_carrier._request);

    fprintf(stderr, "Init carrier request %p\n", the_carrier._request);

    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_create_async_work");
    }
    status = napi_queue_async_work(env, the_carrier._request);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_queue_async_work");
    }
#else
    // the_carrier._request = 0;
    // doStreaming(env, &the_carrier);
#endif

    return nullptr;
}

napi_value stopStream(napi_env env, napi_callback_info info) {
    // nothing yet
    return nullptr;
}

void Init(napi_env env, napi_value exports, napi_value module, void* priv) {
    // Module initialization code goes here
    napi_property_descriptor descs[] = {
        { "initStream",  0, initStream,  0, 0, 0, napi_default, 0 },
        { "startStream", 0, startStream, 0, 0, 0, napi_default, 0 },
        { "setHandler",  0, setHandler,  0, 0, 0, napi_default, 0 },
        { "stopStream",  0, stopStream,  0, 0, 0, napi_default, 0 }
    };
    napi_status status = napi_define_properties(env, exports, 4, descs);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "Error napi_define_properties");
    }
}


NAPI_MODULE(addon, Init)
