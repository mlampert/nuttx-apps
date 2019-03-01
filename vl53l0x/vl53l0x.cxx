#include <nuttx/config.h>
#include <errno.h>
#include <fcntl.h>
#include <nuttx/sensors/vl53l0x.h>
#include <platform/cxxinitialize.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>

#define STATE_QUIT 0x01
#define STATE_STOP 0x02

extern "C"
{

#if defined(BUILD_MODULE)
int main(int argc, FAR char *argv[])
#else
int vl53l0x_main(int argc, char *argv[])
#endif
{
  uint8_t state = 0;
  int fd = open("/dev/vl53l0x0", O_RDONLY);
  if (fd > 0) {
    int fdFlags = fcntl(0, F_GETFL, 0);
    printf("flags: %08X\n", fdFlags);
    fcntl(0, F_SETFL, fdFlags | O_NONBLOCK);
    VL53L0X_data_t data;
    clock_t holdoff = clock();
    for ( ; ; ) {
      uint8_t key=0;
      if (1 == read(0, &key, 1)) {
        VL53L0X_ioctl_t io;
        int rc;

        switch (key) {
          case '0':
          case '1':
          case '2':
          case '3':
            io.val = key - '0';
            rc = ioctl(fd, VL53L0X_IOCTL_PROFILE | VL53L0X_IOCTL_WRITE, reinterpret_cast<uintptr_t>(&io));
            printf("Set profile to %d: %d (%d)\n", key - '0', rc, io.ret);
            state |= STATE_STOP;
            break;

          case 'd':
            rc = ioctl(fd, VL53L0X_IOCTL_DUMP_CONFIG, reinterpret_cast<uintptr_t>(&io));
            printf("Dump config: %d (%d)\n", rc, io.ret);
            state |= STATE_STOP;
            break;

          case 'q':
          case 'Q':
            state ^= STATE_QUIT;
            break;

          case 'r':
            rc = ioctl(fd, VL53L0X_IOCTL_RESET | VL53L0X_IOCTL_WRITE, reinterpret_cast<uintptr_t>(&io));
            printf("Reset: %d (%d)\n", rc, io.ret);
            state |= STATE_STOP;
            break;

          case 's':
            state ^= STATE_STOP;
            break;

          default:
            fprintf(stderr, "Unknown key: %c\n", key);
        }
      } else if (!(state & STATE_STOP)) {
        if (holdoff < clock()) {
          int rc = read(fd, &data, sizeof(data));
          if (rc == sizeof(data)) {
            if (0 == data.status) {
              printf("%6ld: %d - %4d (%4d)\n", clock(), data.status, data.range, data.maxRange);
            } else {
              printf("%6ld: %d - ---- (%4d)\n", clock(), data.status, /*data.range,*/ data.maxRange);
            }
          } else {
            fprintf(stderr, "attempted to read %d bytes - received %d\n", sizeof(data), rc);
          }
          holdoff = clock() + CLOCKS_PER_SEC / 4;
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
