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

import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

import androidx.appcompat.app.AlertDialog;

public class AnchorView extends View {
    private static final int maxPosDiffMetresCount = 250;
    float[] xPosDiffsMetres;
    float[] yPosDiffsMetres;
    float trueWindAngle;
    int posDiffsMetresCount;
    int nextPosDiffMetresPos;
    AlertDialog alert;
    boolean drawTrueWindAngleRequired;

    public AnchorView(Context context, AttributeSet attrs) {
        super(context, attrs);
        Reset();

        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        builder.setMessage("Delete all existing points?");
        builder.setCancelable(true);

        builder.setPositiveButton(
                "Yes",
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        dialog.cancel();
                        Reset();
                        drawAnchorView();
                    }
                });

        builder.setNegativeButton(
                "No",
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        dialog.cancel();
                    }
                });

        alert = builder.create();

        setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                alert.show();
            }
        });
    }

    public void Reset() {
        xPosDiffsMetres = new float[maxPosDiffMetresCount];
        yPosDiffsMetres = new float[maxPosDiffMetresCount];
        posDiffsMetresCount = 0;
        nextPosDiffMetresPos = 0;
    }

    public void AddTrueWindAngle(float trueWindAngle) {
        this.trueWindAngle = trueWindAngle;
        drawTrueWindAngleRequired = true;
    }

    public void AddPosDiffMetres(float xDiffMetres, float yDiffMetres) {
        xPosDiffsMetres[nextPosDiffMetresPos] = xDiffMetres;
        yPosDiffsMetres[nextPosDiffMetresPos] = -yDiffMetres;
        posDiffsMetresCount++;
        if (posDiffsMetresCount == maxPosDiffMetresCount) {
            posDiffsMetresCount = maxPosDiffMetresCount;
        }
        nextPosDiffMetresPos++;
        if (nextPosDiffMetresPos == maxPosDiffMetresCount) {
            nextPosDiffMetresPos = 0;
        }
    }

    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        Paint paint = new Paint();
        canvas.drawRGB(3, 119, 252);
        int width = getWidth();
        int height = getHeight();
        int widthHeightMin = Math.min(width, height);
        float pixelsPerMetre = (float)widthHeightMin / 200.0f;

        paint.setColor(Color.YELLOW);
        paint.setTextSize(20);
        canvas.drawText("25m", 10, 20, paint);
        canvas.drawLine(10, 30, 10 + pixelsPerMetre * 25, 30, paint);
        canvas.drawLine(10, 25, 10, 30, paint);
        canvas.drawLine(10 + pixelsPerMetre * 25, 25, 10 + pixelsPerMetre * 25, 30, paint);
        canvas.drawLine(width / 2 - 10, height / 2 - 10, width / 2 + 10, height / 2 + 10, paint);
        canvas.drawLine(width / 2 + 10, height / 2 - 10, width / 2 - 10, height / 2 + 10, paint);

        int i;
        paint.setColor(Color.BLACK);
        for (i = 0; i < posDiffsMetresCount - 1; i++) {
            canvas.drawPoint(width / 2 + xPosDiffsMetres[i] * pixelsPerMetre,
                    height / 2 + yPosDiffsMetres[i] * pixelsPerMetre, paint);
            canvas.drawPoint(width / 2 + xPosDiffsMetres[i] * pixelsPerMetre - 1,
                    height / 2 + yPosDiffsMetres[i] * pixelsPerMetre -1, paint);
            canvas.drawPoint(width / 2 + xPosDiffsMetres[i] * pixelsPerMetre + 1,
                    height / 2 + yPosDiffsMetres[i] * pixelsPerMetre + 1, paint);
            canvas.drawPoint(width / 2 + xPosDiffsMetres[i] * pixelsPerMetre - 1,
                    height / 2 + yPosDiffsMetres[i] * pixelsPerMetre + 1, paint);
            canvas.drawPoint(width / 2 + xPosDiffsMetres[i] * pixelsPerMetre + 1,
                    height / 2 + yPosDiffsMetres[i] * pixelsPerMetre - 1, paint);
        }

        paint.setColor(Color.YELLOW);
        canvas.drawPoint(width / 2 + xPosDiffsMetres[i] * pixelsPerMetre,
                height / 2 + yPosDiffsMetres[i] * pixelsPerMetre, paint);
        canvas.drawPoint(width / 2 + xPosDiffsMetres[i] * pixelsPerMetre - 1,
                height / 2 + yPosDiffsMetres[i] * pixelsPerMetre -1, paint);
        canvas.drawPoint(width / 2 + xPosDiffsMetres[i] * pixelsPerMetre + 1,
                height / 2 + yPosDiffsMetres[i] * pixelsPerMetre + 1, paint);
        canvas.drawPoint(width / 2 + xPosDiffsMetres[i] * pixelsPerMetre - 1,
                height / 2 + yPosDiffsMetres[i] * pixelsPerMetre + 1, paint);
        canvas.drawPoint(width / 2 + xPosDiffsMetres[i] * pixelsPerMetre + 1,
                height / 2 + yPosDiffsMetres[i] * pixelsPerMetre - 1, paint);

        if (drawTrueWindAngleRequired) {
            drawTrueWindAngleRequired = false;

            paint.setAntiAlias(true);
            paint.setStrokeWidth(4f);
            paint.setStyle(Paint.Style.STROKE);

            float startX = width - 60.0f;
            float startY = height - 60.0f;
            float endX = startX + (float) (40.0 * Math.sin(Math.toRadians(trueWindAngle)));
            float endY = startY - (float) (40.0 * Math.cos(Math.toRadians(trueWindAngle)));
            canvas.drawLine(startX, startY, endX, endY, paint);

            float endX2 = startX + (float) (25.0 * Math.sin(Math.toRadians(trueWindAngle - 10.0f)));
            float endY2 = startY - (float) (25.0 * Math.cos(Math.toRadians(trueWindAngle - 10.0f)));
            canvas.drawLine(endX2, endY2, endX, endY, paint);
            float endX3 = startX + (float) (25.0 * Math.sin(Math.toRadians(trueWindAngle + 10.0f)));
            float endY3 = startY - (float) (25.0 * Math.cos(Math.toRadians(trueWindAngle + 10.0f)));
            canvas.drawLine(endX3, endY3, endX, endY, paint);
        }
    }

    public void drawAnchorView() {
        invalidate();
    }
}
