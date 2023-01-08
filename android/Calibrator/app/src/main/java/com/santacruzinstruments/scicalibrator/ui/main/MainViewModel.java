package com.santacruzinstruments.scicalibrator.ui.main;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;

public class MainViewModel extends ViewModel {
    MutableLiveData<Integer> twaCal = new MutableLiveData<>(0);
    MutableLiveData<String>  twa = new MutableLiveData<>("0=0+0");

    LiveData<Integer> getTwaCal(){
        return twaCal;
    }

    public void setTwaCal(int twaCal) {
        this.twaCal.postValue(twaCal);
    }
    public void submitTwaCal() {
    }

    LiveData<String> getTwa(){
        return twa;
    }

}