#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "io_utils.h"

static const unsigned char hex_digits[10] = {
    0x40, // 0
    0xf9, // 1
    0x24, // 2
    0x30, // 3
    0x19, // 4
    0x12, // 5
    0x02, // 6
    0x78, // 7
    0x00, // 8
    0x10, // 9
};

void display_time(void *ioctrl,
                  uint8_t minutes, uint8_t seconds, uint8_t centisecs)
{
    *((uint32_t*)(ioctrl + 0x70)) = hex_digits[minutes/10];
    *((uint32_t*)(ioctrl + 0x60)) = hex_digits[minutes%10];

    *((uint32_t*)(ioctrl + 0x50)) = hex_digits[seconds/10];
    *((uint32_t*)(ioctrl + 0x40)) = hex_digits[seconds%10];

    *((uint32_t*)(ioctrl + 0x30)) = hex_digits[centisecs/10];
    *((uint32_t*)(ioctrl + 0x20)) = hex_digits[centisecs%10];
}

uint32_t keys(void *ioctrl)
{
    return *((uint32_t*)(ioctrl + 0x90));
}

uint32_t switches(void *ioctrl)
{
    return *((uint32_t*)(ioctrl + 0xa0));
}

int is_pressed_now(uint32_t keys_before, uint32_t keys_now, uint8_t bit)
{
    uint32_t before_bit = keys_before & bit;
    uint32_t now_bit = keys_now & bit;

    if (!now_bit && before_bit) return 1;
    else return 0;
}

void set_volume_leds(void *ioctrl, uint8_t volume)
{
    uint32_t val = 0;

    for (size_t bit = VOL_MIN; bit < VOL_MAX && bit < volume; bit++) {
        val = (val >> 1) | (1 << (VOL_MAX-1));
    }

    *((uint32_t*)(ioctrl + 0x80)) = val;
}

int16_t map_volume(int16_t sample, uint8_t volume)
{
    float volume_ratio = (float)volume / VOL_MAX;
    return (int16_t)(sample * volume_ratio);
}

int parse_wav_header(int audio_fd, struct wav_header *wh)
{
    size_t header_size = sizeof(struct wav_header);
    ssize_t read_ret;

    /* Reposition file offset to start of file */
    lseek(audio_fd, 0, SEEK_SET);

    read_ret = read(audio_fd, wh, header_size);
    if (read_ret < header_size) {
        fprintf(stderr, "Couldn't read whole wav header.\n");
        return -1;
    }

    if (wh->file_blkID != WAV_FILE_BLKID) {
        fprintf(stderr, "[%s] Wrong file block ID. Found : 0x%X\n", __func__, wh->file_blkID);
        return -1;
    }

    if (wh->fmt_blkID != WAV_FMT_BLKID) {
        fprintf(stderr, "[%s] Wrong audio format block ID\n", __func__);
        return -1;
    }

    if (wh->data_blkID != WAV_DATA_BLKID) {
        fprintf(stderr, "[%s] Wrong data block ID\n", __func__);
        return -1;
    }

    return 0;
}

int copy_wav_data(int audio_fd, const struct wav_file *wf)
{
    size_t to_read = wf->wh.data_size;
    ssize_t read_ret;
    void *audio_datas_p = wf->audio_datas;

    lseek(audio_fd, 44, SEEK_SET);
    do {
        read_ret = read(audio_fd, audio_datas_p, to_read);
        if (read_ret < 0) {
            perror("Reading audio data from wav file");
            return -1;
        }
        else if (read_ret == 0 && to_read > 0) {
            fprintf(stderr, "Reached EOF but data remains to be read\n");
            return -1;
        }

        to_read -= read_ret;
        audio_datas_p += read_ret;

    } while (to_read);

    return 0;
}
