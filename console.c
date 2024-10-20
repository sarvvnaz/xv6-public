// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#define leftarrow 0xe4
#define rightarrow 0xe5
#define left 1
#define right 0
#define NUMCOL 80
#define NUMROW 25
#define INPUT_BUF 128

static void consputc(int);
static int capacity = 0;
static int panicked = 0;

static struct {
  char buf[INPUT_BUF];
  int len;
  int copying;  // Flag to indicate if we're currently copying
} clipboard;

static struct {
  struct spinlock lock;
  int locking;
} cons;



static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do{
    buf[i++] = digits[x % base];
  }while((x /= base) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0)
    consputc(buf[i]);
}
//PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if(locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint*)(void*)(&fmt + 1);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if((s = (char*)*argp++) == 0)
        s = "(null)";
      for(; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if(locking)
    release(&cons.lock);
}

void
panic(char *s)
{
  int i;
  uint pcs[10];

  cli();
  cons.locking = 0;
  // use lapiccpunum so that we can call panic from mycpu()
  cprintf("lapicid %d: panic: ", lapicid());
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for(i=0; i<10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for(;;)
    ;
}

//PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory

static void
cgaputc(int c)
{
  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT+1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT+1);

  if(c == '\n')
    pos += 80 - pos%80;
  else if(c == BACKSPACE){
    if(pos > 0) --pos;
  } else
    crt[pos++] = (c&0xff) | 0x0700;  // black on white

  if(pos < 0 || pos > 25*80)
    panic("pos under/overflow");

  if((pos/80) >= 24){  // Scroll up.
    memmove(crt, crt+80, sizeof(crt[0])*23*80);
    pos -= 80;
    memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT+1, pos>>8);
  outb(CRTPORT, 15);
  outb(CRTPORT+1, pos);
  crt[pos] = ' ' | 0x0700;
}

void
consputc(int c)
{
  if(panicked){
    cli();
    for(;;)
      ;
  }

  if(c == BACKSPACE){
    uartputc('\b'); uartputc(' '); uartputc('\b');
  } else
    uartputc(c);
  cgaputc(c);
}

#define INPUT_BUF 128
struct {
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} input;

#define C(x)  ((x)-'@')  // Control-x

void
consoleintr(int (*getc)(void))
{
  int c, doprocdump = 0;

  acquire(&cons.lock);
  while((c = getc()) >= 0){
    switch(c){
    case C('P'):  // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      doprocdump = 1;
      break;
    case C('U'):  // Kill line.
      while(input.e != input.w &&
            input.buf[(input.e-1) % INPUT_BUF] != '\n'){
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    case C('H'): case '\x7f':  // Backspace
      if(input.e != input.w){
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    case leftarrow : //leftarrow
      if(input.e > input.r) {
        setcursor(left);
      }
      break;
    case rightarrow : //rightarrow
        setcursor(right);
      
      break;

    case C('S'):  // Start copying to clipboard
      clipboard.len = 0;
      clipboard.copying = 1;
      break;
    case C('F'):  // Stop copying and paste
      if (clipboard.copying) {
        clipboard.copying = 0;
        // Paste the clipboard contents
        for (int i = 0; i < clipboard.len; i++) {
          if (input.e-input.r < INPUT_BUF) {
            c = clipboard.buf[i];
            input.buf[input.e++ % INPUT_BUF] = c;
            consputc(c);
          }
        }
      }
      break;
    default:
      if (c == '\n') {
        capacity =0;
        math_process();
      }
      if(c != 0 && input.e-input.r < INPUT_BUF){
        c = (c == '\r') ? '\n' : c;
        input.buf[input.e++ % INPUT_BUF] = c;
        consputc(c);
        if (clipboard.copying && clipboard.len < INPUT_BUF) {
          clipboard.buf[clipboard.len++] = c;
        }
        if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
          input.w = input.e;
          wakeup(&input.r);
        }
      }
      break;
    }
  }
  release(&cons.lock);
  if(doprocdump) {
    procdump();  // now call procdump() wo. cons.lock held
  }
}

int
consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while(n > 0){
    while(input.r == input.w){
      if(myproc()->killed){
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if(c == C('D')){  // EOF
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

int
consolewrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for(i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

void
consoleinit(void)
{
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
}

void
setcursor(int direction)
{
    int pos ;
    outb(CRTPORT,14);
    pos = inb(CRTPORT+1) << 8;
    outb(CRTPORT, 15);
    pos |= inb(CRTPORT+1) ;
    int current_row = NUMCOL * (pos / NUMCOL) + 2;
    int current_col = ((((int)pos / NUMCOL) + 1) * NUMCOL - 1);
    if (direction == left) {
      if (crt[pos -2] != (('$' & 0xff) | 0x0700)) {
        pos--;
        capacity++;
        }
    }
    else if (direction == right && capacity > 0) {
      if (pos < current_col) {
        pos++;
        capacity --;
        }
        }
    outb(CRTPORT,14);
    outb(CRTPORT+1,pos >>8);
    outb(CRTPORT, 15);
    outb(CRTPORT+1,pos) ;

}
  
char* get_last_input () 
{
  static char output[INPUT_BUF];
  int start = input.e - 1;
  int end = input.e;
  while(start >= input.r && input.buf[start % INPUT_BUF] != '\n') 
  { 
    start -- ;
  }
  if (start < input.r) 
  {
    start = input.r;
  } else {
    start ++;
  }
  int i = 0;
  while (start < end && i < INPUT_BUF -1) {
    output[i++] = input.buf[start % INPUT_BUF];
    start ++;
  }
  output[i] = '\0';
  return output;
}
int is_digit (char c) {
  return c >= '0' && c <= '9';
}
void int_to_string(int num, char* str) {
  int i =0, is_negative = 0;
  if (num <0) {
    is_negative = 1;
    num = -num;
  }
  do {
    str[i++] = (num %10) + '0';
    num /= 10;
  } while (num >0);
  if (is_negative) {
    str[i++] = '-';
  }
  str[i] = '\0';
  for (int j = 0 ; j < i/2 ; j++) {
    char temp = str[j];
    str[j] = str[i-j-1];
    str[i-j-1] = temp;
   }
}
void math_process () 
{
  char *input_str = get_last_input();
  char *p = input_str;

  while (*p != '\0') {
    if (is_digit(*p)) {
      int num1 = 0, num2 = 0;
      char operator = 0;
      char *num1_start = p;
    
      while(is_digit(*p)) {
        num1 = num1 * 10 + (*p - '0');
        p++;
      }
      if (*p == '+' || *p == '-' || *p == '*' || *p == '/'){
        operator = *p;
        p++;
      } else {
        p++;
        continue;
      }
      if (is_digit(*p)) {
        while(is_digit(*p)) {
          num2 = num2 *10 + (*p -'0');
        }
      }else {
        p++;
        continue;
      }
        
      if (*p == '=' && *(p + 1) == '?') {
        p += 2;
      
        int result = 0;
        switch (operator)
        {
          case '+': result = num1 + num2; break;
          case '-': result = num1 - num2; break;
          case '*': result = num1 * num2; break;
          case'/': if (num2 != 0) result = num1 / num2; break;
          default: continue;
        }
        char resullt_str[16];

        int_to_string(result, resullt_str);
        int shift_len = (p - num1_start) ;
        memmove(num1_start + strlen(resullt_str), p, strlen(p) + 1);
        memcpy(num1_start, resullt_str, strlen(resullt_str));
        p = num1_start + strlen(resullt_str);
        }
        else {
          p++;
        }   
        }else {
          p++;
        }
        
  }

} 