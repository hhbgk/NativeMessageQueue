package com.example.bob.nativemessagequeue;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import com.example.bob.nativemessagequeue.api.IPlayer;
import com.example.bob.nativemessagequeue.api.MessageClient;

public class MainActivity extends AppCompatActivity {
    MessageClient messageClient;
    int what = 100;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Button button = findViewById(R.id.button);
        messageClient = new MessageClient();
        messageClient.setOnCompletionListener(onCompletionListener);

        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                messageClient.putMessage(what);
                what+=100;
            }
        });
    }

    private final IPlayer.OnCompletionListener onCompletionListener = new IPlayer.OnCompletionListener() {
        @Override
        public void onCompletion() {
            Toast.makeText(MainActivity.this, "On completion", Toast.LENGTH_LONG).show();
        }
    };

    @Override
    protected void onStop() {
        super.onStop();
        messageClient.destroy();
    }
}
