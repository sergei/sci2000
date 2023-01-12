package com.santacruzinstruments.scicalibrator.ui.main;

import androidx.lifecycle.ViewModelProvider;

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.SeekBar;

import com.santacruzinstruments.scicalibrator.R;
import com.santacruzinstruments.scicalibrator.databinding.FragmentMainBinding;

public class MainFragment extends Fragment {

    private FragmentMainBinding binding;
    private MainViewModel mViewModel;

    public static MainFragment newInstance() {
        return new MainFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mViewModel = new ViewModelProvider(this).get(MainViewModel.class);
        mViewModel.startNmea2000(getContext());
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {

        binding = FragmentMainBinding.inflate(inflater, container, false);

        // TWA
        binding.buttonTwa.setOnClickListener(view -> {
            if( binding.seekBarTwa.getVisibility() == View.VISIBLE){
                binding.seekBarTwa.setVisibility(View.GONE);
                binding.buttonTwaCancel.setVisibility(View.GONE);
                binding.twaCalValue.setVisibility(View.GONE);
                ((Button)view).setText(R.string.calibrate);
                mViewModel.submitAwaCal();
            }else{
                binding.seekBarTwa.setVisibility(View.VISIBLE);
                binding.buttonTwaCancel.setVisibility(View.VISIBLE);
                binding.twaCalValue.setVisibility(View.VISIBLE);
                ((Button)view).setText(R.string.submit);
            }
        });
        binding.buttonTwaCancel.setOnClickListener(view -> {
            binding.seekBarTwa.setVisibility(View.GONE);
            binding.buttonTwaCancel.setVisibility(View.GONE);
            binding.buttonTwa.setText(R.string.calibrate);
        });
        binding.seekBarTwa.setMin(-45);
        binding.seekBarTwa.setMax(45);
        binding.seekBarTwa.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                mViewModel.setAwaCal(progress);
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {

            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {

            }
        });

        mViewModel.getIsConnected().observe(getViewLifecycleOwner(), isConnected -> {
            if ( isConnected ){
                binding.notConnectedIndicator.setVisibility(View.GONE);
                binding.main.setVisibility(View.VISIBLE);
            }else{
                binding.notConnectedIndicator.setVisibility(View.VISIBLE);
                binding.main.setVisibility(View.GONE);
            }
        });

        mViewModel.getAwaCal().observe(getViewLifecycleOwner(),
                val -> binding.twaCalValue.setText(getString(R.string.cal_angle, val)));
        mViewModel.getAwa().observe(getViewLifecycleOwner(),
                val -> binding.twaValue.setText(val));

        return binding.getRoot();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }
}