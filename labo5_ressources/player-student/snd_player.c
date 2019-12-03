#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <alchemy/task.h>
#include <alchemy/timer.h>
#include <alchemy/event.h>

#include "io_utils.h"

#define SND_PERIOD     1
//#define MASK_SW0       0x1

struct snd_task_args {
    int snd_fd;
    struct wav_file wf;
};

RT_TASK snd_rt_task;
RT_TASK chrono_rt_task;
RT_TASK controlButton_rt_task;
RT_TASK playPause_rt_task;

RT_EVENT playPause_rt_event;

void playPause_task(void *cookie)
{
        unsigned int mask_ret;
        rt_event_create(&playPause_rt_event,"playPauseFlag",0,EV_PRIO);
        do{
                rt_event_wait(&playPause_rt_event,0x1,&mask_ret,EV_ANY,TM_INFINITE);
                printf("event1 (Pause)\n");
                rt_event_clear(&playPause_rt_event,0x1,&mask_ret);
                rt_event_wait(&playPause_rt_event,0x1,&mask_ret,EV_ANY,TM_INFINITE);
                printf("event2 (Play)\n");
                rt_event_clear(&playPause_rt_event,0x1,&mask_ret);

        }while(1);
}

void controlButton_task(void *cookie)
{


        int err = rt_task_set_periodic(NULL, TM_NOW,  10000000L);
        do{
                if(keys(cookie) == 0xe)
                {
                        printf("TEST\n");
                        rt_event_signal(&playPause_rt_event,0x1);
                }
                //printf("switch 0 value : 0x%x\n", switches(cookie) & SW0);
                printf("keys value: 0x%x\n", keys(cookie));
                rt_task_wait_period(NULL);
        }while(1);
}

void chrono_task(void *cookie)
{
        int err = rt_task_set_periodic(NULL, TM_NOW,  10000000L);
        unsigned minute = 0;
        unsigned seconds = 0;
        unsigned centieme = 0;
        display_time(cookie, minute, seconds, centieme);
        do {
            centieme++;
            if(centieme == 100)
            {
                    centieme = 0;
                    seconds++;
                    if(seconds == 60)
                    {
                            seconds = 0;
                            minute++;
                            if(minute == 99)
                            {
                                    minute = 0;
                            }
                    }
            }
            rt_task_wait_period(NULL);
            display_time(cookie, minute, seconds, centieme);
    } while (1); //TODO : Controller fin musique et stop ?
}

void snd_task(void *cookie)
{
    struct snd_task_args *args = (struct snd_task_args*)cookie;
    size_t to_write = args->wf.wh.data_size;
    ssize_t write_ret;
    void *audio_datas_p = args->wf.audio_datas;

    //RTIME period = (((RTIME)SND_PERIOD)*((RTIME)MS));
    int err = rt_task_set_periodic(NULL, TM_NOW,  1000000L);

    do {
        write_ret = write(args->snd_fd, audio_datas_p, to_write);
        if (write_ret < 0) {
            rt_printf("Error writing to sound driver\n");
            break;
        }

        to_write -= write_ret;
        audio_datas_p += write_ret;

        rt_task_wait_period(NULL);
    } while (to_write);

}

int
main(int argc, char *argv[])
{
    int rtsnd_fd;
    int audio_fd;
    struct snd_task_args args;
    void *ioctrls;

    if (argc < 2) {
        fprintf(stderr, "Please provide an audio file\n");
        exit(EXIT_FAILURE);
    }

    /* Ouverture du driver RTDM */
    rtsnd_fd = open("/dev/rtdm/snd", O_RDWR);
    if (rtsnd_fd < 0) {
        perror("Opening /dev/rtdm/snd");
        exit(EXIT_FAILURE);
    }

    args.snd_fd = rtsnd_fd;

    ioctrls = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, rtsnd_fd, 0);
    if (ioctrls == MAP_FAILED) {
        perror("Mapping real-time sound file descriptor");
        exit(EXIT_FAILURE);
    }

    /* Ouverture du fichier audio */
    audio_fd = open(argv[1], O_RDONLY);
    if (audio_fd < 0) {
        perror("Opening audio file");
        exit(EXIT_FAILURE);
    }

    /* Lecture de l'entête du fichier wav. Valide au passage que le
     * fichier est bien au format wav */
    if (parse_wav_header(audio_fd, &(args.wf.wh))) {
        exit(EXIT_FAILURE);
    }

    printf("Successfully parsed wav file...\n");

    args.wf.audio_datas = malloc(args.wf.wh.data_size);
    if (args.wf.audio_datas == NULL) {
        perror("Allocating for audio data");
        exit(EXIT_FAILURE);
    }

    /* Copie des données audio du fichier wav */
    if (copy_wav_data(audio_fd, &(args.wf))) {
        fprintf(stderr, "Error copying audio data\n");
        exit(EXIT_FAILURE);
    }

    printf("Successfully copied audio data from wav file...\n");

    mlockall(MCL_CURRENT | MCL_FUTURE);

    /* EXEMPLE d'utilisation des différentes fonctions en lien avec les
     * entrées/sorties */
    //printf("switch 0 value : 0x%x\n", switches(ioctrls) & SW0);
    printf("keys value: 0x%x\n", keys(ioctrls));
    //display_time(ioctrls, 1, 2, 3);
    //set_volume_leds(ioctrls, VOL_MIDDLE);
    /* Fin de l'exemple */


    printf("Playing...\n");
    rt_task_spawn(&snd_rt_task, "snd_task", 0, 99, T_JOINABLE, snd_task, &args);
    rt_task_spawn(&chrono_rt_task,"chrono_task",0,99,T_JOINABLE,chrono_task, ioctrls);
    rt_task_spawn(&controlButton_rt_task,"control_button_task",0,99,T_JOINABLE,controlButton_task,ioctrls);
    rt_task_spawn(&playPause_rt_task,"play_pause_task",0,99,T_JOINABLE,playPause_task,ioctrls); //TODO : Peut être enlever
    rt_task_join(&snd_rt_task);
    rt_task_join(&chrono_rt_task);
    rt_task_join(&controlButton_rt_task);
    rt_task_join(&playPause_rt_task);

    close(rtsnd_fd);
    if (munmap(ioctrls, 4096) == -1) {
        perror("Unmapping");
        exit(EXIT_FAILURE);
    }
    free(args.wf.audio_datas);
    munlockall();

    return EXIT_SUCCESS;
}
