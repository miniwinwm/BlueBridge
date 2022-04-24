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
import androidx.fragment.app.DialogFragment;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.media.MediaPlayer;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.hivemq.client.mqtt.MqttClient;
import com.hivemq.client.mqtt.mqtt3.Mqtt3AsyncClient;
import com.hivemq.client.mqtt.mqtt3.Mqtt3BlockingClient;
import com.hivemq.client.mqtt.mqtt3.message.connect.connack.Mqtt3ConnAckReturnCode;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.List;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends AppCompatActivity implements MqttSettingsDialogFragment.MqttSettingsDialogListener {
    final long maxGpsDataAge = 40000;
    final long maxDepthDataAge = 40000;
    final long maxWindDataAge = 40000;
    final long maxHeadingDataAge = 40000;
    final long maxPressureDataAge = 60000;
    final long alarmRearmTime = 60000;
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
    volatile boolean isConnected = false;
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
    long settingsCode;
    String settingsBroker;
    int settingsPort;
    boolean settingsWatchingPing;
    String settingsCodeHexString;
    ImageButton settingsButton;
    int settingsConnection;
    Mqtt3AsyncClient client;
    ImageView bluetoothImageView;
    ImageView strength0ImageView;
    ImageView strength1ImageView;
    ImageView strength2ImageView;
    ImageView strength3ImageView;
    ImageView strength4ImageView;
    ImageView strength5ImageView;

    Thread thread = new Thread() {
        byte[] nmeaMessageArray = new byte[101];
        public void run() {
            byte[] bluetoothBytes = new byte[20];
            int nextBytePosition = 0;
            long lastUpdateTime = 0;

            while (true) {
                if (isConnected) {
                    if (settingsConnection == 0) {
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
                                } else {
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
                                                        } catch (Exception e) {
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
                                                        } catch (Exception e) {
                                                        }
                                                    }
                                                } else if (nmeaHeader.equals("HDT")) {
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
                                                        } catch (Exception e) {
                                                        }
                                                    }
                                                } else if (nmeaHeader.equals("MWV")) {
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
                                                        } catch (Exception e) {
                                                        }
                                                    }
                                                } else if (nmeaHeader.equals("RMC")) {
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
                                                            } catch (Exception e) {
                                                            }

                                                            String fieldString3 = fieldList.get(3);
                                                            boolean latitudeParsedOk = false;
                                                            boolean latitudeHemisphereParsedOk = false;
                                                            try {
                                                                float latitudeNmea = Float.parseFloat(fieldString3);
                                                                latitudeNmea /= 100.0f;
                                                                latitude = (float) Math.floor((float) latitudeNmea);
                                                                latitudeNmea -= latitude;
                                                                latitudeNmea /= 0.6f;
                                                                latitude += latitudeNmea;
                                                                latitudeParsedOk = true;
                                                            } catch (Exception e) {
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
                                                                longitude = (float) Math.floor((float) longitudeNmea);
                                                                longitudeNmea -= longitude;
                                                                longitudeNmea /= 0.6f;
                                                                longitude += longitudeNmea;
                                                                longitudeParsedOk = true;
                                                            } catch (Exception e) {
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
                                    } else {
                                        nmeaMessageStarted = false;
                                    }
                                }
                            }
                        } catch (IOException e) {
                            if (connectionCloseRequest) {
                                connectionCloseRequest = false;
                            } else {
                                isConnected = false;

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

                        runOnUiThread(new Runnable() {
                            public void run() {
                                if (System.currentTimeMillis() - depthReceivedTime > maxDepthDataAge) {
                                    textViewDepth.setText("----");
                                }

                                if (System.currentTimeMillis() - sogReceivedTime > maxGpsDataAge) {
                                    textViewSog.setText("----");
                                }

                                if (System.currentTimeMillis() - windspeedReceivedTime > maxWindDataAge) {
                                    textViewWindspeed.setText("----");
                                }

                                if (System.currentTimeMillis() - headingReceivedTime > maxWindDataAge) {
                                    textViewHeading.setText("----");
                                }

                                if (System.currentTimeMillis() - pressureReceivedTime > maxPressureDataAge) {
                                    textViewPressure.setText("----");
                                }
                            }
                        });
                    } else if (settingsConnection == 1) {
                        runOnUiThread(new Runnable() {
                            public void run() {
                                if (System.currentTimeMillis() - depthReceivedTime < maxDepthDataAge) {
                                    textViewDepth.setText(Float.toString(depth) + " m");
                                } else {
                                    textViewDepth.setText("----");
                                }

                                if (System.currentTimeMillis() - sogReceivedTime < maxGpsDataAge) {
                                    textViewSog.setText(Float.toString(sog) + " Kts");
                                } else {
                                    textViewSog.setText("----");
                                }

                                if (System.currentTimeMillis() - windspeedReceivedTime < maxWindDataAge) {
                                    textViewWindspeed.setText(Float.toString(windspeed) + " Kts");
                                } else {
                                    textViewWindspeed.setText("----");
                                }

                                if (System.currentTimeMillis() - headingReceivedTime < maxWindDataAge) {
                                    textViewHeading.setText(Integer.toString(Math.round(heading)) + "°");
                                } else {
                                    textViewHeading.setText("----");
                                }

                                if (System.currentTimeMillis() - pressureReceivedTime < maxPressureDataAge) {
                                    textViewPressure.setText(Integer.toString(Math.round(pressure)) + " mb");
                                } else {
                                    textViewPressure.setText("----");
                                }
                            }
                        });
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
                if (settingsWatchingPing && isWatching && System.currentTimeMillis() - lastPingTime > 10000 && !dataLossAlarmActive && !watchingAlarmActive) {
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

    public void onDialogPositiveClick(DialogFragment dialog)
    {
        // reload settings
        settingsCode = preferences.getLong("code", 0);
        settingsCodeHexString = String.format("%08X", settingsCode);
        settingsBroker = preferences.getString("broker", "broker.emqx.io");
        settingsPort = preferences.getInt("port", 1883);
        settingsConnection = preferences.getInt("connection", 0);
        settingsWatchingPing = (preferences.getInt("ping", 0) == 0) ? false : true;
    }

    public void onDialogNegativeClick(DialogFragment dialog)
    {
    }

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

    private void setSignalStrengthIcon(int strength) {
        runOnUiThread(new Runnable() {
            public void run() {
                if (strength < 0) {
                    strength0ImageView.setVisibility(View.INVISIBLE);
                    strength1ImageView.setVisibility(View.INVISIBLE);
                    strength2ImageView.setVisibility(View.INVISIBLE);
                    strength3ImageView.setVisibility(View.INVISIBLE);
                    strength4ImageView.setVisibility(View.INVISIBLE);
                    strength5ImageView.setVisibility(View.INVISIBLE);
                } else if (strength == 0) {
                    strength0ImageView.setVisibility(View.VISIBLE);
                    strength1ImageView.setVisibility(View.INVISIBLE);
                    strength2ImageView.setVisibility(View.INVISIBLE);
                    strength3ImageView.setVisibility(View.INVISIBLE);
                    strength4ImageView.setVisibility(View.INVISIBLE);
                    strength5ImageView.setVisibility(View.INVISIBLE);
                } else if (strength == 1) {
                    strength0ImageView.setVisibility(View.INVISIBLE);
                    strength1ImageView.setVisibility(View.VISIBLE);
                    strength2ImageView.setVisibility(View.INVISIBLE);
                    strength3ImageView.setVisibility(View.INVISIBLE);
                    strength4ImageView.setVisibility(View.INVISIBLE);
                    strength5ImageView.setVisibility(View.INVISIBLE);
                } else if (strength == 2) {
                    strength0ImageView.setVisibility(View.INVISIBLE);
                    strength1ImageView.setVisibility(View.INVISIBLE);
                    strength2ImageView.setVisibility(View.VISIBLE);
                    strength3ImageView.setVisibility(View.INVISIBLE);
                    strength4ImageView.setVisibility(View.INVISIBLE);
                    strength5ImageView.setVisibility(View.INVISIBLE);
                } else if (strength == 3) {
                    strength0ImageView.setVisibility(View.INVISIBLE);
                    strength1ImageView.setVisibility(View.INVISIBLE);
                    strength2ImageView.setVisibility(View.INVISIBLE);
                    strength3ImageView.setVisibility(View.VISIBLE);
                    strength4ImageView.setVisibility(View.INVISIBLE);
                    strength5ImageView.setVisibility(View.INVISIBLE);
                } else if (strength == 4) {
                    strength0ImageView.setVisibility(View.INVISIBLE);
                    strength1ImageView.setVisibility(View.INVISIBLE);
                    strength2ImageView.setVisibility(View.INVISIBLE);
                    strength3ImageView.setVisibility(View.INVISIBLE);
                    strength4ImageView.setVisibility(View.VISIBLE);
                    strength5ImageView.setVisibility(View.INVISIBLE);
                } else if (strength == 5) {
                    strength0ImageView.setVisibility(View.INVISIBLE);
                    strength1ImageView.setVisibility(View.INVISIBLE);
                    strength2ImageView.setVisibility(View.INVISIBLE);
                    strength3ImageView.setVisibility(View.INVISIBLE);
                    strength4ImageView.setVisibility(View.INVISIBLE);
                    strength5ImageView.setVisibility(View.VISIBLE);
                }
            }
        });
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
        anchorView = (AnchorView)findViewById((R.id.anchor_view));
        settingsButton = (ImageButton)findViewById((R.id.settingsButton));
        bluetoothImageView = (ImageView)findViewById(R.id.bluetoothImageView);
        strength0ImageView = (ImageView)findViewById(R.id.strength0ImageView);
        strength1ImageView = (ImageView)findViewById(R.id.strength1ImageView);
        strength2ImageView = (ImageView)findViewById(R.id.strength2ImageView);
        strength3ImageView = (ImageView)findViewById(R.id.strength3ImageView);
        strength4ImageView = (ImageView)findViewById(R.id.strength4ImageView);
        strength5ImageView = (ImageView)findViewById(R.id.strength5ImageView);
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
        settingsCode = preferences.getLong("code", 0);
        settingsCodeHexString = String.format("%08X", settingsCode);
        settingsBroker = preferences.getString("broker", "broker.emqx.io");
        settingsPort = preferences.getInt("port", 1883);
        settingsConnection = preferences.getInt("connection", 0);
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
        settingsButton.setBackgroundColor(Color.WHITE);
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

    public void settingsButtonOnClick(View view) {
        new MqttSettingsDialogFragment(settingsCode, settingsBroker, settingsPort, settingsConnection, settingsWatchingPing, preferences).show(getSupportFragmentManager(), "MQTTSD");
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

    private void connectBluetooth(View view) {
        if (isConnected) {
            // disconnect
            connectionCloseRequest = true;
            isConnected = false;
            try {
                bluetoothInputStream.close();
            } catch (IOException e) {
            }
            try {
                bluetoothSocket.close();
            } catch (IOException e) {
            }
            connectButton.setBackgroundColor(Color.GREEN);
            watchingButton.setBackgroundColor(Color.GRAY);
            watchingButton.setEnabled(false);
            watchingButton.setText("START");
            connectButton.setText("CONNECT");
            isWatching = false;
            resetAllCurrentReadingsTexts();
            settingsButton.setEnabled(true);
            settingsButton.setVisibility(View.VISIBLE);
            bluetoothImageView.setVisibility(View.INVISIBLE);
        } else {
            // connect
            if (bluetoothAdapter == null) {
                messageBox("Bluetooth is not supported");
            } else if (!bluetoothAdapter.isEnabled()) {
                messageBox("Bluetooth is not enabled");
            } else {
                connectButton.setEnabled(false);
                settingsButton.setEnabled(false);
                settingsButton.setVisibility(View.INVISIBLE);
                bluetoothImageView.setVisibility(View.VISIBLE);
                connectButton.setText("CONNECTING");
                connectButton.setBackgroundColor(Color.GRAY);

                Thread connectThread = new Thread(new Runnable() {
                    public void run() {
                        Set<BluetoothDevice> pairedDevices = bluetoothAdapter.getBondedDevices();
                        boolean deviceFound = false;
                        if (pairedDevices.size() > 0) {
                            for (BluetoothDevice device : pairedDevices) {
                                String devicename = device.getName();
                                if (devicename.equals("BlueBridge")) {
                                    deviceFound = true;
                                    try {
                                        bluetoothSocket = device.createRfcommSocketToServiceRecord(MY_UUID_SECURE);
                                        bluetoothSocket.connect();
                                        if (bluetoothSocket.isConnected()) {
                                            bluetoothInputStream = bluetoothSocket.getInputStream();
                                            isConnected = true;
                                        } else {
                                            messageBoxThread("Could not connect to BlueBridge");
                                        }
                                    } catch (IOException e) {
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
                                if (isConnected) {
                                    connectButton.setBackgroundColor(Color.RED);
                                    connectButton.setText("DISCONNECT");
                                    watchingButton.setBackgroundColor(Color.GREEN);
                                    watchingButton.setEnabled(true);
                                    nmeaMessageStarted = false;
                                } else {
                                    connectButton.setBackgroundColor(Color.GREEN);
                                    connectButton.setText("CONNECT");
                                    settingsButton.setEnabled(true);
                                    settingsButton.setVisibility(View.VISIBLE);
                                    bluetoothImageView.setVisibility(View.INVISIBLE);
                                }
                            }
                        });
                    }
                });
                connectThread.start();
            }
        }
    }

    private void connectInternet(View view) {
        if (isConnected) {
            // disconnect
            isConnected = false;
            connectButton.setBackgroundColor(Color.GREEN);
            watchingButton.setBackgroundColor(Color.GRAY);
            watchingButton.setEnabled(false);
            watchingButton.setText("START");
            connectButton.setText("CONNECT");
            isWatching = false;
            resetAllCurrentReadingsTexts();
            settingsButton.setEnabled(true);
            settingsButton.setVisibility(View.VISIBLE);
            client.disconnect();
            setSignalStrengthIcon(-1);
        } else {
            if (!isNetworkAvailable()) {
                messageBox("No internet connection available");
            } else if (settingsCode == 0L) {
                messageBox("Code not set in settings");
            } else {
                connectButton.setEnabled(false);
                settingsButton.setEnabled(false);
                settingsButton.setVisibility(View.INVISIBLE);
                connectButton.setText("CONNECTING");
                connectButton.setBackgroundColor(Color.GRAY);
                setSignalStrengthIcon(0);

                Thread connectThread = new Thread(new Runnable() {
                    public void run() {
                        Mqtt3BlockingClient blockingClient = MqttClient.builder()
                                .useMqttVersion3()
                                .serverHost(settingsBroker)
                                .serverPort(settingsPort)
                                .buildBlocking();
                        try {
                            if (blockingClient.connectWith().keepAlive(30).send().getReturnCode() == Mqtt3ConnAckReturnCode.SUCCESS) {
                                client = blockingClient.toAsync();
                                client.subscribeWith()
                                        .topicFilter(settingsCodeHexString + "/all")
                                        .callback(publish -> {
                                            String payload = new String(publish.getPayloadAsBytes());

                                            String[] split = payload.split(",");
                                            try {
                                                depth = Float.parseFloat(split[8]);
                                                depthReceivedTime = System.currentTimeMillis();
                                            } catch (Exception e) {
                                            }

                                            try {
                                                sog = Float.parseFloat(split[3]);
                                                sogReceivedTime = System.currentTimeMillis();
                                            } catch (Exception e) {
                                            }

                                            try {
                                                latitude = Float.parseFloat(split[13]);
                                                latitudeReceivedTime = System.currentTimeMillis();
                                            } catch (Exception e) {
                                            }

                                            try {
                                                longitude = Float.parseFloat(split[14]);
                                                longitudeReceivedTime = System.currentTimeMillis();
                                            } catch (Exception e) {
                                            }

                                            try {
                                                heading = Float.parseFloat(split[7]);
                                                headingReceivedTime = System.currentTimeMillis();
                                            } catch (Exception e) {
                                            }

                                            try {
                                                windspeed = Float.parseFloat(split[11]);
                                                windspeedReceivedTime = System.currentTimeMillis();
                                            } catch (Exception e) {
                                            }

                                            try {
                                                pressure = Float.parseFloat(split[15]);
                                                pressureReceivedTime = System.currentTimeMillis();
                                            } catch (Exception e) {
                                            }

                                            try {
                                                int signalStrength = Integer.parseInt(split[0]);
                                                if (signalStrength < 1) {
                                                    setSignalStrengthIcon(0);
                                                } else if (signalStrength < 9) {
                                                    setSignalStrengthIcon(1);
                                                } else if (signalStrength < 15) {
                                                    setSignalStrengthIcon(2);
                                                } else if (signalStrength < 21) {
                                                    setSignalStrengthIcon(3);
                                                } else if (signalStrength < 27) {
                                                    setSignalStrengthIcon(4);
                                                } else {
                                                    setSignalStrengthIcon(5);
                                                }
                                            } catch (Exception e) {
                                                setSignalStrengthIcon(0);
                                            }
                                        })
                                        .send()
                                        .whenComplete((subAck, throwable) -> {
                                            if (throwable != null) {
                                                // Handle failure to subscribe
                                                messageBoxThread("Connection failure");
                                            } else {
                                                // subscribe success
                                                runOnUiThread(new Runnable() {
                                                    public void run() {
                                                        connectButton.setEnabled(true);
                                                        if (isConnected) {
                                                            connectButton.setBackgroundColor(Color.RED);
                                                            connectButton.setText("DISCONNECT");
                                                            watchingButton.setBackgroundColor(Color.GREEN);
                                                            watchingButton.setEnabled(true);
                                                            nmeaMessageStarted = false;
                                                        } else {
                                                            connectButton.setEnabled(true);
                                                            connectButton.setBackgroundColor(Color.GREEN);
                                                            connectButton.setText("CONNECT");
                                                            settingsButton.setEnabled(true);
                                                            settingsButton.setVisibility(View.VISIBLE);
                                                            setSignalStrengthIcon(-1);
                                                        }
                                                    }
                                                });

                                                isConnected = true;
                                            }
                                        });
                            }
                        } catch (Exception e) {
                            messageBoxThread("Connection refused");
                            runOnUiThread(new Runnable() {
                                public void run() {
                                    connectButton.setEnabled(true);
                                    connectButton.setBackgroundColor(Color.GREEN);
                                    connectButton.setText("CONNECT");
                                    settingsButton.setEnabled(true);
                                    settingsButton.setVisibility(View.VISIBLE);
                                    setSignalStrengthIcon(-1);
                                }
                            });
                        }
                    }
                });
                connectThread.start();
            }
        }
    }

    private boolean isNetworkAvailable() {
        ConnectivityManager connectivityManager
                = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo activeNetworkInfo = connectivityManager.getActiveNetworkInfo();
        return activeNetworkInfo != null && activeNetworkInfo.isConnected();
    }

    public void connectButtonOnClick(View view) {
        saveAllTextEdits();
        if (settingsConnection == 0) {
            // bluetooth connection
            connectBluetooth(view);
        } else {
            connectInternet(view);
        }

        resetAllCurrentReadingsTexts();
    }

    public void watchingButtonOnClick(View view) {
        saveAllTextEdits();
        anchorView.Reset();
        anchorView.drawAnchorView();

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
