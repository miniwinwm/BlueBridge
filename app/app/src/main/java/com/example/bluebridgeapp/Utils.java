/*
MIT License

Copyright (c) John Blaiklock 2022 BlueBridge

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

import android.app.Activity;
import android.view.inputmethod.InputMethodManager;

public class Utils {
    public final static float DEGREES_TO_RADS = 57.296f;

    /**
     * Calculate distance in metres between 2 points coordinates in degrees
     *
     * @param lat1 Latitude of first point in degrees
     * @param long1 longitude of first point in degrees
     * @param lat2 Latitude of second point in degrees
     * @param long2 longitude of second point in degrees
     * @return Distance between points in m
     * */
    public static float DistanceBetweenPoints(float lat1, float long1, float lat2, float long2)
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

    /**
     * Hide the soft keyboard shown when text input is expected
     *
     * @param activity The activity owning the input control corresponding to the soft keyboard
     */
    public static void hideSoftKeyboard(Activity activity) {
        InputMethodManager inputMethodManager = (InputMethodManager) activity.getSystemService(Activity.INPUT_METHOD_SERVICE);
        if (activity.getCurrentFocus() != null) {
            if (inputMethodManager.isAcceptingText()) {
                inputMethodManager.hideSoftInputFromWindow(activity.getCurrentFocus().getWindowToken(),0);
            }
        }
    }
}
