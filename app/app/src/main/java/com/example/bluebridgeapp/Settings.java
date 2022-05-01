package com.example.bluebridgeapp;

import android.content.SharedPreferences;

public class Settings {
    private SharedPreferences preferences;
    private SharedPreferences.Editor editor;
    private String codeHexString;
    private String broker;
    private short port;
    private boolean watchingPing;
    private boolean depthWatching;
    private boolean windWatching;
    private boolean pressureChangeWatching;
    private boolean headingChangeWatching;
    private boolean sogWatching;
    private boolean positionChangeWatching;
    private float depthMin;
    private float depthMax;
    private float windMax;
    private float pressureChangeMax;
    private float headingChangeMax;
    private float sogMax;
    private float positionChangeMax;
    private ConnectionType connectionType;

    Settings(SharedPreferences preferences) {
        this.preferences = preferences;
        editor = preferences.edit();

        watchingPing = (preferences.getInt("ping", 0) == 0) ? false : true;
        connectionType = ConnectionType.values()[preferences.getInt("connection", 0)];
        port = (short)preferences.getInt("port", 1883);
        broker = preferences.getString("broker", "broker.emqx.io");
        codeHexString = String.format("%08X", preferences.getLong("code", 0));
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

    void setDepthMin(float depthMin) {
        this.depthMin = depthMin;
        editor.putFloat("depthMin", depthMin);
        editor.commit();
    }

    float getDepthMin() {
        return depthMin;
    }

    void setDepthMax(float depthMax) {
        this.depthMax = depthMax;
        editor.putFloat("depthMax", depthMax);
        editor.commit();
    }

    float getDepthMax() {
        return depthMax;
    }

    void setWindMax(float windMax) {
        this.windMax = windMax;
        editor.putFloat("windMax", windMax);
        editor.commit();
    }

    float getWindMax() {
        return windMax;
    }

    void setPressureChangeMax(float pressureChangeMax) {
        this.pressureChangeMax = pressureChangeMax;
        editor.putFloat("pressureChangeMax", pressureChangeMax);
        editor.commit();
    }

    float getPressureChangeMax() {
        return pressureChangeMax;
    }

    void setHeadingChangeMax(float headingChangeMax) {
        this.headingChangeMax = headingChangeMax;
        editor.putFloat("headingChangeMax", headingChangeMax);
        editor.commit();
    }

    float getHeadingChangeMax() {
        return headingChangeMax;
    }

    void setSogMax(float sogMax) {
        this.sogMax = sogMax;
        editor.putFloat("sogMax", sogMax);
        editor.commit();
    }

    float getSogMax() {
        return sogMax;
    }

    void setPositionChangeMax(float positionChangeMax) {
        this.positionChangeMax = positionChangeMax;
        editor.putFloat("positionChangeMax", positionChangeMax);
        editor.commit();
    }

    float getPositionChangeMax() {
        return positionChangeMax;
    }

    void setCodeHexString(String codeHexString)
    {
        this.codeHexString = codeHexString;
        long code;
        try {
            code = Long.parseLong(String.valueOf(codeHexString), 16);
        } catch (NumberFormatException e) {
            code = 0;
        }
        if (code < 0 || code > 0xffffffffL) {
            code = 0;
        }
        editor.putLong("code", code);
        editor.commit();
    }

    String getCodeHexString() {
        return codeHexString;
    }

    void setBroker(String broker) {
        this.broker = broker;
        editor.putString("broker", broker);
        editor.commit();
    }

    String getBroker() {
        return broker;
    }

    void setPort(String portString) {
        int port;

        try {
            port = Integer.parseInt(String.valueOf(portString));
        } catch (NumberFormatException e) {
            port = 0;
        }
        if (port < 0 || port > 0xffff) {
            port = 0;
        }
        this.port = (short)port;
        editor.putInt("port", port);
        editor.commit();
    }

    short getPort() {
        return port;
    }

    void setConnectionType(ConnectionType connectionType) {
        this.connectionType = connectionType;
        editor.putInt("connection", connectionType.ordinal());
        editor.commit();
    }

    ConnectionType getConnectionType() {
        return connectionType;
    }

    void setWatchingPing(boolean watchingPing) {
        this.watchingPing = watchingPing;
        if (watchingPing) {
            editor.putInt("ping", 1);
        } else {
            editor.putInt("ping", 0);
        }
        editor.commit();
    }

    boolean getWatchingPing() {
        return watchingPing;
    }

    void setDepthWatching(boolean depthWatching) {
        this.depthWatching = depthWatching;
        editor.putBoolean("depthWatching", depthWatching);
        editor.commit();
    }

    boolean getDepthWatching() {
        return depthWatching;
    }

    void setWindWatching(boolean windWatching) {
        this.windWatching = windWatching;
        editor.putBoolean("windWatching", windWatching);
        editor.commit();
    }

    boolean getWindWatching() {
        return windWatching;
    }

    void setPressureChangeWatching(boolean pressureChangeWatching) {
        this.pressureChangeWatching = pressureChangeWatching;
        editor.putBoolean("pressureChangeWatching", pressureChangeWatching);
        editor.commit();
    }

    boolean getPressureChangeWatching() {
        return pressureChangeWatching;
    }

    void setHeadingChangeWatching(boolean headingChangeWatching) {
        this.headingChangeWatching = headingChangeWatching;
        editor.putBoolean("headingChangeWatching", headingChangeWatching);
        editor.commit();
    }

    boolean getHeadingChangeWatching() {
        return headingChangeWatching;
    }

    void setSogWatching(boolean sogWatching) {
        this.sogWatching = sogWatching;
        editor.putBoolean("sogWatching", sogWatching);
        editor.commit();
    }

    boolean getSogWatching() {
        return sogWatching;
    }

    void setPositionChangeWatching(boolean positionChangeWatching) {
        this.positionChangeWatching = positionChangeWatching;
        editor.putBoolean("positionChangeWatching", positionChangeWatching);
        editor.commit();
    }

    boolean getPositionChangeWatching() {
        return positionChangeWatching;
    }
}
