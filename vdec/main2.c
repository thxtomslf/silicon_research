#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include "main.h"  // Предполагается, что этот заголовочный файл содержит необходимые объявления

// Функция для получения текущего времени в виде строки
void getCurrentTimeString(char* buffer, size_t bufferSize) {
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, bufferSize, "%H:%M:%S", timeinfo);
}

// Поток OSD для наложения текущего времени
void* osdTimeOverlayThread(void* arg) {
    struct _fbg* fbg = (struct _fbg*)arg;
    char timeString[9];  // Формат времени HH:MM:SS

    while (1) {
        getCurrentTimeString(timeString, sizeof(timeString));
        fbg_clear(fbg, 0);  // Очистка экрана
        fbg_write(fbg, timeString, 10, 10);  // Наложение времени в позиции (10, 10)
        fbg_flip(fbg);  // Обновление дисплея
        usleep(1000000);  // Обновление каждую секунду
    }
    return NULL;
}

int main(int argc, const char* argv[]) {
    // Инициализация устройств вывода и декодера
    VO_DEV vo_device_id = 0;
    VO_LAYER vo_layer_id = 0;
    VO_CHN vo_channel_id = 0;
    VDEC_CHN vdec_channel_id = 0;

    // Настройка OSD
    struct _fbg* fbg = fbg_fbdevSetup(NULL, 0, 1280, 720);  // Пример разрешения
    if (!fbg) {
        fprintf(stderr, "Ошибка: не удалось инициализировать OSD\n");
        return 1;
    }

    pthread_t osd_thread;
    if (pthread_create(&osd_thread, NULL, osdTimeOverlayThread, fbg) != 0) {
        fprintf(stderr, "Ошибка: не удалось создать поток OSD\n");
        fbg_close(fbg);
        return 1;
    }

    // Создание сокета для приема видеопотока
    int socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd < 0) {
        perror("Ошибка: не удалось создать сокет");
        fbg_close(fbg);
        return 1;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(5600);  // Пример порта
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Ошибка: не удалось привязать сокет");
        close(socket_fd);
        fbg_close(fbg);
        return 1;
    }

    // Установка неблокирующего режима
    if (fcntl(socket_fd, F_SETFL, O_NONBLOCK) == -1) {
        perror("Ошибка: не удалось установить неблокирующий режим");
        close(socket_fd);
        fbg_close(fbg);
        return 1;
    }

    // Буферы для приема и декодирования
    uint8_t* rx_buffer = malloc(1024 * 1024);
    if (!rx_buffer) {
        fprintf(stderr, "Ошибка: не удалось выделить память для rx_buffer\n");
        close(socket_fd);
        fbg_close(fbg);
        return 1;
    }

    uint8_t* nal_buffer = malloc(1024 * 1024);
    if (!nal_buffer) {
        fprintf(stderr, "Ошибка: не удалось выделить память для nal_buffer\n");
        free(rx_buffer);
        close(socket_fd);
        fbg_close(fbg);
        return 1;
    }

    // Основной цикл обработки видеопотока
    while (1) {
        // Прием данных из сокета
        int rx_size = recv(socket_fd, rx_buffer, 1024 * 1024, 0);
        if (rx_size <= 0) {
            if (rx_size < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("Ошибка при получении данных");
            }
            usleep(1000);  // Ожидание, если данных нет
            continue;
        }

        // Декодирование полученного кадра
        uint32_t nal_size = 0;
        uint8_t* nal_data = decode_frame(rx_buffer, rx_size, 0, nal_buffer, &nal_size);
        if (!nal_data || nal_size == 0) {
            fprintf(stderr, "Ошибка: декодирование кадра не удалось\n");
            continue;  // Пропуск, если декодирование не удалось
        }
	// Отправка декодированного кадра в видеодекодер
        VDEC_STREAM_S stream;
        memset(&stream, 0, sizeof(stream));
        stream.pu8Addr = nal_data;
        stream.u32Len = nal_size;
        stream.bEndOfFrame = HI_TRUE;

        int ret = HI_MPI_VDEC_SendStream(vdec_channel_id, &stream, -1);
        if (ret != HI_SUCCESS) {
            fprintf(stderr, "Ошибка: не удалось отправить данные в VDEC\n");
            continue;
        }

        // Здесь можно добавить дополнительную обработку, если необходимо

        // Обновление OSD с текущим временем
        // Это уже выполняется в отдельном потоке osdTimeOverlayThread
    }

    // Очистка ресурсов
    free(rx_buffer);
    free(nal_buffer);
    pthread_join(osd_thread, NULL);
    fbg_close(fbg);
    close(socket_fd);
    return 0;
}
