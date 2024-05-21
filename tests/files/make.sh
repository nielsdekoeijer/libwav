#!/bin/bash

# Define parameters
sampling_rates=(44100 48000 96000 22050)
bit_depths=(16 24 32 8)
channels=(1 2)

# Loop through parameters and create files
for rate in "${sampling_rates[@]}"; do
  for depth in "${bit_depths[@]}"; do
    for channel in "${channels[@]}"; do
      # Determine encoding based on bit depth
      if [ "$depth" -eq 32 ]; then
        encoding="float"
        filename="${rate}Hz_${depth}bit_float_${channel}ch.wav"
        sox -n -r $rate -b $depth -e $encoding -c $channel $filename synth 0.5 sine 440 vol 0.5
      elif [ "$depth" -eq 8 ]; then
        encoding="unsigned-integer"
        filename="${rate}Hz_${depth}bit_unsigned_${channel}ch.wav"
        sox -n -r $rate -b $depth -e $encoding -c $channel $filename synth 0.5 sine 440 vol 0.5
      else
        encoding="signed-integer"
        filename="${rate}Hz_${depth}bit_signed_${channel}ch.wav"
        sox -n -r $rate -b $depth -e $encoding -c $channel $filename synth 0.5 sine 440 vol 0.5
      fi
    done
  done
done

