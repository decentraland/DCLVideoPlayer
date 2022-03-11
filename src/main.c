//#include "decoder.h"
#include "player.h"
#include "logger.h"

int runDecoder()
{
    logging("Running Decoder\n");

    DecoderContext* dectx = create("http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4");
    if (dectx == NULL)
    {
        logging("error on create");
        return -1;
    }

    int i = 50000000;
    while(i >= 0)
    {
        ProcessOutput processOutput;
        int res = process_frame(dectx, &processOutput);

        if (processOutput.videoFrame) {
            av_frame_free(&processOutput.videoFrame);
        }

        if (processOutput.audioFrame) {
            av_frame_free(&processOutput.audioFrame);
        }

        if (res >= 0) {
            --i;
        }
    }

    destroy(dectx);
    return 0;
}

int main()
{
    logging("Hello world\n");

    runDecoder();
    return 0;
}