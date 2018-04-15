package com.example.bob.nativemessagequeue.api;

import android.os.Handler;
import android.os.Message;
import android.util.Log;

import java.lang.ref.WeakReference;

/**
 * Created by bob on 2018/4/8.
 */

public class MessageClient extends AbstractMQ {
    static String tag = "MessageQueue";//getClass().getSimpleName();
    static {
        System.loadLibrary("native-lib");
    }

    private native boolean nativeInit();
    private native String getNativeString();
    public native boolean putMessage(int what);
    public native boolean destroy();
    public MessageClient() {
        Log.i(tag,"init "+nativeInit());
    }

    private final Handler handler = new Handler(new Handler.Callback() {
        @Override
        public boolean handleMessage(Message msg) {
            Log.e(tag, "what="+msg.what+", arg1="+msg.arg1+",arg2="+msg.arg2+", obj="+msg.obj);
            switch (msg.what) {
                case 200:
                    notifyOnCompletion();
                    break;
                case 300:
                    notifyOnPrepared();
                    break;
            }
            return false;
        }
    });

    private static void postEventFromNative(Object weakThiz, int what, int arg1, int arg2, Object obj) {
        if (weakThiz == null) {
            Log.e(tag, "weak thiz is null");
            return;
        }
        WeakReference<MessageClient> weakReference = new WeakReference<>((MessageClient) weakThiz);
        if (weakReference.get() == null) {
            Log.e(tag, "message queue is null");
            return;
        }
        if (weakReference.get().handler != null) {
            Message m = weakReference.get().handler.obtainMessage(what, arg1, arg2, obj);
            weakReference.get().handler.sendMessage(m);
        }
    }
}
