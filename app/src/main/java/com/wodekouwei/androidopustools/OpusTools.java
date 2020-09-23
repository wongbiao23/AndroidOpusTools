package com.wodekouwei.androidopustools;

import android.content.Context;
import android.util.Log;

import com.wodekouwei.sdk.library.OpusUtil;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;

public class OpusTools {
  private static final String TAG = "OpusTools";
  public static int sampleRate = Constants.DEFAULT_AUDIO_SAMPLE_RATE;
  public static int channels = Constants.DEFAULT_OPUS_CHANNEL;
  public static int bitRate = Constants.DEFAULT_OPUS_BIT_RATE;
  public static int complexity = Constants.DEFAULT_OPUS_COMPLEXITY;
  private static int bytePerMs = sampleRate * channels * bitRate / 8 / 1000;
  private static final int maxFrameSize = 5760;

  public static void test(Context context) {
    String path = "/sdcard/";
//    String source = "movie.raw";
//    String encode = path + source + ".enc";
//    encodeFile(context, source, encode);
//    String source = "movie.raw.enc";
    String sourceName = "test.opus";
    String source = path + sourceName;
    String decode = path + sourceName + ".dec";
    decodeFile(context, source, decode);
  }

  public static void encodeFile(Context context, String in, String out) {
    int bytePerTime = bytePerMs*20;
    Log.d(TAG, "Encode byte per time: " + bytePerTime);
    long encoder = OpusUtil._createOpusEncoder(sampleRate, channels, bitRate, complexity);
    File output = new File(out);
    BufferedOutputStream bufferedOutputStream = null;
    InputStream input = null;
    try {
      if (output.exists()) {
        output.delete();
      }
      if (!output.createNewFile()) {
        Log.w(TAG, "Create output file failed:" + output.getAbsolutePath());
        return;
      }
      bufferedOutputStream = new BufferedOutputStream(new FileOutputStream(output, false));
      input = context.getClassLoader().getResourceAsStream("assets/" + in);
      Log.d(TAG, "The raw file length " + input.available());
      byte[] rawData = new byte[bytePerTime];
      byte[] encData = new byte[bytePerTime];
      int length;
      while((length = input.read(rawData)) != -1) {
        Log.d(TAG, "Read " + length + ", left " + input.available());
        short[] rawShort = ArrayUtil.bytes2shorts(rawData, length);
        int encSize = OpusUtil._encodeOpus(encoder, rawShort, 0, encData);
        Log.d(TAG, "Raw size " + length + ", encode size " + encSize);
        if (encSize > 0) {
          byte[] encFinal = new byte[encSize];
          System.arraycopy(encData, 0, encFinal, 0, encSize);
          bufferedOutputStream.write(encFinal);
        }
      }
      Log.d(TAG, "Encode file finished");
    } catch(Exception e) {
      Log.d(TAG, e.getMessage());
    } finally {
      try {
        if (input != null) {
          input.close();
        }
        if (bufferedOutputStream != null) {
          bufferedOutputStream.flush();
          bufferedOutputStream.close();
        }
      } catch(Exception e) {
        Log.w(TAG, e.getMessage());
      }
      OpusUtil._destroyOpusEncoder(encoder);
    }
  }

  public static void decodeFile(Context context, String in, String out) {
    int bytePerTime = 80;
    long decoder = OpusUtil._createOpusDecoder(sampleRate, channels);
    File output = new File(out);
    BufferedOutputStream bufferedOutputStream = null;
    InputStream input = null;
    try {
      if (output.exists()) {
        output.delete();
      }
      if (!output.createNewFile()) {
        Log.w(TAG, "Create output file failed:" + output.getAbsolutePath());
        return;
      }
      bufferedOutputStream = new BufferedOutputStream(new FileOutputStream(output, false));
//      input = context.getClassLoader().getResourceAsStream("assets/" + in);
      input = new FileInputStream(new File(in));
      Log.d(TAG, "The raw file length " + input.available());
      byte[] rawData = new byte[bytePerTime];
      byte[] decData = new byte[maxFrameSize * channels * 2];
      int length;
      while((length = input.read(rawData)) != -1) {
        Log.d(TAG, "Read " + length + ", left " + input.available());
        int decSize = OpusUtil._decodeOpus(decoder, rawData, decData, maxFrameSize);
        Log.d(TAG, "Raw size " + length + ", decode size " + decSize);
        if (decSize > 0) {
          byte[] decFinal = new byte[decSize];
          System.arraycopy(decData, 0, decFinal, 0, decSize);
          bufferedOutputStream.write(decFinal);
        }
      }
    } catch(Exception e) {
      Log.d(TAG, e.getMessage());
    } finally {
      try {
        if (input != null) {
          input.close();
        }
        if (bufferedOutputStream != null) {
          bufferedOutputStream.flush();
          bufferedOutputStream.close();
        }
      } catch(Exception e) {
        Log.w(TAG, e.getMessage());
      }
      OpusUtil._destroyOpusDecoder(decoder);
    }
  }
}
