package com.example.bluebridgeapp;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;

public class AnchorView extends View {
    private static final int maxPosDiffMetresCount = 250;
    float[] xPosDiffsMetres;
    float[] yPosDiffsMetres;
    int posDiffsMetresCount;
    int nextPosDiffMetresPos;

    public AnchorView(Context context) {
        super(context);
        Reset();
    }

    public void Reset() {
        xPosDiffsMetres = new float[maxPosDiffMetresCount];
        yPosDiffsMetres = new float[maxPosDiffMetresCount];
        posDiffsMetresCount = 0;
        nextPosDiffMetresPos = 0;
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

    public AnchorView(Context context, AttributeSet attrs) {
        super(context, attrs);
        Reset();
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
    }

    public void drawAnchorView() {
        invalidate();
    }
}
