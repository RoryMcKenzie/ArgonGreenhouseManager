package com.example.greenhousemanager;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import com.google.android.material.snackbar.Snackbar;

import org.eclipse.paho.android.service.MqttAndroidClient;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;

public class MainActivity extends AppCompatActivity {

    //Declare MQTT Client, Notification Manager and Notification Builder
    public MqttAndroidClient client;
    NotificationManagerCompat notificationManager;
    NotificationCompat.Builder builder;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        setupNotifications();
        MQTT();
    }

    private void setupNotifications(){
        //Create NotificationChannel, only needed if using Android Oreo or newer
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            CharSequence name = getString(R.string.channel_name);
            String description = getString(R.string.channel_description);
            int importance = NotificationManager.IMPORTANCE_DEFAULT;
            NotificationChannel channel = new NotificationChannel("100", name, importance);
            channel.setDescription(description);
            // Register the channel with the system
            NotificationManager notificationManager = getSystemService(NotificationManager.class);
            notificationManager.createNotificationChannel(channel);
        }

        //Create an  intent, this decides what happens when the app is tapped
        //For this notification, it will disappear
        Intent intent = new Intent(this, MainActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, 0);

        //Set notification content
        builder = new NotificationCompat.Builder(this, "100")
                .setSmallIcon(R.drawable.ic_stat_warning)
                .setContentTitle("Security Warning")
                .setContentText("An incorrect password has been entered 3 times in a row")
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                .setContentIntent(pendingIntent);

        notificationManager = NotificationManagerCompat.from(this);
    }
    //Method for all MQTT functionality
    public void MQTT(){
        //Initiates 2 new TextViews, for temperature and humidity
        final TextView temperature = findViewById(R.id.tv_temp);
        final TextView humidity = findViewById(R.id.tv_hum);
        //Create MQTT client
        String clientId = MqttClient.generateClientId();
        client = new MqttAndroidClient(this.getApplicationContext(), "tcp://test.mosquitto.org:1883", clientId);

        try {
            //Connect to MQTT broker
            IMqttToken token = client.connect();
            token.setActionCallback(new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    //Connection successful, subscribe to relevant channels
                    try {
                        if (client.isConnected()) {
                            client.subscribe("tempandhum", 0);
                            client.subscribe("notification", 0);
                            client.subscribe("readingerror",0);
                            publishRefresh();
                            client.setCallback(new MqttCallback() {
                                @Override
                                public void connectionLost(Throwable cause) {
                                }

                                @Override
                                public void messageArrived(String topic, MqttMessage message) throws Exception {
                                    //This method is called when the client receives an MQTT message
                                    if (topic.equals("tempandhum")) {
                                        //If it's from the topic tempandhum, the message is split
                                        // into temp and humidity, rounded and displayed
                                        String[] parsed = message.toString().split(",");

                                        double temp = Math.round(Double.parseDouble(parsed[0]) * 10) / 10.0;
                                        double hum = Math.round(Double.parseDouble(parsed[1]) * 10) / 10.0;

                                        temperature.setText(Double.toString(temp));
                                        humidity.setText(Double.toString(hum));
                                    } else if (topic.equals("notification")){
                                        //create notification
                                        notificationManager.notify(0, builder.build());
                                    } else if (topic.equals("readingerror")){
                                        final Snackbar snackbar = Snackbar.make((findViewById(android.R.id.content)), "Sensor reading failed. Please retry.", Snackbar.LENGTH_INDEFINITE);
                                        snackbar.setAction("RETRY", new View.OnClickListener() {
                                            @Override
                                            public void onClick(View v) {
                                                publishRefresh();
                                                snackbar.dismiss();
                                            }
                                        });
                                        snackbar.show();
                                    }
                                }

                                @Override
                                public void deliveryComplete(IMqttDeliveryToken token) {
                                }
                            });
                        }
                    } catch (Exception e) {
                        Log.e("error", "couldn't subscribe");
                    }
                }
                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    Log.d("connect", "onFailure");
                }
            });
        } catch (MqttException e) {
            Log.e("error", "error occurred");
        }
    }

    //When refresh button clicked, call publishRefresh() method
    public void onClickRefresh(View v)
    {
        publishRefresh();
    }

    //Publishes MQTT message requesting temperature and humidity data from Argon
    public void publishRefresh(){
        String topic = "tempandhumrequest";
        String payload = "refresh";
        try {
            MqttMessage message = new MqttMessage(payload.getBytes());
            client.publish(topic, message);
        } catch (MqttException e) {
            e.printStackTrace();
        }
    }
}
