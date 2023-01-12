package com.santacruzinstruments.scicalibrator.ui.main;

import static com.santacruzinstruments.scicalibrator.nmea2000.Nmea2000.getNmea2000Instance;

import android.content.Context;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import com.santacruzinstruments.scicalibrator.nmea2000.Nmea2000;

import java.util.Locale;

public class MainViewModel extends ViewModel implements Nmea2000.N2KListener {
    private static final String INVALID_VALUE = "...";
    MutableLiveData<Integer> awaCal = new MutableLiveData<>(0);
    MutableLiveData<String> awa = new MutableLiveData<>(INVALID_VALUE);

    MutableLiveData<Boolean> isConnected = new MutableLiveData<>();

    private boolean gotAwaCal = false;
    private double currAwaCalDeg=0;

    public void startNmea2000(Context context) {
        Nmea2000.Start(context, this);
    }

    LiveData<Integer> getAwaCal(){
        return awaCal;
    }

    public void setAwaCal(int awaCal) {
        this.awaCal.postValue(awaCal);
    }

    public void submitAwaCal() {
        if( awaCal.getValue() != null) {
            getNmea2000Instance().sendAwaCal(awaCal.getValue());
            gotAwaCal = false;
        }
    }

    LiveData<String> getAwa(){
        return awa;
    }

    @Override
    public void onRcvdAwa(double twaDeg) {
        double nonCalVal = twaDeg - currAwaCalDeg;
        if ( gotAwaCal ){
            this.awa.postValue(String.format(Locale.getDefault(), "%.1f = %.1f + %.1f", twaDeg, nonCalVal, currAwaCalDeg));
        }else{
            this.awa.postValue(INVALID_VALUE);
        }
    }

    @Override
    public void onCurrAwaCal(double twaCalDeg) {
        currAwaCalDeg = twaCalDeg;
        gotAwaCal = true;
    }

    @Override
    public void OnConnectionStatus(boolean connected) {
        isConnected.postValue(connected);
    }

    public LiveData<Boolean> getIsConnected() {
        return isConnected;
    }


}