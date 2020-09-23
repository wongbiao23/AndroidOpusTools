package com.wodekouwei.androidopustools;

import android.Manifest;
import android.content.pm.PackageManager;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;

public class MainActivity extends AppCompatActivity {
  private String targetFilePath;
  private OpusRecorderTask opusRecorderTask;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    if (checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_DENIED) {
      String permissions[] = {
          Manifest.permission.RECORD_AUDIO, Manifest.permission.READ_EXTERNAL_STORAGE,
          Manifest.permission.WRITE_EXTERNAL_STORAGE
      };
      requestPermissions(permissions,
          1000);
    }
    findViewById(R.id.startRecord).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        if (opusRecorderTask != null) {
          return;
        }
//        targetFilePath = "/sdcard/" + System.currentTimeMillis() + ".opus";
        targetFilePath = "/sdcard/test.opus";
        opusRecorderTask = new OpusRecorderTask(targetFilePath);
        new Thread(opusRecorderTask).start();
      }
    });
    findViewById(R.id.stopRecord).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        if (opusRecorderTask != null) {
          opusRecorderTask.stop();
          opusRecorderTask = null;
        }
      }
    });
    findViewById(R.id.startTest).setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v) {
        OpusMSTools.test(getApplicationContext());
      }
    });
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
      @NonNull int[] grantResults) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
  }
}
