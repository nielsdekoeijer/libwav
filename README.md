# Header Only Wave File IO 
I wrote my own wav file parser after being dissatisified with what I could find online. Not optimized. Heavy WIP.

## Support:
- I aim to be able to read most wav files into a f32 or f64 array, automatically normalized between [-1, 1].
- I aim to be able to write, but only f32 pcm (f32 => anything else? ffmpeg).
- No sample rate conversion
- WAV files are read into memory directly, no streaming api (i.e. if your wav is big consider using something else)


## Todo:
- Add support for reading non f32
- Add possibility to read with an offset, such that implicitly large files are supported
- Actually write unit tests to validate
