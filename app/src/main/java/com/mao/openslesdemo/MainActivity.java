package com.mao.openslesdemo;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.view.View;

import java.io.File;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    public void playPcm(View view) {
        String path=Environment.getExternalStoragePublicDirectory("").getAbsolutePath();
        nativePlay(path+File.separator+"mydream.pcm");
    }

    public void shutdown(View view) {
        release();
    }

    public native void nativePlay(String url);

    public native void release();

    @Override
    protected void onDestroy() {
        super.onDestroy();
        release();
    }

}
