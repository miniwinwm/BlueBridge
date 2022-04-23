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

    long code;
    String broker;
    int port;
    int connection;
    SharedPreferences preferences;

    public interface MqttSettingsDialogListener
    {
        public void onDialogPositiveClick(DialogFragment dialog);
        public void onDialogNegativeClick(DialogFragment dialog);
    }

    MqttSettingsDialogListener listener;

    public MqttSettingsDialogFragment(long code, String broker, int port, int connection, SharedPreferences preferences)
    {
        this.code = code;
        this.broker = broker;
        this.port = port;
        this.connection = connection;
        this.preferences = preferences;
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

        codeEditText.setText(String.format("%08X", code));
        brokerEditText.setText(broker);
        portEditText.setText(String.format("%d", port));
        if (connection == 0) {
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

        builder.setView(view)
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int id) {
                        SharedPreferences.Editor editor = preferences.edit();

                        // code
                        try {
                            code = Long.parseLong(String.valueOf(codeEditText.getText()), 16);
                        } catch (NumberFormatException e) {
                            code = 0;
                        }
                        if (code < 0 || code > 0xffffffffL) {
                            code = 0;
                        }
                        editor.putLong("code", code);

                        // broker
                        editor.putString("broker", String.valueOf(brokerEditText.getText()));

                        // port
                        try {
                            port = Integer.parseInt(String.valueOf(portEditText.getText()));
                        } catch (NumberFormatException e) {
                            port = 0;
                        }
                        if (port < 0 || port > 0xffff) {
                            port = 0;
                        }
                        editor.putInt("port", port);

                        // connection
                        if (bluetoothRadioButton.isChecked()) {
                            editor.putInt("connection", 0);
                        } else {
                            editor.putInt("connection", 1);
                        }

                        editor.commit();
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
