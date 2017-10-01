#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

// NAPI
#include <node_api.h>

// Keep a handle on some stuff
typedef struct {
    napi_ref        _callback;
    napi_async_work _request;

    int    num_frames;
    void  *data;
    size_t data_length;
} carrier;

// Global var keeping handle on callback and task
carrier the_carrier;


// Parse the argument as an object with fields: frames,  ...
//
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

    // Only one argument, an object
    if (argc != 1) {
        napi_throw_error(env, "error", "Should have only one parameter");
        return nullptr;
    }

    // Get the 'frames' field
    napi_value field;
    status = napi_create_string_utf8(env, "frames", -1, &field);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_create_string_utf8");
        return nullptr;
    }

    // Get the property
    napi_value fieldvalue;
    status = napi_get_property(env, argv[0], field, &fieldvalue);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_get_property");
        return nullptr;
    }

    // Get the integer value
    int32_t fieldint;
    status = napi_get_value_int32(env, fieldvalue, &fieldint);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_get_value_int32");
        return nullptr;
    }

    // Store the value in global variable
    the_carrier.num_frames = fieldint;
    fprintf(stderr, ">> FRAMES %d\n", the_carrier.num_frames);

    return nullptr;
}

// TASK
void doStreaming(napi_env env, void* param) {

    fprintf(stderr, "do streaming\n");

    // get the parameter of the task
    carrier* c = static_cast<carrier*>(param);


    // do some work
    usleep(100000);

    size_t msgsize = 4*1024*1024;
    // create some data
    void *input_data = (void*)malloc(msgsize);
    // put some values in
    memset(input_data, 0, msgsize);


    // Put the data in the data structure
    c->data = input_data;
    c->data_length = msgsize;
}

// When task is done
void doneStreaming(napi_env env, napi_status status, void* data) {
    fprintf(stderr, "done streaming %d\n", status == napi_ok);

    napi_status lstatus;

    // get the parameter of the task
    carrier* c = static_cast<carrier*>(data);

    // Get the callback from the reference in global memory
    napi_value callback;
    lstatus = napi_get_reference_value(env, c->_callback, &callback);
    if (lstatus != napi_ok) {
        napi_throw_error(env, "error", "napi_get_reference_value");
        return;
    }

    // global object needed to call a function 
    napi_value global;
    lstatus = napi_get_global(env, &global);
    if (lstatus != napi_ok) {
        napi_throw_error(env, "error", "napi_get_global");
        return;
    }

    // Prepare the return buffer
    napi_value myBuffer;

    fprintf(stderr, "Before creating buffer %ld\n", c->data_length);


#if 1
    // create a buffer with a copy
    void *result_data;

    lstatus = napi_create_buffer_copy(env, c->data_length, c->data, &result_data, &myBuffer);

    if (lstatus != napi_ok) {
        napi_throw_error(env, "error", "napi_create_buffer_copy");
    }
    free(c->data);

#else
    lstatus = napi_create_external_buffer(env, msgsize, input_data,
                nullptr, nullptr,  // no handlers for now
                &myBuffer);

    if (lstatus != napi_ok) {
        napi_throw_error(env, "error", "napi_create_external_buffer");
    }
#endif

    fprintf(stderr, "After creating buffer\n");

    // Trigger the callback
    napi_value result;
    fprintf(stderr, "Before call\n");

    napi_value ret[1];
    ret[0]  = myBuffer;
    lstatus = napi_call_function(env, global, callback, 1, ret, &result);
    if (lstatus != napi_ok) {
        napi_throw_error(env, "error", "napi_call_function");
    }

    fprintf(stderr, "After call\n");

    c->num_frames = c->num_frames - 1;
    if (c->num_frames > 0) {
        // async_resource_name
        napi_value rsrc;
        status = napi_create_string_utf8(env, "streaming", -1, &rsrc);

        // Create the task
        lstatus = napi_create_async_work(env, 0, rsrc,
            doStreaming, doneStreaming, c, &c->_request);
        if (lstatus != napi_ok) {
            napi_throw_error(env, "error", "napi_create_async_work");
        }

        // Put the task in the queue
        lstatus = napi_queue_async_work(env, c->_request);
        if (lstatus != napi_ok) {
            napi_throw_error(env, "error", "napi_queue_async_work");
        }
    }
}

// Set a callbak to call, called when we get data
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

    // first parameter is a callback, keep a reference to it
    napi_value cb = argv[0];
    napi_create_reference(env, cb, 1, &the_carrier._callback);
    return nullptr;
}


napi_value startStream(napi_env env, napi_callback_info info) {
    napi_status status;

    // async_resource_name
    napi_value rsrc;
    status = napi_create_string_utf8(env, "streaming", -1, &rsrc);

    // Create the task
    status = napi_create_async_work(env, nullptr, rsrc,
        doStreaming, doneStreaming, &the_carrier, &the_carrier._request);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_create_async_work");
    }

    // Put the task in the queue
    status = napi_queue_async_work(env, the_carrier._request);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_queue_async_work");
    }

    return nullptr;
}

napi_value stopStream(napi_env env, napi_callback_info info) {
    // nothing yet
    return nullptr;
}

napi_value Init(napi_env env, napi_value exports) {
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
    return exports;
}

NAPI_MODULE(addon, Init)
