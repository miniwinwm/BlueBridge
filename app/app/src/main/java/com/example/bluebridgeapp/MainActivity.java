/*
MIT License

Copyright (c) John Blaiklock 2022 BlueThing

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

package com.example.bluebridgeapp;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.ToggleButton;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {
    final long maxGpsDataAge = 2000;
    final long maxDepthDataAge = 2000;
    final long maxWindDataAge = 2000;
    final long maxHeadingDataAge = 2000;
    final long maxPressureDataAge = 30000;
    final long alarmRearmTime = 30000;
    final float DEGREES_TO_RADS = 57.296f;
    SharedPreferences preferences;
    ToggleButton depthToggleButton;
    boolean depthWatching;
    ToggleButton windToggleButton;
    boolean windWatching;
    ToggleButton pressureChangeToggleButton;
    boolean pressureChangeWatching;
    ToggleButton headingChangeToggleButton;
    boolean headingChangeWatching;
    ToggleButton sogToggleButton;
    boolean sogWatching;
    ToggleButton positionChangeToggleButton;
    boolean positionChangeWatching;
    float depthMin;
    EditText depthMinEditText;
    float depthMax;
    EditText depthMaxEditText;
    float windMax;
    EditText windMaxEditText;
    float pressureChangeMax;
    EditText pressureChangeMaxEditText;
    float headingChangeMax;
    EditText headingChangeMaxEditText;
    float sogMax;
    EditText sogMaxEditText;
    float positionChangeMax;
    EditText positionChangeMaxEditText;
    boolean keyboardListenersAttached = false;
    ViewGroup rootLayout;
    Button connectButton;
    Button watchingButton;
    volatile boolean isBluetoothConnected = false;
    volatile boolean isWatching = false;
    MediaPlayer mediaPlayer;
    BluetoothAdapter bluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    BluetoothSocket bluetoothSocket;
    static final UUID MY_UUID_SECURE = UUID.fromString(("00001101-0000-1000-8000-00805F9B34FB"));
    InputStream bluetoothInputStream;
    boolean nmeaMessageStarted = false;
    TextView textViewDepth;
    TextView textViewPressure;
    TextView textViewHeading;
    TextView textViewWindspeed;
    TextView textViewSog;
    TextView textViewPositionChange;
    TextView textViewPressureChange;
    TextView textViewHeadingChange;
    float depth;
    long depthReceivedTime = 0L;
    float pressure;
    long pressureReceivedTime = 0L;
    float heading;
    long headingReceivedTime = 0L;
    float windspeed;
    long windspeedReceivedTime = 0L;
    float sog;
    long sogReceivedTime = 0L;
    float latitude;
    long latitudeReceivedTime = 0L;
    float longitude;
    long longitudeReceivedTime = 0L;
    float startPressure;
    float startHeading;
    float startLatitude;
    float startLongitude;
    float headingChange;
    float pressureChange;
    float positionChange;
    volatile boolean connectionCloseRequest = false;
    AnchorView anchorView;
    long lastAnchorPlotUpdateTime;
    AlertDialog alert;
    long lastAlarmTime = 0;
    long lastPingTime = 0;
    boolean dataLossAlarmActive = false;
    boolean watchingAlarmActive = false;

    Thread thread = new Thread() {
        byte[] nmeaMessageArray = new byte[101];
        public void run() {
            byte[] bluetoothBytes = new byte[20];
            int nextBytePosition = 0;
            long lastUpdateTime = 0;

            while (true) {
                if (isBluetoothConnected) {
                    try {
                        int bytesRead = bluetoothInputStream.read(bluetoothBytes, 0, 20);
                        for (int i = 0; i < bytesRead; i++) {
                            byte nextByte = bluetoothBytes[i];
                            if (!nmeaMessageStarted) {
                                if (nextByte == '!' || nextByte == '$') {
                                    nmeaMessageStarted = true;
                                    nmeaMessageArray[0] = nextByte;
                                    nextBytePosition = 1;
                                }
                            }
                            else {
                                if (nextBytePosition < 100) {
                                    nmeaMessageArray[nextBytePosition] = nextByte;
                                    nextBytePosition++;
                                    if (nextByte == '\n') {
                                        nmeaMessageStarted = false;

                                        String nmeaMessageString = new String(nmeaMessageArray, StandardCharsets.US_ASCII).substring(0, nextBytePosition - 1);
                                        Log.d("nmea", nmeaMessageString);

                                        List<String> fieldList = Arrays.asList(nmeaMessageString.split(","));
                                        if (fieldList.size() > 0) {
                                            String nmeaHeader = fieldList.get(0).substring(3, 6);

                                            if (nmeaHeader.equals("DPT")) {
                                                // depth DPT
                                                if (fieldList.size() >= 2) {
                                                    String fieldString = fieldList.get(1);
                                                    try {
                                                        depth = Float.parseFloat(fieldString);
                                                        depthReceivedTime = System.currentTimeMillis();
                                                        runOnUiThread(new Runnable() {
                                                            public void run() {
                                                                textViewDepth.setText(fieldString + " m");
                                                            }
                                                        });
                                                    }
                                                    catch (Exception e) {
                                                    }
                                                }
                                            } else if (nmeaHeader.equals("XDR")) {
                                                // pressure XDR
                                                if (fieldList.size() >= 3) {
                                                    String fieldString = fieldList.get(2);
                                                    try {
                                                        pressure = Float.parseFloat(fieldString);
                                                        pressure *= 1000.0f;
                                                        pressureReceivedTime = System.currentTimeMillis();
                                                        runOnUiThread(new Runnable() {
                                                            public void run() {
                                                                textViewPressure.setText(Integer.toString(Math.round(pressure)) + " mb");
                                                            }
                                                        });
                                                    }
                                                    catch (Exception e) {
                                                    }
                                                }
                                            }
                                            else if (nmeaHeader.equals("HDT")) {
                                                // heading HDT
                                                if (fieldList.size() >= 2) {
                                                    String fieldString = fieldList.get(1);
                                                    try {
                                                        heading = Float.parseFloat(fieldString);
                                                        headingReceivedTime = System.currentTimeMillis();
                                                        runOnUiThread(new Runnable() {
                                                            public void run() {
                                                                textViewHeading.setText(Integer.toString(Math.round(heading)) + "°");
                                                            }
                                                        });
                                                    }
                                                    catch (Exception e) {
                                                    }
                                                }
                                            }
                                            else if (nmeaHeader.equals("MWV")) {
                                                // wind MWV
                                                if (fieldList.size() >= 4) {
                                                    String fieldString = fieldList.get(3);
                                                    try {
                                                        windspeed = Float.parseFloat(fieldString);
                                                        windspeedReceivedTime = System.currentTimeMillis();
                                                        runOnUiThread(new Runnable() {
                                                            public void run() {
                                                                textViewWindspeed.setText(Integer.toString(Math.round(windspeed)) + " Kts");
                                                            }
                                                        });
                                                    }
                                                    catch (Exception e) {
                                                    }
                                                }
                                            }
                                            else if (nmeaHeader.equals("RMC")) {
                                                // GPS RMC
                                                if (fieldList.size() >= 8) {
                                                    String fieldString1 = fieldList.get(2);
                                                    if (fieldString1.equals("A")) {
                                                        String fieldString2 = fieldList.get(7);
                                                        try {
                                                            sog = Float.parseFloat(fieldString2);
                                                            sogReceivedTime = System.currentTimeMillis();
                                                            runOnUiThread(new Runnable() {
                                                                public void run() {
                                                                    textViewSog.setText(fieldString2 + " Kts");
                                                                }
                                                            });
                                                        }
                                                        catch (Exception e) {
                                                        }

                                                        String fieldString3 = fieldList.get(3);
                                                        boolean latitudeParsedOk = false;
                                                        boolean latitudeHemisphereParsedOk = false;
                                                        try {
                                                            float latitudeNmea = Float.parseFloat(fieldString3);
                                                            latitudeNmea /= 100.0f;
                                                            latitude = (float)Math.floor((float)latitudeNmea);
                                                            latitudeNmea -= latitude;
                                                            latitudeNmea /= 0.6f;
                                                            latitude += latitudeNmea;
                                                            latitudeParsedOk = true;
                                                        }
                                                        catch (Exception e) {
                                                        }

                                                        String fieldString4 = fieldList.get(4);
                                                        if (fieldString4.equals("N")) {
                                                            latitudeHemisphereParsedOk = true;
                                                        } else if (fieldString4.equals("S")) {
                                                            latitude = -latitude;
                                                            latitudeHemisphereParsedOk = true;
                                                        }

                                                        if (latitudeParsedOk && latitudeHemisphereParsedOk) {
                                                            latitudeReceivedTime = System.currentTimeMillis();
                                                        }

                                                        String fieldString5 = fieldList.get(5);
                                                        boolean longitudeParsedOk = false;
                                                        boolean longitudeHemisphereParsedOk = false;

                                                        try {
                                                            float longitudeNmea = Float.parseFloat(fieldString5);
                                                            longitudeNmea /= 100.0f;
                                                            longitude = (float)Math.floor((float)longitudeNmea);
                                                            longitudeNmea -= longitude;
                                                            longitudeNmea /= 0.6f;
                                                            longitude += longitudeNmea;
                                                            longitudeParsedOk = true;
                                                        }
                                                        catch (Exception e) {
                                                        }

                                                        String fieldString6 = fieldList.get(6);
                                                        if (fieldString6.equals("E")) {
                                                            longitudeHemisphereParsedOk = true;
                                                        } else if (fieldString6.equals("W")) {
                                                            longitude = -longitude;
                                                            longitudeHemisphereParsedOk = true;
                                                        }

                                                        if (longitudeParsedOk && longitudeHemisphereParsedOk) {
                                                            longitudeReceivedTime = System.currentTimeMillis();
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                else {
                                    nmeaMessageStarted = false;
                                }
                            }
                        }
                    }
                    catch (IOException e) {
                        if (connectionCloseRequest) {
                            connectionCloseRequest = false;
                        }
                        else {
                            isBluetoothConnected = false;

                            if (isWatching) {
                                isWatching = false;
                                runOnUiThread(new Runnable() {
                                    public void run() {
                                        watchingButton.setBackgroundColor(Color.GRAY);
                                        watchingButton.setEnabled(false);
                                        watchingButton.setText("START");
                                    }
                                });
                                // sound alarm for loss of data
                                mediaPlayer.release();
                                mediaPlayer = MediaPlayer.create(getApplicationContext(), R.raw.beeper);
                                mediaPlayer.setVolume(1.0f, 1.0f);
                                mediaPlayer.setLooping(true);
                                mediaPlayer.start();
                            }

                            runOnUiThread(new Runnable() {
                                public void run() {
                                    messageBoxThread("Bluetooth connection lost.");
                                    connectButton.setBackgroundColor(Color.GREEN);
                                    connectButton.setText("CONNECT");
                                    connectButton.setEnabled(true);
                                    setupWatchingButtons(true);
                                    resetAllCurrentReadingsTexts();
                                }
                            });
                        }
                    }
                }

                if (System.currentTimeMillis() - lastUpdateTime > 1000 && isWatching) {
                    lastUpdateTime = System.currentTimeMillis();

                    runOnUiThread(new Runnable() {
                        public void run() {
                            float headingChange;
                            if (System.currentTimeMillis() - headingReceivedTime < maxHeadingDataAge) {
                                headingChange = heading - startHeading;
                                if (headingChange > 180.0f) {
                                    headingChange -= 180.0f;
                                }
                                if (headingChange < -180.0f) {
                                    headingChange += 180.0f;
                                }
                                if (headingChangeWatching) {
                                    textViewHeadingChange.setText(Integer.toString(Math.round(headingChange)) + "°");
                                }
                            } else {
                                textViewHeadingChange.setText("----");
                            }

                            float pressureChange;
                            if (System.currentTimeMillis() - pressureReceivedTime < maxPressureDataAge) {
                                pressureChange = pressure - startPressure;
                                if (pressureChangeWatching) {
                                    textViewPressureChange.setText(Integer.toString(Math.round(pressureChange)) + " mb");
                                }
                            } else {
                                textViewPressureChange.setText("----");
                            }

                            float positionChange;
                            if (System.currentTimeMillis() - latitudeReceivedTime < maxGpsDataAge && System.currentTimeMillis() - longitudeReceivedTime < maxGpsDataAge) {
                                positionChange = distance_between_points(latitude, longitude, startLatitude, startLongitude);

                                if (System.currentTimeMillis() - lastAnchorPlotUpdateTime > 5000 && isWatching) {
                                    lastAnchorPlotUpdateTime = System.currentTimeMillis();
                                    float startLatitudeRads = startLatitude / DEGREES_TO_RADS;
                                    float startLongitudeRads = startLongitude / DEGREES_TO_RADS;
                                    float latitudeRads = latitude / DEGREES_TO_RADS;
                                    float longitudeRads = longitude / DEGREES_TO_RADS;

                                    float positionChangeDistance_x = (float) ((longitudeRads - startLongitudeRads) * Math.cos((startLatitudeRads + latitudeRads) / 2)) * 6371000.0f;
                                    float positionChangeDistance_y = (latitudeRads - startLatitudeRads) * 6371000.0f;
                                    anchorView.AddPosDiffMetres(positionChangeDistance_x, positionChangeDistance_y);
                                    anchorView.drawAnchorView();
                                }

                                if (positionChangeWatching) {
                                    textViewPositionChange.setText(Integer.toString(Math.round(positionChange)) + " m");
                                }
                            } else {
                                textViewPositionChange.setText("----");
                            }
                        }
                    });
                }

                // periodic ping sound
                if (isWatching && System.currentTimeMillis() - lastPingTime > 10000 && !dataLossAlarmActive && !watchingAlarmActive) {
                    mediaPlayer.release();
                    mediaPlayer = MediaPlayer.create(getApplicationContext(), R.raw.ping);
                    mediaPlayer.setVolume(0.2f, 0.2f);
                    mediaPlayer.setLooping(false);
                    mediaPlayer.start();
                    lastPingTime = System.currentTimeMillis();
                }

                // do data available checking
                if (System.currentTimeMillis() - lastAlarmTime > alarmRearmTime && isWatching && !watchingAlarmActive) {
                    if (depthWatching) {
                        if (System.currentTimeMillis() - depthReceivedTime > maxDepthDataAge)
                        {
                            if (!alert.isShowing()) {
                                messageBoxThread("No depth data received");
                                dataLossAlarmActive = true;
                            }
                        }
                        else {
                            if (alert.isShowing()) {
                                mediaPlayer.stop();
                                alert.cancel();
                                dataLossAlarmActive = false;
                            }
                        }
                    }

                    if (windWatching) {
                        if (System.currentTimeMillis() - windspeedReceivedTime > maxWindDataAge)
                        {
                            if (!alert.isShowing()) {
                                messageBoxThread("No windspeed data received");
                                dataLossAlarmActive = true;
                            }
                        }
                        else {
                            if (alert.isShowing()) {
                                mediaPlayer.stop();
                                alert.cancel();
                                dataLossAlarmActive = false;
                            }
                        }
                    }

                    if (headingChangeWatching) {
                        if (System.currentTimeMillis() - headingReceivedTime > maxHeadingDataAge)
                        {
                            if (!alert.isShowing()) {
                                messageBoxThread("No heading data received");
                                dataLossAlarmActive = true;
                            }
                        }
                        else {
                            if (alert.isShowing()) {
                                mediaPlayer.stop();
                                alert.cancel();
                                dataLossAlarmActive = false;
                            }
                        }
                    }

                    if (pressureChangeWatching) {
                        if (System.currentTimeMillis() - pressureReceivedTime > maxPressureDataAge)
                        {
                            if (!alert.isShowing()) {
                                messageBoxThread("No pressure data received");
                                dataLossAlarmActive = true;
                            }
                        }
                        else {
                            if (alert.isShowing()) {
                                mediaPlayer.stop();
                                alert.cancel();
                                dataLossAlarmActive = false;
                            }
                        }
                    }

                    if (sogWatching) {
                        if (System.currentTimeMillis() - sogReceivedTime > maxGpsDataAge)
                        {
                            if (!alert.isShowing()) {
                                messageBoxThread("No SOG data received");
                                dataLossAlarmActive = true;
                            }
                        }
                        else {
                            if (alert.isShowing()) {
                                mediaPlayer.stop();
                                alert.cancel();
                                dataLossAlarmActive = false;
                            }
                        }
                    }

                    if (positionChangeWatching) {
                        if (System.currentTimeMillis() - latitudeReceivedTime > maxGpsDataAge || System.currentTimeMillis() - longitudeReceivedTime > maxGpsDataAge)
                        {
                            if (!alert.isShowing()) {
                                messageBoxThread("No position data received");
                                dataLossAlarmActive = true;
                            }
                        }
                        else {
                            if (alert.isShowing()) {
                                mediaPlayer.stop();
                                alert.cancel();
                                dataLossAlarmActive = false;
                            }
                        }
                    }
                    if (dataLossAlarmActive && !alert.isShowing()) {
                        mediaPlayer.release();
                        mediaPlayer = MediaPlayer.create(getApplicationContext(), R.raw.beeper);
                        mediaPlayer.setVolume(1.0f, 1.0f);
                        mediaPlayer.setLooping(true);
                        mediaPlayer.start();
                        lastAlarmTime = System.currentTimeMillis();
                    }
                }

                // do watching
                if (System.currentTimeMillis() - lastAlarmTime > alarmRearmTime && isWatching && !dataLossAlarmActive && !watchingAlarmActive ) {
                    if (depthWatching) {
                        if (depth < depthMin || depth > depthMax) {
                            lastAlarmTime = System.currentTimeMillis();
                            if (!alert.isShowing()) {
                                messageBoxThread("Depth alarm");
                                watchingAlarmActive = true;
                            }
                        }
                    }

                    if (windWatching) {
                        if (windspeed > windMax) {
                            lastAlarmTime = System.currentTimeMillis();
                            if (!alert.isShowing()) {
                                messageBoxThread("Wind alarm");
                                watchingAlarmActive = true;
                            }
                        }
                    }

                    if (headingChangeWatching) {
                        if (Math.abs(headingChange) > headingChangeMax) {
                            lastAlarmTime = System.currentTimeMillis();
                            if (!alert.isShowing()) {
                                messageBoxThread("Heading change alarm");
                                watchingAlarmActive = true;
                            }
                        }
                    }

                    if (pressureChangeWatching) {
                        if (Math.abs(pressureChange) > pressureChangeMax) {
                            lastAlarmTime = System.currentTimeMillis();
                            if (!alert.isShowing()) {
                                messageBoxThread("Pressure change alarm");
                                watchingAlarmActive = true;
                            }
                        }
                    }

                    if (sogWatching) {
                        if (sog > sogMax) {
                            lastAlarmTime = System.currentTimeMillis();
                            if (!alert.isShowing()) {
                                messageBoxThread("SOG alarm");
                                watchingAlarmActive = true;
                            }
                        }
                    }

                    if (positionChangeWatching) {
                        if (positionChange > positionChangeMax) {
                            lastAlarmTime = System.currentTimeMillis();
                            if (!alert.isShowing()) {
                                messageBoxThread("Position change alarm");
                                watchingAlarmActive = true;
                            }
                        }
                    }

                    if (watchingAlarmActive && !alert.isShowing()) {
                        mediaPlayer.release();
                        mediaPlayer = MediaPlayer.create(getApplicationContext(), R.raw.beeper);
                        mediaPlayer.setVolume(1.0f, 1.0f);
                        mediaPlayer.setLooping(true);
                        mediaPlayer.start();
                        lastAlarmTime = System.currentTimeMillis();
                    }
                }

                android.os.SystemClock.sleep(10);
            }
        }
    };

    private void messageBoxThread(String message) {
        runOnUiThread(new Runnable() {
                          public void run() {
                              alert.setMessage(message);
                              alert.show();
                          }
        });
    }

    private void messageBox(String message) {
        alert.setMessage(message);
        alert.show();
    }

    protected void onStop() {
        saveAllTextEdits();
        super.onStop();
    }

    public static void hideSoftKeyboard(Activity activity) {
        InputMethodManager inputMethodManager = (InputMethodManager) activity.getSystemService(Activity.INPUT_METHOD_SERVICE);
        if (activity.getCurrentFocus() != null) {
            if (inputMethodManager.isAcceptingText()) {
                inputMethodManager.hideSoftInputFromWindow(activity.getCurrentFocus().getWindowToken(),0);
            }
        }
    }

    private void saveAllTextEdits() {
        saveDepthMin();
        saveDepthMax();
        saveWindMax();
        savePressureChangeMax();
        saveHeadingChangeMax();
        saveSogMax();
        savePositionChangeMax();
        hideSoftKeyboard(this);
        depthMinEditText.clearFocus();
        depthMaxEditText.clearFocus();
        windMaxEditText.clearFocus();
        headingChangeMaxEditText.clearFocus();
        pressureChangeMaxEditText.clearFocus();
        sogMaxEditText.clearFocus();
        positionChangeMaxEditText.clearFocus();
    }

    private void saveDepthMin() {
        if (depthMinEditText.getText().toString().equals("")) {
            depthMinEditText.setText(String.valueOf(depthMin));
        }
        else {
            depthMin = Float.parseFloat(depthMinEditText.getText().toString());
            SharedPreferences.Editor editor = preferences.edit();
            editor.putFloat("depthMin", depthMin);
            editor.commit();
        }
    }

    private void saveDepthMax() {
        if (depthMaxEditText.getText().toString().equals("")) {
            depthMaxEditText.setText(String.valueOf(depthMax));
        }
        else {
            depthMax = Float.parseFloat(depthMaxEditText.getText().toString());
            SharedPreferences.Editor editor = preferences.edit();
            editor.putFloat("depthMax", depthMax);
            editor.commit();
        }
    }

    private void saveWindMax() {
        if (windMaxEditText.getText().toString().equals("")) {
            windMaxEditText.setText(String.valueOf(windMax));
        }
        else {
            windMax = Float.parseFloat(windMaxEditText.getText().toString());
            SharedPreferences.Editor editor = preferences.edit();
            editor.putFloat("windMax", windMax);
            editor.commit();
        }
    }

    private void saveHeadingChangeMax() {
        if (headingChangeMaxEditText.getText().toString().equals("")) {
            headingChangeMaxEditText.setText(String.valueOf(headingChangeMax));
        }
        else {
            headingChangeMax = Float.parseFloat(headingChangeMaxEditText.getText().toString());
            SharedPreferences.Editor editor = preferences.edit();
            editor.putFloat("headingChangeMax", headingChangeMax);
            editor.commit();
        }
    }

    private void savePressureChangeMax() {
        if (pressureChangeMaxEditText.getText().toString().equals("")) {
            pressureChangeMaxEditText.setText(String.valueOf(pressureChangeMax));
        }
        else {
            pressureChangeMax = Float.parseFloat(pressureChangeMaxEditText.getText().toString());
            SharedPreferences.Editor editor = preferences.edit();
            editor.putFloat("pressureChangeMax", pressureChangeMax);
            editor.commit();
        }
    }

    private void saveSogMax() {
        if (sogMaxEditText.getText().toString().equals("")) {
            sogMaxEditText.setText(String.valueOf(sogMax));
        }
        else {
            sogMax = Float.parseFloat(sogMaxEditText.getText().toString());
            SharedPreferences.Editor editor = preferences.edit();
            editor.putFloat("sogMax", sogMax);
            editor.commit();
        }
    }

    private void savePositionChangeMax() {
        if (positionChangeMaxEditText.getText().toString().equals("")) {
            positionChangeMaxEditText.setText(String.valueOf(positionChangeMax));
        }
        else {
            positionChangeMax = Float.parseFloat(positionChangeMaxEditText.getText().toString());
            SharedPreferences.Editor editor = preferences.edit();
            editor.putFloat("positionChangeMax", positionChangeMax);
            editor.commit();
        }
    }

    private void getUiIds() {
        connectButton = (Button)findViewById((R.id.connectButton));
        watchingButton = (Button)findViewById((R.id.watchingButton));
        textViewDepth = (TextView)findViewById(R.id.textViewDepth);
        textViewPressure = (TextView)findViewById(R.id.textViewPressure);
        textViewHeading  = (TextView)findViewById(R.id.textViewHeading);
        textViewWindspeed  = (TextView)findViewById(R.id.textViewWindspeed);
        textViewSog  = (TextView)findViewById(R.id.textViewSog);
        textViewPositionChange  = (TextView)findViewById(R.id.textViewPositionChange);
        textViewHeadingChange  = (TextView)findViewById(R.id.textViewHeadingChange);
        textViewPressureChange  = (TextView)findViewById(R.id.textViewPressureChange);
        depthToggleButton = (ToggleButton)findViewById((R.id.depthToggleButton));
        windToggleButton = (ToggleButton)findViewById((R.id.windToggleButton));
        pressureChangeToggleButton = (ToggleButton)findViewById((R.id.pressureChangeToggleButton));
        headingChangeToggleButton = (ToggleButton)findViewById((R.id.headingChangeToggleButton));
        sogToggleButton = (ToggleButton)findViewById((R.id.sogToggleButton));
        positionChangeToggleButton = (ToggleButton)findViewById((R.id.positionChangeToggleButton));
        depthMinEditText = (EditText)findViewById((R.id.editTextNumberDepthMin));
        depthMaxEditText = (EditText)findViewById((R.id.editTextNumberDepthMax));
        windMaxEditText = (EditText)findViewById((R.id.editTextNumberWindMax));
        pressureChangeMaxEditText = (EditText)findViewById((R.id.editTextNumberPressureChangeMax));
        headingChangeMaxEditText = (EditText)findViewById((R.id.editTextNumberHeadingChangeMax));
        sogMaxEditText = (EditText)findViewById((R.id.editTextNumberSogMax));
        positionChangeMaxEditText = (EditText)findViewById((R.id.editTextNumberPositionChangeMax));
        anchorView = (AnchorView) findViewById((R.id.anchor_view));
    }

    private void getPreferences() {
        depthWatching = preferences.getBoolean("depthWatching", true);
        windWatching = preferences.getBoolean("windWatching", true);
        pressureChangeWatching = preferences.getBoolean("pressureChangeWatching", true);
        headingChangeWatching = preferences.getBoolean("headingChangeWatching", true);
        sogWatching = preferences.getBoolean("sogWatching", true);
        positionChangeWatching = preferences.getBoolean("positionChangeWatching", true);
        depthMin = preferences.getFloat("depthMin", 2.0f);
        depthMax = preferences.getFloat("depthMax", 5.0f);
        windMax = preferences.getFloat("windMax", 20.0f);
        pressureChangeMax = preferences.getFloat("pressureChangeMax", 12.0f);
        headingChangeMax = preferences.getFloat("headingChangeMax", 40.0f);
        sogMax = preferences.getFloat("sogMax", 2.0f);
        positionChangeMax = preferences.getFloat("positionChangeMax", 50.0f);
    }

    void setupWatchingParametersTextEdits(boolean enabled) {
        depthMinEditText.setEnabled(enabled);
        depthMaxEditText.setEnabled(enabled);
        windMaxEditText.setEnabled(enabled);
        headingChangeMaxEditText.setEnabled(enabled);
        pressureChangeMaxEditText.setEnabled(enabled);
        sogMaxEditText.setEnabled(enabled);
        positionChangeMaxEditText.setEnabled(enabled);
    }

    void resetAllCurrentReadingsTexts() {
        textViewDepth.setText("----");
        textViewWindspeed.setText("----");
        textViewPressure.setText("----");
        textViewPressureChange.setText("----");
        textViewHeading.setText("----");
        textViewHeadingChange.setText("----");
        textViewSog.setText("----");
        textViewPositionChange.setText("----");
    }

    void setupWatchingButtons(boolean enabled) {
        if (enabled) {
            depthToggleButton.setChecked(depthWatching);
            if (depthWatching) {
                depthToggleButton.setBackgroundColor(Color.GREEN);
            } else {
                depthToggleButton.setBackgroundColor(Color.RED);
            }

            windToggleButton.setChecked(windWatching);
            if (windWatching) {
                windToggleButton.setBackgroundColor(Color.GREEN);
            } else {
                windToggleButton.setBackgroundColor(Color.RED);
            }

            pressureChangeToggleButton.setChecked(pressureChangeWatching);
            if (pressureChangeWatching) {
                pressureChangeToggleButton.setBackgroundColor(Color.GREEN);
            } else {
                pressureChangeToggleButton.setBackgroundColor(Color.RED);
            }

            headingChangeToggleButton.setChecked(headingChangeWatching);
            if (headingChangeWatching) {
                headingChangeToggleButton.setBackgroundColor(Color.GREEN);
            } else {
                headingChangeToggleButton.setBackgroundColor(Color.RED);
            }

            sogToggleButton.setChecked(sogWatching);
            if (sogWatching) {
                sogToggleButton.setBackgroundColor(Color.GREEN);
            } else {
                sogToggleButton.setBackgroundColor(Color.RED);
            }

            positionChangeToggleButton.setChecked(positionChangeWatching);
            if (positionChangeWatching) {
                positionChangeToggleButton.setBackgroundColor(Color.GREEN);
            } else {
                positionChangeToggleButton.setBackgroundColor(Color.RED);
            }
        } else {
            depthToggleButton.setBackgroundColor(Color.GRAY);
            windToggleButton.setBackgroundColor(Color.GRAY);
            pressureChangeToggleButton.setBackgroundColor(Color.GRAY);
            headingChangeToggleButton.setBackgroundColor(Color.GRAY);
            sogToggleButton.setBackgroundColor(Color.GRAY);
            positionChangeToggleButton.setBackgroundColor(Color.GRAY);
        }

        depthToggleButton.setEnabled(enabled);
        windToggleButton.setEnabled(enabled);
        pressureChangeToggleButton.setEnabled(enabled);
        headingChangeToggleButton.setEnabled(enabled);
        sogToggleButton.setEnabled(enabled);
        positionChangeToggleButton.setEnabled(enabled);

        depthToggleButton.setTextColor(Color.BLACK);
        windToggleButton.setTextColor(Color.BLACK);
        pressureChangeToggleButton.setTextColor(Color.BLACK);
        headingChangeToggleButton.setTextColor(Color.BLACK);
        sogToggleButton.setTextColor(Color.BLACK);
        positionChangeToggleButton.setTextColor(Color.BLACK);
    }

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getUiIds();
        connectButton.setBackgroundColor(Color.GREEN);
        connectButton.setTextColor(Color.BLACK);
        watchingButton.setBackgroundColor(Color.GRAY);
        watchingButton.setTextColor(Color.BLACK);
        mediaPlayer = MediaPlayer.create(getApplicationContext(), R.raw.beeper);
        mediaPlayer.setVolume(1.0f, 1.0f);
        mediaPlayer.start();

        preferences = getApplicationContext().getSharedPreferences(("MyPreferences"), MODE_PRIVATE);
        getPreferences();
        setupWatchingButtons(true);

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setCancelable(false);
        builder.setPositiveButton(
                "OK",
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        mediaPlayer.stop();
                        dialog.cancel();
                        watchingAlarmActive = false;
                        dataLossAlarmActive = false;
                    }
                });
        alert = builder.create();
        alert.setCanceledOnTouchOutside(false);

        depthMinEditText.setText(String.valueOf(depthMin));
        depthMinEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    saveDepthMin();
                }
            }
        });

        depthMaxEditText.setText(String.valueOf(depthMax));
        depthMaxEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    saveDepthMax();
                }
            }
        });

        windMaxEditText.setText(String.valueOf(windMax));
        windMaxEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    saveWindMax();
                }
            }
        });

        pressureChangeMaxEditText.setText(String.valueOf(pressureChangeMax));
        pressureChangeMaxEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    savePressureChangeMax();
                }
            }
        });

        headingChangeMaxEditText.setText(String.valueOf(headingChangeMax));
        headingChangeMaxEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    saveHeadingChangeMax();
                }
            }
        });

        sogMaxEditText.setText(String.valueOf(sogMax));
        sogMaxEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    saveSogMax();
                }
            }
        });

        positionChangeMaxEditText.setText(String.valueOf(positionChangeMax));
        positionChangeMaxEditText.setOnFocusChangeListener(new View.OnFocusChangeListener() {
            public void onFocusChange(View v, boolean hasFocus) {
                if (!hasFocus) {
                    savePositionChangeMax();
                }
            }
        });

        thread.start();
    }

    public void depthToggleButtonOnClick(View view) {
        depthWatching = depthToggleButton.isChecked();
        SharedPreferences.Editor editor = preferences.edit();
        editor.putBoolean("depthWatching", depthWatching);
        editor.commit();
        setupWatchingButtons(true);
    }

    public void windToggleButtonOnClick(View view) {
        windWatching = windToggleButton.isChecked();
        SharedPreferences.Editor editor = preferences.edit();
        editor.putBoolean("windWatching", windWatching);
        editor.commit();
        setupWatchingButtons(true);
    }

    public void pressureToggleButtonOnClick(View view) {
        pressureChangeWatching = pressureChangeToggleButton.isChecked();
        SharedPreferences.Editor editor = preferences.edit();
        editor.putBoolean("pressureChangeWatching", pressureChangeWatching);
        editor.commit();
        setupWatchingButtons(true);
    }

    public void headingToggleButtonOnClick(View view) {
        headingChangeWatching = headingChangeToggleButton.isChecked();
        SharedPreferences.Editor editor = preferences.edit();
        editor.putBoolean("headingChangeWatching", headingChangeWatching);
        editor.commit();
        setupWatchingButtons(true);
    }

    public void sogToggleButtonOnClick(View view) {
        sogWatching = sogToggleButton.isChecked();
        SharedPreferences.Editor editor = preferences.edit();
        editor.putBoolean("sogWatching", sogWatching);
        editor.commit();
        setupWatchingButtons(true);
    }

    public void positionToggleButtonOnClick(View view) {
        positionChangeWatching = positionChangeToggleButton.isChecked();
        SharedPreferences.Editor editor = preferences.edit();
        editor.putBoolean("positionChangeWatching", positionChangeWatching);
        editor.commit();
        setupWatchingButtons(true);
    }

    public void connectButtonOnClick(View view) {
        saveAllTextEdits();
        if (isBluetoothConnected) {
            // disconnect
            connectionCloseRequest = true;
            isBluetoothConnected = false;
            try {
                bluetoothInputStream.close();
            }
            catch (IOException e) {
            }
            try {
                bluetoothSocket.close();
            }
            catch (IOException e) {
            }
            connectButton.setBackgroundColor(Color.GREEN);
            watchingButton.setBackgroundColor(Color.GRAY);
            watchingButton.setEnabled(false);
            watchingButton.setText("START");
            connectButton.setText("CONNECT");
            isWatching = false;
            resetAllCurrentReadingsTexts();
        }
        else {
            // connect
            if (bluetoothAdapter == null) {
                messageBox("Bluetooth is not supported");
            } else if (!bluetoothAdapter.isEnabled()) {
                messageBox("Bluetooth is not enabled");
            } else {
                connectButton.setEnabled(false);
                connectButton.setText("CONNECTING");
                connectButton.setBackgroundColor(Color.GRAY);

                Thread connectThread = new Thread(new Runnable() {
                    public void run() {
                        Set<BluetoothDevice> pairedDevices = bluetoothAdapter.getBondedDevices();
                        boolean deviceFound = false;
                        if (pairedDevices.size() > 0) {
                            for (BluetoothDevice device : pairedDevices) {
                                String devicename = device.getName();
                                if (devicename.equals("BlueBridge"))
                                {
                                    deviceFound = true;
                                    try {
                                        bluetoothSocket = device.createRfcommSocketToServiceRecord(MY_UUID_SECURE);
                                        bluetoothSocket.connect();
                                        if (bluetoothSocket.isConnected())
                                        {
                                            bluetoothInputStream = bluetoothSocket.getInputStream();
                                            isBluetoothConnected = true;
                                        }
                                        else {
                                            messageBoxThread("Could not connect to BlueBridge");
                                        }
                                    }
                                    catch(IOException e) {
                                        messageBoxThread("Connection error");
                                    }
                                    break;
                                }
                            }
                        }

                        final boolean deviceFoundFinal = deviceFound;
                        runOnUiThread(new Runnable() {
                            public void run() {
                                if (!deviceFoundFinal) {
                                    messageBox("BlueBridge not found");
                                }

                                connectButton.setEnabled(true);
                                if (isBluetoothConnected) {
                                    connectButton.setBackgroundColor(Color.RED);
                                    connectButton.setText("DISCONNECT");
                                    watchingButton.setBackgroundColor(Color.GREEN);
                                    watchingButton.setEnabled(true);
                                    nmeaMessageStarted = false;
                                }
                                else {
                                    connectButton.setBackgroundColor(Color.GREEN);
                                    connectButton.setText("CONNECT");
                                }
                            }
                        });
                    }
                });
                connectThread.start();
            }
        }
    }

    public void watchingButtonOnClick(View view) {
        saveAllTextEdits();
        anchorView.Reset();
        anchorView.drawAnchorView();
        WindowManager.LayoutParams params = getWindow().getAttributes();
        params.screenBrightness = 0;
        getWindow().setAttributes(params);

        if (isWatching) {
            watchingButton.setBackgroundColor(Color.GREEN);
            watchingButton.setText("START");
            isWatching = false;
            setupWatchingButtons(true);
            setupWatchingParametersTextEdits(true);
            connectButton.setEnabled(true);
            connectButton.setBackgroundColor(Color.RED);
        }
        else {
            if (depthMin >= depthMax) {
                messageBox("Minimum depth must be less than maximum depth.");
                return;
            }
            if (!depthWatching && !headingChangeWatching && !windWatching && !pressureChangeWatching && !sogWatching && !positionChangeWatching) {
                messageBox("No measurements chosen to watch.");
                return;
            }
            if ((!depthWatching || System.currentTimeMillis() - depthReceivedTime < maxDepthDataAge) &&
                    (!windWatching || System.currentTimeMillis() - windspeedReceivedTime < maxWindDataAge) &&
                    (!headingChangeWatching || System.currentTimeMillis() - headingReceivedTime < maxHeadingDataAge) &&
                    (!pressureChangeWatching || System.currentTimeMillis() - pressureReceivedTime < maxPressureDataAge) &&
                    (!sogWatching || System.currentTimeMillis() - sogReceivedTime < maxGpsDataAge) &&
                    (!positionChangeWatching || (System.currentTimeMillis() - latitudeReceivedTime < maxGpsDataAge && System.currentTimeMillis() - longitudeReceivedTime < maxGpsDataAge))) {
                if (headingChangeWatching) {
                    startHeading = heading;
                }
                if (pressureChangeWatching) {
                    startPressure = pressure;
                }
                if (positionChangeWatching) {
                    startLatitude = latitude;
                    startLongitude = longitude;
                }
                watchingButton.setBackgroundColor(Color.RED);
                watchingButton.setText("STOP");
                isWatching = true;

                setupWatchingButtons(false);
                setupWatchingParametersTextEdits(false);
                connectButton.setEnabled(false);
                connectButton.setBackgroundColor(Color.GRAY);
            } else {
                messageBox("Not all required data available to start watching.");
            }
        }
    }

    float distance_between_points(float lat1, float long1, float lat2, float long2)
    {
        float half_dlat;
        float half_dlong;
        float a;
        float c;

        lat1 /= DEGREES_TO_RADS;
        long1 /= DEGREES_TO_RADS;
        lat2 /= DEGREES_TO_RADS;
        long2 /= DEGREES_TO_RADS;
        half_dlat = (lat2 - lat1) / 2.0f;
        half_dlong = (long2 - long1) / 2.0f;
        a = (float)(Math.sin(half_dlat) * Math.sin(half_dlat) + Math.sin(half_dlong) * Math.sin(half_dlong) * Math.cos(lat1) * Math.cos(lat2));
        c = 2.0f * (float)(Math.atan2(Math.sqrt(a), Math.sqrt(1.0f - a)));

        return 6371000.0f * c;
    }
}
