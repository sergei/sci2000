package com.santacruzinstruments.scicalibrator.ui.main;

import static com.santacruzinstruments.scicalibrator.nmea2000.Nmea2000.getNmea2000Instance;

import static java.lang.Math.abs;

import android.content.Context;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

import com.santacruzinstruments.scicalibrator.nmea2000.ItemType;
import com.santacruzinstruments.scicalibrator.nmea2000.Nmea2000;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public class MainViewModel extends ViewModel implements Nmea2000.N2KListener {
    private static final String INVALID_VALUE = "...";

    MutableLiveData<Boolean> isConnected = new MutableLiveData<>();

    static class Calibratable{
        final String name;
        final boolean isDegree;
        MutableLiveData<Integer> cal = new MutableLiveData<>(0);
        MutableLiveData<String> value = new MutableLiveData<>(INVALID_VALUE);
        boolean gotCal = false;
        double currCal = 0;

        Calibratable(String name, boolean isDegree) {
            this.name = name;
            this.isDegree = isDegree;
        }
    }

    public Map<ItemType, Calibratable> getItems() {
        return items;
    }

    private final HashMap<ItemType, Calibratable> items = new HashMap<>();
    public List<ItemType> getItemsList() {
        return new LinkedList<>(items.keySet());
    }

    public MainViewModel() {
        items.put(ItemType.AWA, new Calibratable("AWA", true));
        items.put(ItemType.AWS, new Calibratable("AWS", false));
    }

    @Override
    public void OnConnectionStatus(boolean connected) {
        isConnected.postValue(connected);
    }

    public LiveData<Boolean> getIsConnected() {
        return isConnected;
    }

    public void startNmea2000(Context context) {
        Nmea2000.Start(context, this);
    }

    LiveData<Integer> getCal(ItemType item){
        Calibratable c = items.get(item);
        assert c != null;
        return c.cal;
    }

    public void setCal(ItemType item, int calValue) {
        Calibratable c = items.get(item);
        assert c != null;
        c.cal.postValue(calValue);
    }

    public void submitCal(ItemType item) {
        Calibratable c = items.get(item);

        assert c != null;
        if( c.cal.getValue() != null) {
            getNmea2000Instance().sendCal(item, c.cal.getValue());
            c.gotCal = false;
        }
    }

    LiveData<String> getValue(ItemType item){
        Calibratable c = items.get(item);
        assert c != null;
        return c.value;
    }

    @Override
    public void onRcvdValue(ItemType item, double value) {
        Calibratable c = items.get(item);
        assert c != null;
        String sign = c.currCal > 0 ? "+" : "-";
        if ( c.isDegree ){
            double nonCalVal = value - c.currCal;
            if ( c.gotCal ){
                c.value.postValue(String.format(Locale.getDefault(), "%.1f° = %.1f° %s %.1f°", value, nonCalVal, sign, abs(c.currCal)));
            }else{
                c.value.postValue(INVALID_VALUE);
            }
        }else{
            double ratio = 1 + c.currCal / 100.;
            double nonCalVal = value / ratio;
            if ( c.gotCal ){
                c.value.postValue(String.format(Locale.getDefault(), "%.1f = %.1f %s %.1f %%", value, nonCalVal, sign, abs(c.currCal)));
            }else{
                c.value.postValue(INVALID_VALUE);
            }
        }
    }

    @Override
    public void onRcvdCalibration(ItemType item, double calValue) {
        Calibratable c = items.get(item);
        assert c != null;
        c.gotCal = true;
        c.currCal = calValue;
        c.cal.postValue((int) calValue);
    }

}