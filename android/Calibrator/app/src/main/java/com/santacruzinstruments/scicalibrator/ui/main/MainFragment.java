package com.santacruzinstruments.scicalibrator.ui.main;

import androidx.lifecycle.ViewModelProvider;

import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.TextView;

import com.santacruzinstruments.scicalibrator.R;
import com.santacruzinstruments.scicalibrator.databinding.FragmentMainBinding;
import com.santacruzinstruments.scicalibrator.nmea2000.ItemType;

import java.util.List;

public class MainFragment extends Fragment {

    static class CalItemViewHolder extends RecyclerView.ViewHolder{
        TextView itemLabel;
        TextView itemValue;
        Button buttonCalibrate;
        Button buttonCancel;
        TextView calValue;
        SeekBar seekBar;
        public CalItemViewHolder(@NonNull View itemView) {
            super(itemView);

            itemLabel = itemView.findViewById(R.id.itemLabel);
            itemValue = itemView.findViewById(R.id.itemValue);
            buttonCalibrate = itemView.findViewById(R.id.buttonCalibrate);
            buttonCancel = itemView.findViewById(R.id.buttonCancel);
            calValue = itemView.findViewById(R.id.calValue);
            seekBar = itemView.findViewById(R.id.seekBar);
        }
    }

    class CalItemViewAdapter extends RecyclerView.Adapter<CalItemViewHolder>{

        private final MainViewModel viewModel;
        private final List<ItemType> calibratableItems;
        public CalItemViewAdapter(MainViewModel viewModel, List<ItemType> calibratableItems) {
            this.viewModel = viewModel;
            this.calibratableItems = calibratableItems;
        }

        @NonNull
        @Override
        public CalItemViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
            LayoutInflater inflater = LayoutInflater.from(parent.getContext());

            // Inflate the layout
            View calItemView = inflater.inflate(R.layout.cal_ietm, parent, false);
            return new CalItemViewHolder(calItemView);
        }

        @Override
        public void onBindViewHolder(@NonNull CalItemViewHolder holder, int position) {
            ItemType it = this.calibratableItems.get(position);
            final MainViewModel.Calibratable calibratable = this.viewModel.getItems().get(it);
            assert calibratable != null;
            holder.itemLabel.setText(calibratable.name);

            holder.buttonCalibrate.setOnClickListener(view -> {
                if( holder.seekBar.getVisibility() == View.VISIBLE){
                    holder.seekBar.setVisibility(View.GONE);
                    holder.buttonCancel.setVisibility(View.GONE);
                    holder.calValue.setVisibility(View.GONE);
                    ((Button)view).setText(R.string.calibrate);
                    mViewModel.submitCal(it);
                }else{
                    holder.seekBar.setVisibility(View.VISIBLE);
                    holder.buttonCancel.setVisibility(View.VISIBLE);
                    holder.calValue.setVisibility(View.VISIBLE);
                    ((Button)view).setText(R.string.submit);
                }
            });

            holder.buttonCancel.setOnClickListener(view -> {
                holder.seekBar.setVisibility(View.GONE);
                holder.calValue.setVisibility(View.GONE);
                holder.buttonCancel.setVisibility(View.GONE);
                holder.buttonCalibrate.setText(R.string.calibrate);
            });

            if ( calibratable.isDegree ){
                holder.seekBar.setMin(-45);
                holder.seekBar.setMax(45);
            }else{
                holder.seekBar.setMin(-50);
                holder.seekBar.setMax(50);
            }

            holder.seekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                @Override
                public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                    if ( fromUser ){
                        mViewModel.setCal(it, progress);
                    }
                }

                @Override
                public void onStartTrackingTouch(SeekBar seekBar) {

                }

                @Override
                public void onStopTrackingTouch(SeekBar seekBar) {

                }
            });

            mViewModel.getCal(it).observe(getViewLifecycleOwner(),
                    val -> {
                        holder.calValue.setText(getString(R.string.cal_angle, val));
                        holder.seekBar.setProgress(val);
                    });

            mViewModel.getValue(it).observe(getViewLifecycleOwner(),
                    val -> holder.itemValue.setText(val));
        }

        @Override
        public int getItemCount() {
            return this.calibratableItems.size();
        }
    }


    private FragmentMainBinding binding;
    private MainViewModel mViewModel;
    private List<ItemType> mCalibratableItems;
    CalItemViewAdapter mCalItemViewAdapter;

    public static MainFragment newInstance() {
        return new MainFragment();
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mViewModel = new ViewModelProvider(this).get(MainViewModel.class);
        mViewModel.startNmea2000(getContext());
        mCalibratableItems = mViewModel.getItemsList();
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {

        binding = FragmentMainBinding.inflate(inflater, container, false);

        mViewModel.getIsConnected().observe(getViewLifecycleOwner(), isConnected -> {
            if ( isConnected ){
                binding.notConnectedIndicator.setVisibility(View.GONE);
                binding.recyclerView.setVisibility(View.VISIBLE);
            }else{
                binding.notConnectedIndicator.setVisibility(View.VISIBLE);
                binding.recyclerView.setVisibility(View.GONE);
            }
        });


        RecyclerView recyclerView = binding.recyclerView;
        mCalItemViewAdapter = new CalItemViewAdapter(mViewModel, mCalibratableItems);
        recyclerView.setAdapter(mCalItemViewAdapter);
        recyclerView.setLayoutManager( new LinearLayoutManager(getContext()));

        return binding.getRoot();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        binding = null;
    }
}