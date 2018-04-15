#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include "debug.h"
#include "msg_queue.h"

#define tag "test"
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

static MessageQueue msg_queue;
static int running = 0;
static pthread_t tid;
static void *queue_runnable(void *arg);
//static jmethodID on_event_cb_id;
static JavaVM *gJVM = NULL;
static jobject gObj = NULL;

JNIEXPORT jstring jni_get_string(JNIEnv *env, jobject thiz) {
    const char* hello = "Hello, test native code with mwmwatch.";
    return (*env)->NewStringUTF(env, hello);
}
static jboolean jni_init(JNIEnv *env, jobject thiz)
{
    gObj = (*env)->NewGlobalRef(env, thiz);
    jclass clazz = (*env)->GetObjectClass(env, thiz);
    if (clazz == NULL) {
        (*env)->ThrowNew(env, "java/lang/NullPointerException", "Unable to find exception class");
    }
    //on_event_cb_id = (*env)->GetMethodID(env, clazz, "postEventFromNative", "(IIILjava/lang/Object;)V");
    //if (!on_event_cb_id) return JNI_FALSE;
    int ret = pthread_create(&tid, NULL, queue_runnable, NULL);
    if (ret != NULL) return JNI_FALSE;
    return JNI_TRUE;
}
static jboolean jni_destroy(JNIEnv *env, jobject thiz)
{
    logi(tag, "%s", __func__);
    msg_queue_abort(&msg_queue);
    running = 0;
    msg_queue_destroy(&msg_queue);
    return JNI_TRUE;
}
static void on_event_callback(int what, int arg1, int arg2, void *obj, int len)
{
    assert(gJVM != NULL);
    JNIEnv *env = NULL;
    jboolean isAttached = JNI_FALSE;

    if ((*gJVM)->GetEnv(gJVM, (void**) &env, JNI_VERSION_1_6) < 0)
    {
        if ((*gJVM)->AttachCurrentThread(gJVM, &env, NULL) < 0)
        {
            loge(tag, "AttachCurrentThread failed");
            return;
        }
        isAttached = JNI_TRUE;
    }
    assert(env != NULL);
    jstring jstring1 = NULL;
    if (obj) jstring1 = (*env)->NewStringUTF(env, obj);
    //(*env)->CallVoidMethod (env, gObj, on_event_cb_id, (jint)what, arg1, arg2, jstring1);
    jclass cls = (*env)->GetObjectClass(env, gObj);
    if (cls == NULL) {
        return;
    }
    jmethodID static_method_id = (*env)->GetStaticMethodID(env, cls, "postEventFromNative", "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (!static_method_id) {
        loge(tag, "method id is null");
        return;
    }
    (*env)->CallStaticVoidMethod(env, cls, static_method_id, gObj, what, arg1,arg2, jstring1);
    if (jstring1) (*env)->DeleteLocalRef(env, jstring1);
    if (isAttached) (*gJVM)->DetachCurrentThread(gJVM);
}
static void *queue_runnable(void *arg)
{
    pthread_detach(pthread_self());
    msg_queue_init(&msg_queue);
    msg_queue_start(&msg_queue);
    running = 1;
    //logw(tag, "start running...");
    int ret;
    while (running)
    {
        AVMessage message;
        ret = msg_queue_get(&msg_queue, &message, 1);
        if (ret <= 0)
        {
            msg_free_res(&message);
            continue;
        }
        logw(tag, "ret=%d, msg what=%d", ret, message.what);
        if (message.obj)
        {
            on_event_callback(message.what, message.arg1, message.arg2, message.obj, message.len);
        }
        else
        {
            on_event_callback(message.what, message.arg1, message.arg2, NULL, 0);
        }
        msg_free_res(&message);
    }
    logi(tag, "queue runnable over");
    pthread_exit(NULL);
}
static jboolean jni_put_msg(JNIEnv *env, jobject thiz, jint jmsg)
{
    //msg_queue_put_simple1(&msg_queue, what);
    char *c = malloc(16);
    sprintf(c, "%s %d", "message is ", jmsg);
    //loge(tag, "C===%s", c);
    msg_queue_put_simple4(&msg_queue, jmsg, 1, 2, c, 16);
    if (c) free(c);
    return JNI_TRUE;
}
static JNINativeMethod method[] = {
        {"nativeInit", "()Z", (void *) jni_init},
        {"getNativeString", "()Ljava/lang/String;", (void*) jni_get_string},
        {"putMessage", "(I)Z", (void*)jni_put_msg},
        {"destroy", "()Z", (void*)jni_destroy}
};
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    JNIEnv *env = NULL;
    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) return JNI_ERR;
    gJVM = vm;
    jclass gClass = (*env)->FindClass(env, "com/example/bob/nativemessagequeue/api/MessageClient");
    if (gClass == NULL) return JNI_ERR;

    (*env)->RegisterNatives(env, gClass, method, NELEM(method));

    return JNI_VERSION_1_6;
}