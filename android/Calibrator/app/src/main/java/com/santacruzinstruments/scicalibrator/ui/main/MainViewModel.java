package com.santacruzinstruments.scicalibrator.ui.main;

import static com.santacruzinstruments.scicalibrator.nmea2000.Nmea2000.getNmea2000Instance;

import android.content.Context;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import com.santacruzinstruments.scicalibrator.nmea2000.Nmea2000;

public class MainViewModel extends ViewModel implements Nmea2000.N2KListener {
    MutableLiveData<Integer> twaCal = new MutableLiveData<>(0);
    MutableLiveData<String>  twa = new MutableLiveData<>("0=0+0");

    public void startNmea2000(Context context) {
        Nmea2000.Start(context, this);
    }

    LiveData<Integer> getTwaCal(){
        return twaCal;
    }

    public void setTwaCal(int twaCal) {
        this.twaCal.postValue(twaCal);
    }

    public void submitTwaCal() {
        if( twaCal.getValue() != null)
            getNmea2000Instance().sendTwaCal(twaCal.getValue());
    }

    LiveData<String> getTwa(){
        return twa;
    }

    @Override
    public void onTwa(double twaDeg) {
    }

    @Override
    public void onTwaCal(double twaCalDeg) {

    }
}