// Sample demo for QuickLZ 1.5.x

// Remember to define QLZ_COMPRESSION_LEVEL and QLZ_STREAMING_MODE to the same values for the compressor and decompressor

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <fcntl.h>

#include "quicklz.h"

int main(int argc, char* argv[])
{
    bool compress = true; // compression as default
    uint32_t frame_size = 400*240*3; // top screen as default
    uint32_t comp_size;
    char* frame_prev;
    char* frame_curr;
    char* frame_comp;
    
    // set binary mode for stdin / stdout
    #ifdef _WIN32
    setmode(fileno(stdin), O_BINARY);
    setmode(fileno(stdout), O_BINARY);
    #endif
    // freopen(NULL, "rb", stdin);
    // freopen(NULL, "wb", stdout);
    
    // instructions / title
    fprintf(stderr, "--> BootAnim9 Compressor v0.0.1 <--\n");
    fprintf(stderr, "using QuickLZ, http://www.quicklz.com/\n\n");
    if (argc <= 1) {
        fprintf(stderr, "Usage: ban9comp [c|d] [-t|-b] < input.bin > output.bin\n");
        fprintf(stderr, " [c]  -> compress data\n");
        fprintf(stderr, " [d]  -> decompress data\n");
        fprintf(stderr, " [-t] -> top screen data\n");
        fprintf(stderr, " [-b] -> bottom screen data\n\n");
        return 0;
    }
    
    // process command line arguments
    while (--argc) {
        argv++;
        if (strncmp(*argv, "d", 2) == 0) compress = false;
        else if (strncmp(*argv, "-b", 2) == 0) frame_size = 320*240*3;
    }
    frame_prev = (char*) malloc(frame_size);
    frame_curr = (char*) malloc(frame_size);
    frame_comp = (char*) malloc(frame_size + 400); // + 400 for safety
    if (!frame_prev || !frame_curr || !frame_comp) {
        fprintf(stderr, "out of memory\n");
        return 0;
    }
    memset(frame_prev, 0, frame_size); // set prev frame zero
    
    uint32_t total_fsize = 0;
    uint32_t total_csize = 0;
    int n = 0;
    if (compress) { // compression routine
        qlz_state_compress *state_compress = (qlz_state_compress*) malloc(sizeof(qlz_state_compress));
        memset(state_compress, 0, sizeof(qlz_state_compress)); 
        while (fread(frame_curr, 1, frame_size, stdin) > 0) {
            // inter frame delta
            for (uint32_t i = 0; i < frame_size; i++) {
                char delta = frame_curr[i] - frame_prev[i];
                frame_prev[i] = frame_curr[i];
                frame_curr[i] = delta;
            }
            // actual compression
            comp_size = qlz_compress(frame_curr, frame_comp, frame_size, state_compress);
            fwrite(frame_comp, 1, comp_size, stdout);
            // statistics..
            total_fsize += frame_size;
            total_csize += comp_size;
            fprintf(stderr, "frame #%03i, %iB -> %iB...\r", n++, total_fsize, total_csize);
        }
        fprintf(stderr, "%i frames total, %iB -> %iB\n", n, total_fsize, total_csize); 
    } else { // decompression routine
        qlz_state_decompress *state_decompress = (qlz_state_decompress*) malloc(sizeof(qlz_state_decompress));
        memset(state_decompress, 0, sizeof(qlz_state_decompress)); 
        while (fread(frame_comp, 1, 9, stdin) == 9) {
            // decompression of data
            comp_size = qlz_size_compressed(frame_comp);
            fread(frame_comp + 9, 1, comp_size - 9, stdin);
            if (qlz_size_decompressed(frame_comp) != frame_size) {
                fprintf(stderr, "\nbad data or wrong parameter\n");
                return 0;
            }
            qlz_decompress(frame_comp, frame_curr, state_decompress);
            fwrite(frame_curr, 1, frame_size, stdout);
            // undo inter frame delta
            for (uint32_t i = 0; i < frame_size; i++)
                frame_curr[i] += frame_prev[i];
            // copy to prev frame buffer
            memcpy(frame_prev, frame_curr, frame_size);
            // statistics..
            total_fsize += frame_size;
            total_csize += comp_size;
            fprintf(stderr, "frame #%03i, %iB -> %iB...\r", n++, total_csize, total_fsize);
        }
        fprintf(stderr, "%i frames total, %iB -> %iB\n", n, total_csize, total_fsize); 
    }
    fprintf(stderr, "Done!\n\n");
    
    return 0;
}
