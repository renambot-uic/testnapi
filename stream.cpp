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
} carrier;

// Global var keeping handle on callback and task
carrier the_carrier;

// number of iteration
int num_frames;

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
    num_frames = fieldint;
    fprintf(stderr, ">> FRAMES %d\n", num_frames);

    return nullptr;
}

// TASK
void doStreaming(napi_env env, void* param) {
    napi_status lstatus;

    fprintf(stderr, "do streaming\n");

    // get the parameter of the task
    carrier* c = static_cast<carrier*>(param);

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

    // Do the work
    int i = 0;
    while (i < num_frames) {

        // do some work
        sleep(1);


        // Prepare the return buffer
        napi_value myBuffer;
        size_t msgsize = 44;  // some length

        fprintf(stderr, "Before creating buffer %ld\n", msgsize);

        // create some data
        void *input_data = (void*)malloc(msgsize);
        // put some values in
        memset(input_data, 0, msgsize);

        // Diffent ways to create a buffer

#if 1
        // create a buffer with a copy
        void *result_data;

        lstatus = napi_create_buffer_copy(env,msgsize, input_data, &result_data, &myBuffer);

        if (lstatus != napi_ok) {
            napi_throw_error(env, "error", "napi_create_buffer_copy");
        }
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

        i++;
    }
}

// When task is done
void doneStreaming(napi_env env, napi_status status, void* data) {
    fprintf(stderr, "done streaming %d\n", status == napi_ok);  
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

// 
// ASYNC OR NOT
// 

#if 1
    // Create the task
    status = napi_create_async_work(env, doStreaming, doneStreaming,
                &the_carrier, &the_carrier._request);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_create_async_work");
    }

    // Put the task in the queue
    status = napi_queue_async_work(env, the_carrier._request);
    if (status != napi_ok) {
        napi_throw_error(env, "error", "napi_queue_async_work");
    }

#else

    // Synchronous, blocking
    the_carrier._request = 0;
    doStreaming(env, &the_carrier);

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
