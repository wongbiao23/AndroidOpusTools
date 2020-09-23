package com.wodekouwei.androidopustools;

import android.content.Context;
import android.util.Log;

import com.wodekouwei.sdk.library.OpusUtil;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;

public class OpusMSTools {
    private static final String TAG = "OpusTools";
    public static int sampleRate = Constants.DEFAULT_AUDIO_SAMPLE_RATE;
    public static int channels = 4;
    public static int bitRate = 32000;
    public static int sampleSize = 16;
    public static int complexity = 10;
    private static int bytePerMs = sampleRate * channels * sampleSize / 8 / 1000;
    private static final int maxFrameSize = 5760;

    public static void test(Context context) {
        String source = "/sdcard/beijing.raw";
        String encode = "/sdcard/beijing.enc";
        encodeFile(context, source, encode);
//    String source = "movie.raw.enc";
//        String sourceName = "test.opus";
//        String source = path + sourceName;
//        String decode = path + sourceName + ".dec";
//        decodeFile(context, source, decode);
    }

    public static void encodeFile(Context context, String in, String out) {
        int bytePerTime = bytePerMs*20;
        Log.d(TAG, "Encode byte per time: " + bytePerTime);
        long encoder = OpusUtil._createOpusMSEncoder(sampleRate, channels, bitRate, complexity);
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
            input = new FileInputStream(new File(in));
            Log.d(TAG, "The raw file length " + input.available());
            byte[] rawData = new byte[bytePerTime];
            byte[] encData = new byte[bytePerTime];
            int length;
            while((length = input.read(rawData)) != -1) {
                Log.d(TAG, "Read " + length + ", left " + input.available());
                short[] rawShort = ArrayUtil.bytes2shorts(rawData, length);
                int encSize = OpusUtil._encodeOpusMS(encoder, rawShort, 0, encData);
                Log.d(TAG, "Raw size " + length + ", encode size " + encSize);
                if (encSize > 0) {
                    byte[] encFinal = new byte[8 + encSize];
                    /**
                     * 百度opus私有格式，每包数据前面加八个字节，前四个字节为包长，后四个字节保留
                     */
                    encFinal[0] = (byte)(encSize >> 24);
                    encFinal[1] = (byte)((encSize >> 16) & 0xFF);
                    encFinal[2] = (byte)((encSize >> 8) & 0xFF);
                    encFinal[3] = (byte)(encSize & 0xFF);
                    System.arraycopy(encData, 0, encFinal, 8, encSize);
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
            OpusUtil._destroyOpusMSEncoder(encoder);
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
