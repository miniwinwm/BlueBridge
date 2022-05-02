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

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.text.Editable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.TextView;

import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.DialogFragment;

public class MqttSettingsDialogFragment extends DialogFragment {
    EditText codeEditText;
    EditText brokerEditText;
    EditText portEditText;
    RadioButton bluetoothRadioButton;
    RadioButton internetRadioButton;
    TextView codeTextView;
    TextView brokerTextView;
    TextView portTextView;
    CheckBox watchingPingCheckBox;
    Settings settings;

    public interface MqttSettingsDialogListener
    {
        public void onDialogPositiveClick(DialogFragment dialog);
        public void onDialogNegativeClick(DialogFragment dialog);
    }

    MqttSettingsDialogListener listener;

    public MqttSettingsDialogFragment(Settings settings)
    {
        this.settings = settings;
    }

    public void onAttach(Context context)
    {
        super.onAttach(context);
        try
        {
            listener = (MqttSettingsDialogListener)context;
        }
        catch (ClassCastException e)
        {
            // The activity doesn't implement the interface
        }
    }

    public Dialog onCreateDialog(Bundle savedInstanceState) {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
        LayoutInflater inflater = requireActivity().getLayoutInflater();
        View view = inflater.inflate(R.layout.mqtt_settings_dialog, null);

        codeEditText = (EditText)view.findViewById(R.id.codeEditText);
        brokerEditText = (EditText)view.findViewById(R.id.brokerEditText);
        portEditText = (EditText)view.findViewById(R.id.portEditText);
        bluetoothRadioButton = (RadioButton)view.findViewById(R.id.bluetoothRadioButton);
        internetRadioButton = (RadioButton)view.findViewById(R.id.internetRadioButton);
        codeTextView = (TextView)view.findViewById(R.id.codeTextView);
        brokerTextView = (TextView)view.findViewById(R.id.brokerTextView);
        portTextView = (TextView)view.findViewById(R.id.portTextView);
        watchingPingCheckBox = (CheckBox)view.findViewById(R.id.watchingPingCheckBox);

        bluetoothRadioButton.setOnClickListener(new View.OnClickListener()
        {
            public void onClick(View v)
            {
                codeEditText.setEnabled(false);
                brokerEditText.setEnabled(false);
                portEditText.setEnabled(false);
                codeTextView.setEnabled(false);
                brokerTextView.setEnabled(false);
                portTextView.setEnabled(false);
            }
        });

        internetRadioButton.setOnClickListener(new View.OnClickListener()
        {
            public void onClick(View v)
            {
                codeEditText.setEnabled(true);
                brokerEditText.setEnabled(true);
                portEditText.setEnabled(true);
                codeTextView.setEnabled(true);
                brokerTextView.setEnabled(true);
                portTextView.setEnabled(true);
            }
        });

        codeEditText.setText(settings.getCodeHexString());
        brokerEditText.setText(settings.getBroker());
        portEditText.setText(String.format("%d", settings.getPort()));
        if (settings.getConnectionType() == ConnectionType.BLUETOOTH) {
            bluetoothRadioButton.setChecked(true);
            internetRadioButton.setChecked(false);
            codeEditText.setEnabled(false);
            brokerEditText.setEnabled(false);
            portEditText.setEnabled(false);
            codeTextView.setEnabled(false);
            brokerTextView.setEnabled(false);
            portTextView.setEnabled(false);
        } else {
            bluetoothRadioButton.setChecked(false);
            internetRadioButton.setChecked(true);
            codeEditText.setEnabled(true);
            brokerEditText.setEnabled(true);
            portEditText.setEnabled(true);
            codeTextView.setEnabled(true);
            brokerTextView.setEnabled(true);
            portTextView.setEnabled(true);
        }
        watchingPingCheckBox.setChecked(settings.getWatchingPing());

        builder.setView(view)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        settings.setCodeHexString(codeEditText.getText().toString());
                        settings.setBroker(brokerEditText.getText().toString());
                        settings.setPort(portEditText.getText().toString());
                        settings.setConnectionType(bluetoothRadioButton.isChecked() ? ConnectionType.BLUETOOTH : ConnectionType.INTERNET);
                        settings.setWatchingPing(watchingPingCheckBox.isChecked());
                        listener.onDialogPositiveClick(MqttSettingsDialogFragment.this);
                    }
                })
                .setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        MqttSettingsDialogFragment.this.getDialog().cancel();
                    }
                });

        return builder.create();
    }
}
