#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <stdint.h>

#define MS 1000000

#define WAV_FILE_BLKID    0x46464952  // "RIFF" (little-endian)
#define WAV_FMT_BLKID     0x20746D66  // "fmt " (little-endian)
#define WAV_DATA_BLKID    0x61746164  // "data" (little-endian)

/* Bit offsets for keys */
#define KEY0    0x1
#define KEY1    0x2
#define KEY2    0x4
#define KEY3    0x8

/* Bit offsets for switches */
#define SW0     1

#define PLAYING 0
#define PAUSED  1
#define STOPPED 2

#define VOL_MAX     10
#define VOL_MIN     0
#define VOL_MIDDLE  5

struct wav_header {
    /* WAVE file format declaration block */
    uint32_t file_blkID; // "RIFF"
    uint32_t file_size;
    uint32_t file_fmtID; // "WAVE"

    /* Audio format description block */
    uint32_t fmt_blkID; // "fmt"
    uint32_t blk_size;
    uint16_t audio_fmt;
    uint16_t num_chan;
    uint32_t freq; // In Hertz
    uint32_t bytes_per_sec;
    uint16_t bytes_per_blk;
    uint16_t bits_per_sample;

    /* Data block */
    uint32_t data_blkID; // "data"
    uint32_t data_size; // In bytes
};

struct wav_file {
    struct wav_header wh;
    void *audio_datas;
};

/* Affiche le temps sur les afficheurs 7 segments */
void display_time(void *ioctrl,
                  uint8_t minutes, uint8_t seconds, uint8_t centisecs);

/* Retourne l'état des boutons poussoirs (attention, ils sont actifs bas) */
uint32_t keys(void *ioctrl);

/* Retourne l'état des switchs */
uint32_t switches(void *ioctrl);

/* Retourne 1 ou 0 si un bouton viens d'être appuyé en fonction de l'état
 * précédant et l'état courant des boutons poussoirs */
int is_pressed_now(uint32_t keys_before, uint32_t keys_now, uint8_t bit);

/* Affiche le niveau de volume du son sur les leds */
void set_volume_leds(void *ioctrl, uint8_t volume);

/* Adapte le volume du son sur un échantillon donné.
 * L'adaptation est linéaire. Une meilleure façon serait
 * d'adapter de manière logarithmique pour un meilleur rendu
 * sonore */
int16_t map_volume(int16_t sample, uint8_t volume);

/* Lis et enregistre l'en-tête du fichier wav */
int parse_wav_header(int audio_fd, struct wav_header *wh);

/* Copie le contenu audio du fichier wav */
int copy_wav_data(int audio_fd, const struct wav_file *wf);

#endif // IO_UTILS_H
