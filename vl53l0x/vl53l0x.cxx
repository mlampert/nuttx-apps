#include <nuttx/config.h>
#include <errno.h>
#include <fcntl.h>
#include <nuttx/sensors/vl53l0x.h>
#include <platform/cxxinitialize.h>
#include <stdio.h>

extern "C"
{

#if defined(BUILD_MODULE)
int main(int argc, FAR char *argv[])
#else
int vl53l0x_main(int argc, char *argv[])
#endif
{
  int fd = open("/dev/vl53l0x0", O_RDONLY);
  if (fd > 0) {
    int fdFlags = fcntl(0, F_GETFL, 0);
    printf("flags: %08X\n", fdFlags);
    fcntl(0, F_SETFL, fdFlags | O_NONBLOCK);
    VL53L0X_data_t data;
    for (uint8_t key=0; read(0, &key, 1) != 1; ) {
      for (int i=0; i<10; ++i) {
        int rc = read(fd, &data, sizeof(data));
        if (rc == sizeof(data)) {
          if (0 == data.status) {
            printf("%2d: %d - %4d (%4d)\n", i, data.status, data.range, data.maxRange);
          } else {
            printf("%2d: %d - ---- (%4d)\n", i, data.status, /*data.range,*/ data.maxRange);
          }
        } else {
          fprintf(stderr, "%2d: attempted to read %d bytes - received %d\n", i, sizeof(data), rc);
        }
      }
    }
    fcntl(0, F_SETFL, fdFlags);
    close(fd);
  } else {
    fprintf(stderr, "failed to open device: %d\n", errno);
  }
  return 0;
}

}
