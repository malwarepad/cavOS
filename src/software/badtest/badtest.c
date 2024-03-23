#include <stddef.h>
#include <stdint.h>

// just something that runs :)

#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

uint8_t inportb(uint16_t _port) {
  uint8_t rv;
  __asm__ __volatile__("inb %1, %0" : "=a"(rv) : "dN"(_port));
  return rv;
}

void outportb(uint16_t _port, uint8_t _data) {
  __asm__ __volatile__("outb %1, %0" : : "dN"(_port), "a"(_data));
}

int serial_rcvd(int device) { return inportb(device + 5) & 1; }

char serial_recv(int device) {
  while (serial_rcvd(device) == 0)
    ;
  return inportb(device);
}

char serial_recv_async(int device) { return inportb(device); }

int serial_transmit_empty(int device) { return inportb(device + 5) & 0x20; }

void serial_send(int device, char out) {
  while (serial_transmit_empty(device) == 0)
    ;
  outportb(device, out);
}

#define __NR_write 1
size_t my_write(int fd, const void *buf, size_t size) {
  size_t ret;
  asm volatile("syscall"
               : "=a"(ret)
               //                 EDI      RSI       RDX
               : "0"(__NR_write), "D"(fd), "S"(buf), "d"(size)
               : "memory");
  return ret;
}
#define __NR_exit 60
void my_exit(int code) {
  asm volatile("syscall" ::"a"(__NR_exit), "D"(code) : "memory");
}

uint32_t strlength(char *ch) {
  uint32_t i = 0; // Changed counter to 0
  while (ch[i++])
    ;
  return i - 1; // Changed counter to i instead of i--
}

int atoi(char *str) {
  int res = 0;

  for (int i = 0; str[i] != '\0'; ++i)
    res = res * 10 + str[i] - '0';

  return res;
}

void reverse(char s[]) {
  uint64_t i, j;
  char     c;

  for (i = 0, j = strlength(s) - 1; i < j; i++, j--) {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

void itoa(uint64_t n, char s[]) {
  uint64_t i; // , sign

  // if ((sign = n) < 0) /* record sign */
  //   n = -n;           /* make n positive */
  i = 0;
  do {                     /* generate digits in reverse order */
    s[i++] = n % 10 + '0'; /* get next digit */
  } while ((n /= 10) > 0); /* delete it */
  // if (sign < 0)
  //   s[i++] = '-';
  s[i] = '\0';
  reverse(s);
}

void printNum(uint64_t num) {
  char frd[50] = {0};
  itoa((uint64_t)(num), frd);
  my_write(1, frd, strlength(frd));
}

int main(int argc, char **argv) {
  char *msg =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam hendrerit "
      "nulla eget imperdiet varius. Cras at accumsan orci, non sodales eros. "
      "Aenean tincidunt tellus justo, eu vulputate dui eleifend sit amet. Sed "
      "eu nunc volutpat, scelerisque libero in, euismod enim. Orci varius "
      "natoque penatibus et magnis dis parturient montes, nascetur ridiculus "
      "mus. Maecenas efficitur accumsan enim, in tempus justo dignissim ac. "
      "Donec aliquam dignissim volutpat. Praesent mattis dui ac odio mattis "
      "luctus. Sed condimentum consectetur tempus. Sed vestibulum erat eget "
      "pellentesque pharetra. Proin et luctus metus.\n"
      "\n"
      "Donec malesuada ipsum tellus, eu consequat odio ullamcorper non. Proin "
      "cursus nec dolor vel porta. Aenean ac velit nisi. Proin nibh libero, "
      "tincidunt nec blandit nec, porttitor quis massa. Praesent pellentesque "
      "lectus eu orci malesuada, quis ultrices tellus malesuada. Curabitur eu "
      "tristique diam. Proin finibus nisi ligula, ut posuere diam elementum "
      "a.\n"
      "\n"
      "Praesent luctus venenatis dui eget pulvinar. Praesent justo urna, "
      "convallis eu fermentum vitae, lobortis id quam. Aliquam non dolor "
      "finibus, laoreet mauris consectetur, maximus neque. Aliquam sit amet "
      "commodo enim. Sed tempor pulvinar felis, sit amet faucibus neque "
      "fermentum sed. Nulla et euismod nunc, at bibendum odio. Nam neque "
      "justo, sagittis ut nulla hendrerit, semper varius nisi. Donec nec "
      "mattis neque. Cras ultrices ipsum sed lectus sollicitudin pellentesque. "
      "Duis maximus ligula magna. Sed aliquet dictum mi.\n"
      "\n"
      "Vestibulum sit amet dolor eu turpis venenatis vulputate. Vestibulum sed "
      "aliquet libero. Fusce vestibulum nisi turpis, ac molestie est sagittis "
      "non. Nunc eu enim odio. Maecenas id felis neque. Cras luctus metus vel "
      "orci tempor tempor. Sed nec erat lacus. Sed ultricies varius elit ac "
      "blandit. Vestibulum ut rutrum lorem. Fusce varius, dolor at malesuada "
      "pretium, nulla nisi tempus dolor, in vestibulum sem ante efficitur "
      "erat. Phasellus sed velit id justo egestas porta ut et massa.\n"
      "\n"
      "Aenean purus felis, semper a dapibus id, posuere ac est. Morbi in "
      "pulvinar ligula. Sed ullamcorper sapien nec nulla sollicitudin "
      "sollicitudin. In viverra enim quis turpis facilisis, non mollis metus "
      "mollis. Donec sed eleifend mi. Duis ultricies odio ex, ultrices mattis "
      "ipsum tempor et. Morbi rhoncus nulla sit amet arcu pulvinar bibendum. "
      "Integer sed tellus faucibus, feugiat magna ac, luctus arcu. Phasellus "
      "ultrices finibus nisi, in rutrum ante eleifend ac. Duis mi sapien, "
      "rhoncus ac enim id, molestie imperdiet sem. Ut id tortor in ligula "
      "viverra dignissim. Donec purus risus, blandit sed est nec, ullamcorper "
      "vestibulum ipsum. Etiam pharetra feugiat facilisis. \n";

  // for (int i = 0; i < strlength(msg); i++)
  //   serial_send(COM1, msg[i]);

  // int out = my_write(1, msg, strlength(msg));
  // printNum(out);

  float hehe = 0.755;
  hehe += 1;
  hehe /= 2;

  char *nl = "\n";
  for (int i = 0; i < argc; i++) {
    my_write(1, argv[i], strlength(argv[i]));
    my_write(1, nl, 1);
  }

  // printNum(argv);

  // asm volatile("cli");

  // while (1) {
  // }
}

void _start_c(uint64_t rsp) {
  main(*(int *)(rsp), rsp + 8);
  my_exit(0);
}