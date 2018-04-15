package com.example.bob.nativemessagequeue.api;

/**
 * Created by bob on 2018/4/8.
 */

public abstract class AbstractMQ implements IPlayer{
    private OnCompletionListener onCompletionListener;
    private OnPreparedListener onPreparedListener;

    public final void setOnCompletionListener(OnCompletionListener listener){
        onCompletionListener = listener;
    }
    public final void setOnPreparedListener(OnPreparedListener listener){
        onPreparedListener = listener;
    }

    protected final void notifyOnCompletion(){
        if (onCompletionListener != null){
            onCompletionListener.onCompletion();
        }
    }
    protected final void notifyOnPrepared(){
        if (onPreparedListener != null){
            onPreparedListener.onPrepared();
        }
    }
}
