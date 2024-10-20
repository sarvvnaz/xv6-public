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

int first_num = 0;
int sec_num = 0;
int first_start = 0;
int operator = 0;

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
int op_state = 0;

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
    if (c >= 48 && c <= 57){
      switch(op_state){
      case 0 :
        first_start = input.e;
        op_state = 1;
        first_num = first_num * 10 + c - 48;
        break;

      case 1:
        first_num = first_num * 10 + c - 48;
        break;

      case 2:
        op_state = 3;
        sec_num = sec_num * 10 + c - 48;
      break;

      case 3:
        sec_num = sec_num * 10 + c - 48;
        break;

      default:
        break;
      }
    }

    if (c == 45 /*-*/|| c == 43 /*+*/|| c == 42 /*mult*/|| c == 37 /*%*/|| c == 47/*divide*/){
      if (op_state == 1){
        op_state = 2;
        operator = c;
      }
    }
     if (c ==61 /*=*/){
       if(op_state == 3){
         op_state = 4;
        }

    if(c == 63 /*?*/){
      int result = 0;
      if (op_state == 4){

        switch (operator)
        {
        case 45:
          result = first_num - sec_num;
          break;
        
        case 43:
          result = first_num + sec_num;
          break;

        case 42:
          result = first_num * sec_num;
          break;

        case 37:
          result = first_num % sec_num;
          break;

        case 47:
          result = first_num / sec_num;
          break;

        default:
          break;
        }
        
        while(input.e >= first_start){
          consputc(BACKSPACE);
          input.e--;
        }
        int sign_ = 0;
        if (result < 0)
          sign_ = 1;
        printint(result, 10, sign_);
        first_num = 0;
        sec_num = 0;
        first_start = 0;
        operator = 0;
        op_state = 0;
      }
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
  
