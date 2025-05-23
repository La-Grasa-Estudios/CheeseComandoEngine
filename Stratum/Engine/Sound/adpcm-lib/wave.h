#ifndef WAVE_H
#define WAVE_H

// Information taken from online at:
// http://soundfile.sapp.org/doc/WaveFormat/

struct WAVEHeader
{
    char  chunkID[4] = {'R','I','F','F'};       // Contains the letters "RIFF" in ASCII form
    long  chunkSize = 0;                            // 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
    char  format[4]  = {'W','A','V','E'};       // Contains the letters "WAVE" in ASCII form

    char  subchunk1ID[4] = {'f','m','t',' '};   // Contains the letters "fmt " in ASCII form
    long  subchunk1Size  = 16;                  // 16 for PCM
    short audioFormat = 0;                          // PCM = 1 Values other than 1 indicate some form of compression
    short numChannels = 0;                          // Mono = 1, Stereo = 2, etc
    long  sampleRate = 0;                           // 8000, 44100, etc.
    long  byteRate = 0;                             // SampleRate * NumChannels * BitsPerSample/8
    short blockAlign = 0;                           // NumChannels * BitsPerSample/8
    short bitsPerSample = 0;                        // 8 bits = 8, 16 bits = 16, etc

    char  subchunk2ID[4] = {'d','a','t','a'};   // Contains the letters "data" in ASCII form
    long  subchunk2Size = 0;                        // This is the number of bytes in the data
};

bool isCorrectHeader(WAVEHeader &hdr);

#endif
