package com.example.bob.nativemessagequeue.api;

/**
 * Created by bob on 2018/4/8.
 */

public interface IPlayer {
    interface OnCompletionListener {
        void onCompletion();
    }

    interface OnPreparedListener {
        void onPrepared();
    }
}
