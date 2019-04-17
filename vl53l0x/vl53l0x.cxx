#include <nuttx/config.h>
#include <errno.h>
#include <fcntl.h>
#include <nuttx/sensors/vl53l0x.h>
#include <platform/cxxinitialize.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>

#define STATE_RUN  0x01
#define STATE_STOP 0x02
#define STATE_LOG  0x04


static const char* vl53l0xTypeString(unsigned type)
{
  if (VL53L0X_PRODUCT_TYPE_VL53L0X == type) {
    return "VL53L0X";
  }
  if (VL53L0X_PRODUCT_TYPE_VL53L1X == type) {
    return "VL53L1X";
  }
  return "------";
}

static const char* vl53l0xModeString(unsigned mode)
{
  if (VL53L0X_PROFILE_MODE_SINGLE == mode) {
    return "single";
  }
  if (VL53L0X_PROFILE_MODE_CONT == mode) {
    return "continuous";
  }
  return "-----";
}

static const char progress()
{
  static char p[] = "-\\|/";
  static uint8_t i = 0;

  return p[++i & 0x03];
}

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
    fcntl(0, F_SETFL, fdFlags | O_NONBLOCK);
    VL53L0X_data_t data;
    clock_t holdoff = clock();
    for ( uint8_t state = STATE_RUN; state & STATE_RUN; ) {
      uint8_t key=0;
      if (1 == read(0, &key, 1)) {
        VL53L0X_ioctl_t io;
        int rc;

        switch (key) {
          case '0':
          case '1':
          case '2':
          case '3':
            io.profile.id = key - '0';
            io.profile.mode = VL53L0X_PROFILE_MODE_SINGLE;
            rc = ioctl(fd, VL53L0X_IOCTL_PROFILE | VL53L0X_IOCTL_WRITE, reinterpret_cast<uintptr_t>(&io));
            printf("Set profile to %d: %d (%d)\n", key - '0', rc, io.ret);
            state |= STATE_STOP;
            break;

          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            io.profile.id = key - '4';
            rc = ioctl(fd, VL53L0X_IOCTL_PROFILE | VL53L0X_IOCTL_READ, reinterpret_cast<uintptr_t>(&io));
            if (rc) {
              printf("Getting profile failed: %d\n", rc);
            } else {
              printf("Profile %d, %s\n", io.profile.id, vl53l0xModeString(io.profile.mode));
              printf("   sigma final range        : %d - %08X\n", (io.profile.param.checkEnable & VL53L0X_CHECK_ENABLE_SIGMA_FINAL_RANGE ? 1 : 0), io.profile.param.sigmaFinalRange);
              printf("   signal rate final range  : %d - %08X\n", (io.profile.param.checkEnable & VL53L0X_CHECK_ENABLE_SIGNAL_RATE_FINAL_RANGE ? 1 : 0), io.profile.param.signalRateFinalRange);
              printf("   range ignore threshold   : %d - %08X\n", (io.profile.param.checkEnable & VL53L0X_CHECK_ENABLE_RANGE_IGN_THRESHOLD ? 1 : 0), io.profile.param.rangeIgnoreThreshold);
              printf("   measurement timing budget: %d\n", io.profile.param.measurementTimingBudget);
              printf("   vcsel period range       : %d / %d\n", io.profile.param.vcselPeriodPreRange, io.profile.param.vcselPeriodFinalRange);
            }
            break;

          case 'i':
            rc = ioctl(fd,  VL53L0X_IOCTL_INFO | VL53L0X_IOCTL_READ, reinterpret_cast<uintptr_t>(&io));
            if (rc) {
              printf("Getting info failed: %d\n", rc); 
            } else {
              printf("Device: %s  (%s.%d %d.%d)\n", io.info.name, vl53l0xTypeString(io.info.productType), io.info.productType, io.info.versionMajor, io.info.versionMinor);
              printf("  type: %s\n", io.info.type);
              printf("    id: %s\n", io.info.id);
            }
            break;

          case 'l':
            state ^= STATE_LOG;
            break;

          case 'q':
          case 'Q':
            state ^= STATE_RUN;
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
            char buf[16];
            if (4 != data.status) {
              sprintf(buf, "%4d", data.range);
            } else {
              sprintf(buf, "----");
            }
            if (state & STATE_LOG) {
              printf("%6ld: %d - %s (%4d)\n", clock(), data.status, buf, data.maxRange);
            } else {
              printf("%c %d - %s\r", progress(), data.status, buf);
              fflush(stdout);
            }
          } else {
            fprintf(stderr, "attempted to read %d bytes - received %d\n", sizeof(data), rc);
            sleep(3);
          }
          holdoff = clock() + CLOCKS_PER_SEC / 10;
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
