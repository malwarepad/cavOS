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

#define __SYSCALL_LL_E(x) (x)
#define __SYSCALL_LL_O(x) (x)

static __inline long __syscall0(long n) {
  unsigned long ret;
  __asm__ __volatile__("syscall" : "=a"(ret) : "a"(n) : "rcx", "r11", "memory");
  return ret;
}

static __inline long __syscall1(long n, long a1) {
  unsigned long ret;
  __asm__ __volatile__("syscall"
                       : "=a"(ret)
                       : "a"(n), "D"(a1)
                       : "rcx", "r11", "memory");
  return ret;
}

static __inline long __syscall2(long n, long a1, long a2) {
  unsigned long ret;
  __asm__ __volatile__("syscall"
                       : "=a"(ret)
                       : "a"(n), "D"(a1), "S"(a2)
                       : "rcx", "r11", "memory");
  return ret;
}

static __inline long __syscall3(long n, long a1, long a2, long a3) {
  unsigned long ret;
  __asm__ __volatile__("syscall"
                       : "=a"(ret)
                       : "a"(n), "D"(a1), "S"(a2), "d"(a3)
                       : "rcx", "r11", "memory");
  return ret;
}

static __inline long __syscall4(long n, long a1, long a2, long a3, long a4) {
  unsigned long ret;
  register long r10 __asm__("r10") = a4;
  __asm__ __volatile__("syscall"
                       : "=a"(ret)
                       : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
                       : "rcx", "r11", "memory");
  return ret;
}

static __inline long __syscall5(long n, long a1, long a2, long a3, long a4,
                                long a5) {
  unsigned long ret;
  register long r10 __asm__("r10") = a4;
  register long r8 __asm__("r8") = a5;
  __asm__ __volatile__("syscall"
                       : "=a"(ret)
                       : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
                       : "rcx", "r11", "memory");
  return ret;
}

static __inline long __syscall6(long n, long a1, long a2, long a3, long a4,
                                long a5, long a6) {
  unsigned long ret;
  register long r10 __asm__("r10") = a4;
  register long r8 __asm__("r8") = a5;
  register long r9 __asm__("r9") = a6;
  __asm__ __volatile__("syscall"
                       : "=a"(ret)
                       : "a"(n), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8),
                         "r"(r9)
                       : "rcx", "r11", "memory");
  return ret;
}

void printNum(uint64_t num) {
  char frd[50] = {0};
  itoa((uint64_t)(num), frd);
  __syscall3(1, 1, frd, strlength(frd));
}

// static __thread int fr = 2;
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

  // float hehe = 0.755;
  // hehe += 1;
  // hehe /= 2;

  char *nl = "\n";
  for (int i = 0; i < argc; i++) {
    __syscall3(1, 1, (size_t)argv[i], strlength(argv[i]));
    __syscall3(1, 1, (size_t)nl, 1);
  }

  // fr += 3;
  // printNum(fr);
  // while (1) {
  // }

  // char buf[160] = {0};
  // __syscall2(79, buf, 160);
  // __syscall3(1, 1, buf, 160);

  // char str[] = "\nadded (via append) from userspace\n";
  // int  fd = __syscall3(2, "/files/ab.txt", 0x01 | 0x02 | 0x30, 0);
  // __syscall3(1, fd, str, sizeof(str) / sizeof(str[0]));
  // printNum(fd);

  // printNum(argv);

  // asm volatile("cli");

  // while (1) {
  // }
}

void _start_c(uint64_t rsp) {
  main(*(int *)(rsp), rsp + 8);
  __syscall1(60, 0);
}